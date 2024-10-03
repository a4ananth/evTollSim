#pragma once
#include "eVTOL.h"
#include <string>
#include <vector>
#include <memory>


class Fleet : public eVTOL {
    std::string ManufacturerName;
    std::vector<std::unique_ptr<eVTOL>> AircraftsSpawned;

    double MilesPerSession;
    double FaultsPerSession;
    double PassengerMiles;
public:
    Fleet(const std::string& CarrierName, const json& InputData);

    std::string getManufacturerName(const eVTOL* aircraft) const override;
    double getMilesPerSession() override;
    double getFaultsPerSession() override;
    double getPassengerMiles() override;

    ~Fleet() = default;
};

