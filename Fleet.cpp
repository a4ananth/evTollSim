#include "Fleet.h"
#include <chrono>


Fleet::Fleet(const std::string& CarrierName, const json& InputData) :
    ManufacturerName(CarrierName), eVTOL(InputData) 
{
    this->MilesPerSession = 0.0;
    this->FaultsPerSession = 0.0;
    this->PassengerMiles = 0.0;
}


std::string Fleet::getManufacturerName(const eVTOL* aircraft) const {
    return ManufacturerName;
}


double Fleet::getMilesPerSession()  {
    MilesPerSession += (getBatteryDrainRate() * getAirTime().count());
    return MilesPerSession;
}

double Fleet::getFaultsPerSession() {
    FaultsPerSession += getAirTime().count() / 3600;
    return FaultsPerSession;
}

double Fleet::getPassengerMiles() {
    std::size_t factor = AircraftsSpawned.size();
    PassengerMiles += calculatePassengerMiles(factor);
    return PassengerMiles;
}


