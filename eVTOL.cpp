#pragma warning(disable : 4996)
#include "ChargingManager.h"
#include "eVTOL.h"

#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>


void eVTOL::startSimulation(std::unique_ptr<eVTOL>& aircraft) {
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
}


void eVTOL::retireSimulation() {
    /*
    * This function will collect all data at the end of the simulation
    * and call the logger to summarize all data points.
    */

    eVTOL* aircraft = this;

}


std::chrono::time_point<std::chrono::system_clock> eVTOL::getEndOperationTime() const {
    return this->EndOperationTime;
}


std::chrono::time_point<std::chrono::system_clock> eVTOL::getStartOperationTime() const {
    return this->StartOperationTime;
}

std::chrono::duration<double> eVTOL::getAirTime() const {
    return std::chrono::duration<double, std::ratio<60>>(this->airTime);
}


std::chrono::seconds eVTOL::getTimeToCharge() const {
    /*
    * This function returns the time to charge set by the manufacturer.
    */

    const eVTOL* aircraft = this;

    std::chrono::seconds TimeToCharge = std::chrono::duration_cast<std::chrono::seconds>(aircraft->TimeToCharge);
    return TimeToCharge;
}


std::string eVTOL::getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const {
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm LocalTime = *std::localtime(&time);

    std::stringstream TimeForLogs;
    TimeForLogs << std::put_time(&LocalTime, "%Y-%m-%d %H:%M:%S");
    return TimeForLogs.str();
}


void eVTOL::updateBatteryLevel() {
    double drainedLevel = 0.0;
    const double OnePercent = this->BatteryCapacity / 100.0;

    while (!simulationCompleted || drainedLevel != this->BatteryCapacity) {
        drainedLevel += this->BatteryDischargeRate;
        if ((drainedLevel % OnePercent) == 0) --currentBatteryLevel;
    }

}


void eVTOL::startAircraft() {
    this->StartOperationTime = std::chrono::system_clock::now();
}


void eVTOL::calculateBatteryDrainRate() {
    double NetConsumptionPerHour = this->CruiseSpeed * this->CruisingPowerConsumption;
    double ConsumptionPerSecond = NetConsumptionPerHour / (60 * 60);

    this->BatteryDischargeRate = ConsumptionPerSecond;
}


double eVTOL::calculatePassengerMiles(const std::size_t& factor) const {

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(airTime);
    int speed = CruiseSpeed;
    int passengers = maxPassengerCount;

    return (factor * totalTime.count() * speed * passengers);
}


void eVTOL::requestCharge(std::unique_ptr<eVTOL>& aircraft) {
    static std::mutex sendToCharger;
    std::mutex receiveFromCharger;
    std::condition_variable ChargingStatus;

    this->EndOperationTime = std::chrono::system_clock::now();
    std::unique_ptr<eVTOL> currentCraft = nullptr;

    {
        std::lock_guard<std::mutex> lock(sendToCharger);
        currentCraft = std::move(aircraft);
        ChargingManager::getInstance()->requestCharger(currentCraft);
    }

    {
        std::unique_lock<std::mutex> lock(receiveFromCharger);
        ChargingStatus.wait(lock, [&] { return currentCraft != nullptr; });
        if (currentCraft) aircraft = std::move(currentCraft);
        aircraft->currentBatteryLevel = 100;
    }
}

eVTOL::eVTOL(const json& InputData) : 
    CruiseSpeed(InputData.at("Cruise_Speed").get<int>()),
    maxPassengerCount(InputData.at("Passenger_Count").get<int>()),
    BatteryCapacity(InputData.at("Battery_Capacity").get<int>()),
    CruisingPowerConsumption(InputData.at("Energy_use_at_Cruise").get<double>()),
    FaultsPerHour(InputData.at("Probability_of_fault_per_hour").get<double>()),
    TimeToCharge(std::chrono::duration<double, std::ratio<3600>>(InputData.at("Time_to_Charge").get<double>())) {

    this->currentBatteryLevel = 100;              // Initialize battery to full capacity
    this->BatteryDischargeRate = 0.0;             // Initialize battery drain rate to 0.0  

    this->airTime = std::chrono::duration<double>::zero();  // Initialize airTime to 0
}
