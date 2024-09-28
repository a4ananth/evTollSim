#include "eVTOL.h"
#include <chrono>
#include <thread>
#include <string>
#include <iostream>


void eVTOL::startSimulation() {
    eVTOL* aircraft = this;
    const std::chrono::duration<double, std::ratio<60>> timeout(20);
    aircraft->StartOperationTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        BATTERY_NOTIFICATIONS notification = aircraft->getCurrentBatteryLevel();
        std::string batteryLevel{}; 
        if (notification == BATTERY_NOTIFICATIONS::POWER_AVAILABLE) batteryLevel = "POWER_AVAILABLE";
        else if (notification == BATTERY_NOTIFICATIONS::LOW_POWER) batteryLevel = "LOW_POWER";
        else batteryLevel = "CRITICAL_MINIMUM";

        std::cout << batteryLevel << std::endl;
    }
}


void eVTOL::resetCharge() {
    /*
    * This function resets the current battery level to 100%.
    */

    eVTOL* aircraft = this;
    aircraft->currentBatteryLevel = 100.0;
}


eVTOL::BATTERY_NOTIFICATIONS eVTOL::trackRemainingCharge() {
    /*
    * This function is designed to keep a track of the current battery usage.
    * the basis of operations is based on time. Hence the minimium power needed for 1 minute
    * is used as the threshold to trigger a charging event.
    *
    * Once notification minimum is reached, a request will be sent to the Charging manager
    * to check for available chargers. If a charger is available, the aircraft is navigated to charge.
    * If no charger is available, the aircraft is allowed to cruise until critical minimum while the
    * charging manager looks for the next spot. Upon critical minimum, the aircraft airtime is halted
    * and resumes operation only when charging is complete.
    */

    eVTOL* aircraft = this;
    BATTERY_NOTIFICATIONS notifications {};

    while (aircraft->currentBatteryLevel > aircraft->BatteryDrainRate()) {
        notifications = aircraft->getCurrentBatteryLevel();
        std::this_thread::sleep_for(std::chrono::minutes(1));
        updateBatteryLevel();
    }

    aircraft->EndOperationTime = std::chrono::high_resolution_clock::now();
    return notifications;
}


std::chrono::seconds eVTOL::getTimeToCharge() {
    /*
    * This function returns the time to charge set by the manufacturer.
    */

    eVTOL* aircraft = this;

    std::chrono::seconds TimeToCharge = std::chrono::duration_cast<std::chrono::seconds>(aircraft->TimeToCharge);
    return TimeToCharge;
}


eVTOL::BATTERY_NOTIFICATIONS eVTOL::getCurrentBatteryLevel() {
    /*
    * This function is part of the getter fmaily of memeber functions.
    * Returns the current battery level in terms of critical levels.
    */
    eVTOL* aircraft = this;

    BATTERY_NOTIFICATIONS notifications {};

    double BatteryLevelPercentage = (aircraft->currentBatteryLevel / aircraft->BatteryCapacity) * 100;
    std::cout << "Current Battery Level %: " << BatteryLevelPercentage << std::endl;
    if (BatteryLevelPercentage > 15) notifications = BATTERY_NOTIFICATIONS::POWER_AVAILABLE;
    else if (BatteryLevelPercentage <= 15 && BatteryLevelPercentage > 5) notifications = BATTERY_NOTIFICATIONS::LOW_POWER;
    else notifications = BATTERY_NOTIFICATIONS::CRITICAL_MINIMUM;

    return notifications;
}


double eVTOL::BatteryDrainRate() {
    eVTOL* aircraft = this;

    double NetConsumptionPerHour = this->CruiseSpeed * this->CruisingPowerConsumtion;
    double ConsumptionPerMinute = NetConsumptionPerHour / 60;

    return ConsumptionPerMinute;
}


void eVTOL::updateBatteryLevel() {
    eVTOL* aircraft = this;

    double currentLevel = aircraft->currentBatteryLevel.load();
    double updatedBatteryLevel{};

    do {
        updatedBatteryLevel = currentLevel - aircraft->BatteryDrainRate();
    } while (!aircraft->currentBatteryLevel.compare_exchange_weak(currentLevel, updatedBatteryLevel));
}

void eVTOL::restartAircraft() {
    eVTOL* aircraft = this;

    aircraft->airTime += (this->EndOperationTime - this->StartOperationTime);
    this->StartOperationTime = std::chrono::high_resolution_clock::now();
}


double eVTOL::calculatePassengerMiles(int& factor)
{
    eVTOL* aircraft = this;

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(this->EndOperationTime - this->StartOperationTime);
    int speed = this->CruiseSpeed;
    int passengers = this->maxPassengerCount;

    return (factor * totalTime.count() * speed * passengers);
}


eVTOL::eVTOL(int CruiseSpeed, int maxPassengerCount, int BatteryCapacity, double CruisingPowerConsumtion, double FaultsPerHour, double ChargeDuration) {

    CruiseSpeed = CruiseSpeed;
    maxPassengerCount = maxPassengerCount;
    BatteryCapacity = BatteryCapacity;
    CruisingPowerConsumtion = CruisingPowerConsumtion;
    FaultsPerHour = FaultsPerHour;

    TimeToCharge = std::chrono::duration<double, std::ratio<3600>>(ChargeDuration);
    numFaults = 0;
    PassengerMiles = 0.0;
    mileagePerCharge = 0;
    currentBatteryLevel = 100.0;


    StartOperationTime = std::chrono::high_resolution_clock::now();
    EndOperationTime = std::chrono::high_resolution_clock::now();
    airTime = std::chrono::duration<double>::zero();
}
