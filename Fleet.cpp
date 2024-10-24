#pragma once

#include <thread>
#include <memory>
#include <sstream>

#include "Fleet.h"
#include "DataManager.h"
#include "FleetManager.h"
#include "ChargingManager.h"


std::atomic<bool> Fleet::simulationCompleted = false;
std::size_t Fleet::NumberOfAircraftsSpawned = 0;
std::mutex Fleet::sendToCharger;


Fleet::Fleet(const json& InputData) :
    CruiseSpeed(InputData.at("Cruise_Speed").get<int>()),
    maxPassengerCount(InputData.at("Passenger_Count").get<int>()),
    BatteryCapacity(InputData.at("Battery_Capacity").get<int>()),
    CruisingPowerConsumption(InputData.at("Energy_use_at_Cruise").get<double>()),
    FaultsPerHour(InputData.at("Probability_of_fault_per_hour").get<double>()),
    TimeToCharge(std::chrono::duration<double, std::ratio<3600>>(InputData.at("Time_to_Charge").get<double>())) {

    this->isCharging = false;                     // Set initial charging state to false
    this->PassengerMiles = 0.0;                   // Initialize passenger miles to 0
    this->MilesTravelled = 0.0;                   // Initialize miles travelled to 0
    this->currentBatteryLevel = 100;              // Initialize battery to full capacity
    this->BatteryDischargeRate = 0.0;             // Initialize battery drain rate to 0.0
    this->FaultsPerSession = 0;                   // Initialize number of faults per session to 0
    this->NumberOfAircraftsSpawned = 0;           // Initialize number of aircrafts spawned to 0

    this->airTime = std::chrono::duration<double>::zero();  // Initialize airTime to 0
}


void Fleet::startAircraft() {
    if(!simulationCompleted.load()) this->StartOperationTime = std::chrono::system_clock::now();
}


void Fleet::updateBatteryLevel() {
    if (!simulationCompleted.load()) {
        Fleet* aircraft = this;

        double drainedLevel = 0.0;
        const double OnePercent = aircraft->BatteryCapacity / 100.0;

        while (!simulationCompleted.load() || (aircraft->currentBatteryLevel > 0)) {
            // sync with time
            drainedLevel += aircraft->BatteryDischargeRate;
            if (std::fmod(drainedLevel, OnePercent) == 0.0) aircraft->currentBatteryLevel--;
        }
    }
}


void Fleet::calculateBatteryDrainRate() {
    if (!simulationCompleted.load()) {
        Fleet* aircraft = this;

        double NetConsumptionPerHour = aircraft->CruiseSpeed * aircraft->CruisingPowerConsumption;
        double ConsumptionPerSecond = NetConsumptionPerHour / (60 * 60);

        aircraft->BatteryDischargeRate = ConsumptionPerSecond;
    }
}


void Fleet::startSimulation(std::shared_ptr<Fleet>& aircraft) {
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

    while (!simulationCompleted.load() && !aircraft->isCharging) {
        aircraft->startAircraft();
        aircraft->updateBatteryLevel();
        aircraft->requestCharge(aircraft);
    }

}


bool Fleet::completeSimulation() {
    if (!simulationCompleted.load()) simulationCompleted.store(true);
    return simulationCompleted.load();
}


void Fleet::requestCharge(std::shared_ptr<Fleet>& aircraft) {
    if (!simulationCompleted.load()) {
        std::mutex receiveFromCharger;
        std::condition_variable ChargingStatus;

        std::shared_ptr<Fleet> currentCraft = nullptr;

        aircraft->EndOperationTime = std::chrono::system_clock::now();
        aircraft->airTime = aircraft->EndOperationTime - aircraft->StartOperationTime;
        aircraft->MilesTravelled = aircraft->CruiseSpeed * (aircraft->airTime.count() / 3600);
        aircraft->isCharging = true;

        //DataManager* logger = DataManager::getInstance(aircraft->getManufacturerName());
        //logger->createLogData(aircraft);

        if (!simulationCompleted.load()) {
            std::lock_guard<std::mutex> lock(sendToCharger);
            currentCraft = std::move(aircraft);
            ChargingManager::getInstance()->requestCharger(currentCraft);
        }

        std::unique_lock<std::mutex> lock(receiveFromCharger);
        ChargingStatus.wait(lock, [&] { return currentCraft != nullptr || simulationCompleted.load(); });
        if (currentCraft) {
            aircraft = std::move(currentCraft);
            aircraft->currentBatteryLevel = 100;
            aircraft->isCharging = false;
        }
    }
}


std::chrono::time_point<std::chrono::system_clock> Fleet::getEndOperationTime() const {
    return this->EndOperationTime;
}


std::chrono::time_point<std::chrono::system_clock> Fleet::getStartOperationTime() const {
    return this->StartOperationTime;
}


std::chrono::seconds Fleet::getTimeToCharge() const {
    /*
    * This function returns the time to charge set by the manufacturer.
    */

    const Fleet* aircraft = this;

    std::chrono::seconds TimeToCharge = std::chrono::duration_cast<std::chrono::seconds>(aircraft->TimeToCharge);
    return TimeToCharge;
}


std::string Fleet::getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const {
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm LocalTime; 
    localtime_s(&LocalTime, &time);

    std::stringstream TimeForLogs;
    TimeForLogs << std::put_time(&LocalTime, "%Y-%m-%d %H:%M:%S");
    return TimeForLogs.str();
}


double Fleet::getPassengerMiles() const {

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(airTime);
    int speed = CruiseSpeed;
    int passengers = maxPassengerCount;

    return (totalTime.count() * speed * passengers);
}


void Fleet::constructAircrafts(const int& numAircrafts, const json& InputData) {
    NumberOfAircraftsSpawned = numAircrafts;
    for (std::size_t i = 0; i < NumberOfAircraftsSpawned; ++i) {
        std::shared_ptr<Fleet> Instance = std::make_shared<Fleet>(InputData);
        FleetManager::getInstance()->fillFleet(Instance);
    }

}


std::string Fleet::getManufacturerName() const {
    return this->ManufacturerName;
}


double Fleet::getMilesPerSession() const {
    /*
    * This function is written for logging the Miles travelled per session.
    * The result is rounded off and then truncated to 2 decimal places
    */

    double factor = std::pow(10, 2);
    return std::round(this->MilesTravelled * factor) / factor;
}


double Fleet::getFaultsPerSession() const {
    return (this->airTime.count() / 3600);
}


double Fleet::calculatePassengerMiles() const {
    double totalTime = (this->airTime.count() / 3600);
    int speed = CruiseSpeed;
    int passengers = maxPassengerCount;

    return (totalTime * speed * passengers * NumberOfAircraftsSpawned);
}

Fleet::Fleet() :
    ManufacturerName(""),
    CruiseSpeed(0),
    maxPassengerCount(0),
    BatteryCapacity(0),
    CruisingPowerConsumption(0.0),
    FaultsPerHour(0.0),
    TimeToCharge(2.0) 
{
    this->isCharging = false;                     // Set initial charging state to false
    this->PassengerMiles = 0.0;                   // Initialize passenger miles to 0
    this->MilesTravelled = 0.0;                   // Initialize miles travelled to 0
    this->currentBatteryLevel = 100;              // Initialize battery to full capacity
    this->BatteryDischargeRate = 0.0;             // Initialize battery drain rate to 0.0
    this->FaultsPerSession = 0;                   // Initialize number of faults per session to 0
    this->NumberOfAircraftsSpawned = 0;           // Initialize number of aircrafts spawned to 0

    this->airTime = std::chrono::duration<double>::zero();  // Initialize airTime to 0
}
