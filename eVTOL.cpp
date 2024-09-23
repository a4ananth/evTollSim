#include "eVTOL.h"
#include <chrono>

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
    * Just like drones, for safe operations, the critical minimum is set to 5%
    * and notification minimum is set to 15%. 
    * 
    * Once notification minimum is reached, a request will be sent to the Charging manager
    * to check for available chargers. If a charger is available, the aircraft is navigated to charge.
    * If no charger is available, the aircraft is allowed to cruise until critical minimum while the 
    * charging manager looks for the next spot. Upon critical minimum, the aircraft airtime is halted 
    * and resumes operation only when charging is complete. 
    */

    eVTOL* aircraft = this;

    double startingBatteryLevel = this->currentBatteryLevel;
    int currentSpeed = this->CruiseSpeed;

    std::chrono::duration<double> ActiveDuration = std::chrono::high_resolution_clock::now() - this->StartOperationTime;
    std::chrono::hours DurationInHours = std::chrono::duration_cast<std::chrono::hours>(ActiveDuration);

    double miles = currentSpeed * DurationInHours.count();

    BATTERY_NOTIFICATIONS notifications {};

    return 0.0;
}


double eVTOL::calculatePassengerMiles(int& factor)
{
    eVTOL* aircraft = this;

    std::chrono::hours totalTime = std::chrono::duration_cast<std::chrono::hours>(this->EndOperationTime - this->StartOperationTime);
    int speed = this->CruiseSpeed;
    int passengers = this->maxPassengerCount;

    return (factor * totalTime.count() * speed * passengers);
}
