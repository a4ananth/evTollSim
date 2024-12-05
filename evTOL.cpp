#include <cmath>
#include <thread>
#include <fstream>
#include <sstream>
#include <condition_variable>

#include "evTOL.h"
#include "DataLogger.h"
#include "RequestManager.h"


std::atomic<bool> evTOL::simulationComplete{ false };
std::condition_variable evTOL::aircraftCV;

/* ----------------- Constructors ----------------- */

evTOL::evTOL(const json& InputData) :
    ManufacturerName(InputData.at("Name").get<std::string>()),
    CruiseSpeed(InputData.at("Cruise_Speed").get<int>()),
    maxPassengerCount(InputData.at("Passenger_Count").get<int>()),
    BatteryCapacity(InputData.at("Battery_Capacity").get<int>()),
    CruisingPowerConsumption(InputData.at("Energy_use_at_Cruise").get<double>()),
    FaultsPerHour(InputData.at("Probability_of_fault_per_hour").get<double>()),
    TimeToCharge(std::chrono::duration<double, std::ratio<3600>>(InputData.at("Time_to_Charge").get<double>()))
{
    this->currentBatteryLevel = 100;                        // Initialize current battery level to 100%
    this->chargingStatus.store(false);                      // Initialize the charging status to false

    this->airTime = std::chrono::duration<double>::zero();                            // Initialize airTime to 0
	this->EndOperationTime = std::chrono::time_point<std::chrono::system_clock>();    // Initialize end time to 0
    this->StartOperationTime = std::chrono::time_point<std::chrono::system_clock>();  // Initialize start time to 0	
}


/* -------------------- Protected APIs -------------------- */

void evTOL::startAircraft() {
    std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->shared_from_this());
    logger->logData("Starting the aircraft.");
    if (!chargingStatus.load()) StartOperationTime = std::chrono::system_clock::now();
}


void evTOL::updateBatteryLevel() {
    std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->shared_from_this());

    if (!chargingStatus.load()) {
        double NetConsumptionPerHour = CruiseSpeed * CruisingPowerConsumption;
        double ConsumptionPerSecond = NetConsumptionPerHour / (60 * 60);

        double accumulatedConsumption = 0.0;
        double OnePercent = BatteryCapacity * 0.01;

        while (currentBatteryLevel > 0 && !simulationComplete.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            accumulatedConsumption += ConsumptionPerSecond;

            if (accumulatedConsumption >= OnePercent) {
                accumulatedConsumption -= OnePercent;
                currentBatteryLevel--;
            }
        }

		logger->logData("Battery level of aircraft has drained to : " + std::to_string(currentBatteryLevel) + " %.");
    }
}


std::string evTOL::requestCharge(std::shared_ptr<evTOL>& aircraft) {
    std::string request{};
    std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->shared_from_this());

    if (!chargingStatus.load()) {
        EndOperationTime = std::chrono::system_clock::now();
		airTime = getEndOperationTime() - getStartOperationTime();
        logger->logData("This aircraft has requested to be charged. Setting Charging status to : TRUE.");
        chargingStatus.store(true);
        request = RequestManager::createChargingRequest(aircraft);
    }

    return request;
}


void evTOL::receiveFromCharger(const std::string& ticketNumber) {
    bool aircraftReceived = false;
	std::shared_ptr<RequestManager> request = RequestManager::getRequest(ticketNumber);
    std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->shared_from_this());

    while (chargingStatus.load()) {
        std::unique_lock<std::mutex> rxLock(aircraftMtx);
        evTOL::aircraftCV.wait(rxLock, [&] {
            if(request) aircraftReceived = request->thankyou();
            return (aircraftReceived && chargingStatus.load()) || simulationComplete.load();
            });

        if (aircraftReceived && request) {
			logger->performanceSummary(this->shared_from_this());
            chargingStatus.store(false);
            currentBatteryLevel = 100;
            logger->logData("Aircraft received from charging station.");
        }
        else if (simulationComplete.load()) {
            logger->logData("Simulation complete");
            break;
        }
    }

}



/* -------------------- Public APIs -------------------- */

void evTOL::startSimulation() {
    /*
    * This function officially starts the simulation for the aircrafts.
    * This simulation will keep restarting until the timeout is complete.
    *
    * A few things to note for this simulation flow:
    *   1. Once the simulation is started, the aircraft is assumed to be airborne
    *      for the whole duration until the charge drops to 1%.
    *   2. Start time is updated to current time when the simulation starts.
    *   3. As soon as the battery drains to 1%, the aircraft is queued to the charger.
    *   4. Aircraft charges for TimeToCharge duration and is made available again.
    *   5. repeat steps 1 - 5.
    */

	std::string ticketNumber{};
    std::shared_ptr<evTOL> aircraft = this->shared_from_this();
    std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->shared_from_this());

    while (!simulationComplete.load()) {
        if (!chargingStatus.load()) {
            if (currentBatteryLevel <= 1) {
                logger->logData("Battery level is less than 1%. Preparing to dispatch to charger network.");
                ticketNumber = requestCharge(aircraft);
				chargerThread = std::thread(&evTOL::receiveFromCharger, this, std::ref(ticketNumber));
            }
            else {
				if (chargerThread.joinable()) chargerThread.join();
                startAircraft();
                updateBatteryLevel();
            }
        }
        else {
			if (chargerThread.joinable()) chargerThread.join();
        }
    }

}


void evTOL::retireSimulation() {
	simulationComplete.store(true);
	evTOL::aircraftCV.notify_all();
}


int evTOL::getCruiseSpeed() const {
    return CruiseSpeed;
}


int evTOL::getMaxPassengerCount() const {
	return maxPassengerCount;
}


std::string evTOL::get_manufacturer() const {
    return ManufacturerName;
}


std::condition_variable& evTOL::getAircraftCV() {
    return aircraftCV;
}


std::chrono::microseconds evTOL::getTimeToCharge() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(TimeToCharge) / 1000000;
}


std::chrono::duration<double> evTOL::getAirTime() const {
    return airTime;
}


std::chrono::time_point<std::chrono::system_clock> evTOL::getEndOperationTime() const {
    return EndOperationTime;
}


std::chrono::time_point<std::chrono::system_clock> evTOL::getStartOperationTime() const {
    return StartOperationTime;
}


std::string evTOL::getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const {
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm LocalTime;
    localtime_s(&LocalTime, &time);

    std::stringstream TimeForLogs{};
    TimeForLogs << std::put_time(&LocalTime, "%Y-%m-%d %H:%M:%S");

    return TimeForLogs.str();
}