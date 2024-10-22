#pragma once

#include "Fleet.h"


Fleet::Fleet(const std::string& CarrierName, const json& InputData) :
    ManufacturerName(CarrierName), eVTOL(InputData) 
{
    this->MilesPerSession = 0.0;
    this->FaultsPerSession = 0.0;
    this->PassengerMiles = 0.0;
    ++NumberOfAircraftsSpawned;
}


std::string Fleet::getManufacturerName() const {
    return this->ManufacturerName;
}


double Fleet::getMilesPerSession() const {
    return this->getMilesTravelled();
}


double Fleet::getFaultsPerSession() const {
    return (this->getAirTime().count() / 3600);
}


double Fleet::calculatePassengerMiles() const {
    return (this->getPassengerMiles() * NumberOfAircraftsSpawned);
}


