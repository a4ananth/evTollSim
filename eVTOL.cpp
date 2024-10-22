#pragma once

#include <thread>
#include <sstream>

#include "eVTOL.h"


std::atomic<bool> eVTOL::simulationCompleted = false;


void eVTOL::startAircraft() {
    this->StartOperationTime = std::chrono::system_clock::now();
}


void eVTOL::updateBatteryLevel() {
    eVTOL* aircraft = this;

    double drainedLevel = 0.0;
    const double OnePercent = aircraft->BatteryCapacity / 100.0;

    while (!simulationCompleted || (aircraft->currentBatteryLevel > 0)) {
        drainedLevel += aircraft->BatteryDischargeRate;
        if (std::fmod(drainedLevel, OnePercent) == 0.0) aircraft->currentBatteryLevel--;
    }
}


void eVTOL::calculateBatteryDrainRate() {
    eVTOL* aircraft = this;

    double NetConsumptionPerHour = aircraft->CruiseSpeed * aircraft->CruisingPowerConsumption;
    double ConsumptionPerSecond = NetConsumptionPerHour / (60 * 60);

    aircraft->BatteryDischargeRate = ConsumptionPerSecond;
}


void eVTOL::requestCharge(std::unique_ptr<eVTOL>& aircraft) {
    static std::mutex sendToCharger;
    std::mutex receiveFromCharger;
    std::condition_variable ChargingStatus;

    std::unique_ptr<eVTOL> currentCraft = nullptr;

    aircraft->EndOperationTime = std::chrono::system_clock::now();
    aircraft->airTime = aircraft->EndOperationTime - aircraft->StartOperationTime;
    aircraft->MilesTravelled = aircraft->CruiseSpeed * (aircraft->airTime.count() / 3600);
    aircraft->isCharging = true;

    {
        std::lock_guard<std::mutex> lock(sendToCharger);
        currentCraft = std::move(aircraft);
    }

    ChargingManager::getInstance()->requestCharger(currentCraft);

    std::unique_lock<std::mutex> lock(receiveFromCharger);
    ChargingStatus.wait(lock, [&] { return currentCraft != nullptr; });
    if (currentCraft) {
        aircraft = std::move(currentCraft);
        aircraft->currentBatteryLevel = 100;
        aircraft->isCharging = false;
    }
}


void eVTOL::retireSimulation(std::unique_ptr<eVTOL>& aircraft) {
    /*
    * This function will collect all data at the end of the simulation
    * and call the logger to summarize all data points.
    */

    ChargingManager::completeSimulation();

    /*
    * If aircraft is in charging state, the ChargingManager class will take care
    * of calling the logger for the particular aircraft and subsequently release the 
    * aircraft to be properly destroyed from the scope of the simulation.
    * 
    * If aircraft is air-borne, then it needs to land and trigger the end of simulation
    * procedure on itself
    */

    if (!aircraft->isCharging) {
        aircraft->EndOperationTime = std::chrono::system_clock::now();
        DataManager* logger = DataManager::getInstance(aircraft->getManufacturerName());
        logger->createLogData(aircraft);
    }

}


std::chrono::time_point<std::chrono::system_clock> eVTOL::getEndOperationTime() const {
    return this->EndOperationTime;
}


std::chrono::time_point<std::chrono::system_clock> eVTOL::getStartOperationTime() const {
    return this->StartOperationTime;
}


std::chrono::duration<double> eVTOL::getAirTime() const {
    return std::chrono::duration<double, std::ratio<3600>>(this->airTime);
}


double eVTOL::getMilesTravelled() const {
    /*
    * This function is written for logging the Miles travelled per session.
    * The result is rounded off and then truncated to 2 decimal places
    */

    double factor = std::pow(10, 2);
    return std::round(this->MilesTravelled * factor) / factor;
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


double eVTOL::getPassengerMiles() const {

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(airTime);
    int speed = CruiseSpeed;
    int passengers = maxPassengerCount;

    return (totalTime.count() * speed * passengers);
}


eVTOL::eVTOL(const json& InputData) : 
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

    this->airTime = std::chrono::duration<double>::zero();  // Initialize airTime to 0
}


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

    while (!simulationCompleted) {
        aircraft->startAircraft();
        aircraft->updateBatteryLevel();
        aircraft->requestCharge(aircraft);
    }

    aircraft->retireSimulation(aircraft);
}