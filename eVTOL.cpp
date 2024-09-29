#include "ChargingManager.h"
#include "eVTOL.h"

#include <chrono>
#include <thread>


void eVTOL::startSimulation() {
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
    * 
    *   5. repeat steps 1 - 5.
    */


    eVTOL* aircraft = this;
    ChargingManager charger(3);
    const std::chrono::duration<double, std::ratio<60>> timeout(90);
    aircraft->StartOperationTime = std::chrono::high_resolution_clock::now();

    std::chrono::time_point<std::chrono::steady_clock> SimulationStartTime = std::chrono::steady_clock::now();

    while ((SimulationStartTime - std::chrono::steady_clock::now()) <= timeout) {
        std::thread BatteryManager(&eVTOL::trackRemainingCharge, this);

        while (BatteryManager.joinable()) {
            if (aircraft->currentBatteryLevel <= (0.01 * aircraft->BatteryCapacity)) {
                BatteryManager.join();
                break;
            }
        }

        // If for any reason the loop breaks abrupty from the main call, join here to avoid leaks
        if (BatteryManager.joinable()) BatteryManager.join();

        charger.chargeAircraft(aircraft);
    }

}


void eVTOL::resetCharge() {
    /*
    * This function resets the current battery level to 100%.
    */

    eVTOL* aircraft = this;
    aircraft->currentBatteryLevel = aircraft->BatteryCapacity;

    aircraft->restartAircraft();
}


void eVTOL::trackRemainingCharge() {
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

    while (aircraft->currentBatteryLevel > aircraft->BatteryDrainRate()) {
        std::chrono::seconds TimePerMile(aircraft->TimeToTravelOneMile());
        std::this_thread::sleep_for(TimePerMile);
        updateBatteryLevel();
    }

    aircraft->EndOperationTime = std::chrono::high_resolution_clock::now();
}


std::chrono::seconds eVTOL::getTimeToCharge() const {
    /*
    * This function returns the time to charge set by the manufacturer.
    */

    const eVTOL* aircraft = this;

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

    if (BatteryLevelPercentage > 15) notifications = BATTERY_NOTIFICATIONS::POWER_AVAILABLE;
    else if (BatteryLevelPercentage <= 15 && BatteryLevelPercentage > 5) notifications = BATTERY_NOTIFICATIONS::LOW_POWER;
    else notifications = BATTERY_NOTIFICATIONS::CRITICAL_MINIMUM;

    return notifications;
}


double eVTOL::BatteryDrainRate() const {
    const eVTOL* aircraft = this;

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
        if (updatedBatteryLevel < 0) updatedBatteryLevel = 0;
    } while (!aircraft->currentBatteryLevel.compare_exchange_weak(currentLevel, updatedBatteryLevel));
}


void eVTOL::restartAircraft() {
    eVTOL* aircraft = this;

    aircraft->airTime += (this->EndOperationTime - this->StartOperationTime);
    this->StartOperationTime = std::chrono::high_resolution_clock::now();
}


std::chrono::seconds eVTOL::TimeToTravelOneMile() const {
    const eVTOL* aircraft = this;

    return std::chrono::seconds(3600 / aircraft->CruiseSpeed);
}


double eVTOL::calculatePassengerMiles(int& factor)
{
    eVTOL* aircraft = this;

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(this->EndOperationTime - this->StartOperationTime);
    int speed = this->CruiseSpeed;
    int passengers = this->maxPassengerCount;

    return (factor * totalTime.count() * speed * passengers);
}


eVTOL::eVTOL(int CruiseSpeed, int maxPassengerCount, int BatteryCapacity, double CruisingPowerConsumtion, double FaultsPerHour, double ChargeDuration) :
    CruiseSpeed(CruiseSpeed), maxPassengerCount(maxPassengerCount), BatteryCapacity(BatteryCapacity), CruisingPowerConsumtion(CruisingPowerConsumtion), FaultsPerHour(FaultsPerHour), TimeToCharge(std::chrono::duration<double, std::ratio<3600>>(ChargeDuration))
{
    this->numFaults = 0;
    this->PassengerMiles = 0.0;
    this->mileagePerCharge = 0;
    this->currentBatteryLevel = BatteryCapacity;

    this->airTime = std::chrono::duration<double>::zero();
}
