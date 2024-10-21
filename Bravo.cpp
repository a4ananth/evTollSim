#include "Bravo.h"
#include <chrono>


Bravo::Bravo(const std::string& CarrierName, const json& InputData) :
    ManufacturerName(CarrierName), eVTOL(InputData) 
{
    this->MilesPerSession = 0.0;
    this->FaultsPerSession = 0.0;
    this->PassengerMiles = 0.0;
}


std::string Bravo::getManufacturerName(const eVTOL* aircraft) const {
    return ManufacturerName;
}


double Bravo::getMilesPerSession(const eVTOL* aircraft) {
    MilesPerSession += (aircraft->getBatteryDrainRate() * aircraft->getAirTime().count());
    return MilesPerSession;
}

double Bravo::getFaultsPerSession(const eVTOL* aircraft) {
    FaultsPerSession += aircraft->getAirTime().count() / 3600;
    return FaultsPerSession;
}

double Bravo::getPassengerMiles(const eVTOL* aircraft) {
    std::size_t factor = AircraftsSpawned.size();
    PassengerMiles += aircraft->calculatePassengerMiles(factor);
    return PassengerMiles;
}


