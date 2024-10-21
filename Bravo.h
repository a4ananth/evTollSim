#pragma once
#include "eVTOL.h"
#include <string>
#include <vector>
#include <memory>


class Bravo : public eVTOL {
    std::string ManufacturerName;
    std::vector<std::unique_ptr<eVTOL>> AircraftsSpawned;

    double MilesPerSession;
    double FaultsPerSession;
    double PassengerMiles;
public:
    Bravo(const std::string& CarrierName, const json& InputData);

    std::string getManufacturerName(const eVTOL* aircraft) const override;
    double getMilesPerSession(const eVTOL* aircraft) override;
    double getFaultsPerSession(const eVTOL* aircraft) override;
    double getPassengerMiles(const eVTOL* aircraft) override;

    ~Bravo() = default;
};

