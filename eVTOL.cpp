#include "eVTOL.h"
#include <chrono>
#include <thread>


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
    BATTERY_NOTIFICATIONS notifications = BATTERY_NOTIFICATIONS::POWER_AVAILABLE;

    while (aircraft->currentBatteryLevel > aircraft->BatteryDrainRate()) {
        double BatteryLevelPercentage = (aircraft->currentBatteryLevel / aircraft->BatteryCapacity) * 100;

        if (BatteryLevelPercentage >= 15) notifications = BATTERY_NOTIFICATIONS::POWER_AVAILABLE;
        else if (BatteryLevelPercentage < 15 && BatteryLevelPercentage >= 5) notifications = BATTERY_NOTIFICATIONS::LOW_POWER;
        else notifications = BATTERY_NOTIFICATIONS::CRITICAL_MINIMUM;

        std::this_thread::sleep_for(std::chrono::minutes(1));
        aircraft->currentBatteryLevel -= aircraft->BatteryDrainRate();
    }

    return notifications;
}


std::chrono::seconds eVTOL::getTimeToCharge() {
    eVTOL* aircraft = this;

    std::chrono::seconds TimeToCharge = std::chrono::duration_cast<std::chrono::seconds>(aircraft->TimeToCharge);
    return TimeToCharge;
}


double eVTOL::BatteryDrainRate() {
    eVTOL* aircraft = this;

    double NetConsumptionPerHour = this->CruiseSpeed * this->CruisingPowerConsumtion;
    double ConsumptionPerMinute = NetConsumptionPerHour / 60;

    return ConsumptionPerMinute;
}


double eVTOL::calculatePassengerMiles(int& factor)
{
    eVTOL* aircraft = this;

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(this->EndOperationTime - this->StartOperationTime);
    int speed = this->CruiseSpeed;
    int passengers = this->maxPassengerCount;

    return (factor * totalTime.count() * speed * passengers);
}