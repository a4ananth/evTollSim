#pragma once

#include "eVTOL.h"
#include <string>
#include <memory>


class Fleet : public eVTOL {
    std::string ManufacturerName;
    std::size_t NumberOfAircraftsSpawned;

    double MilesPerSession;
    double FaultsPerSession;
    double PassengerMiles;
public:
    Fleet(const std::string& CarrierName, const json& InputData);

    double getMilesPerSession() const override;
    double getFaultsPerSession() const override;
    double calculatePassengerMiles() const override;
    std::string getManufacturerName() const override;

    ~Fleet() = default;
};

