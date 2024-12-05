#pragma once

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "evTOL.h"
#include "RequestManager.h"
#include "ChargingStation.h"

using json = nlohmann::json;

class FleetManager : public evTOL {
public:
	static void InitializeFleet(const std::size_t& numAircrafts);	// Initialize the fleet
	static void stopSimulation();									// Stop the simulation

	void setManufacturerName(const std::size_t sNo);				// Set the manufacturer name

	double getPassengerMiles() const override;						// Get the passenger miles
	double getMilesPerSession() const override; 					// Get the miles per session
	double getFaultsPerSession() const override;					// Get the faults per session
	std::string getManufacturerName() const override;				// Get the manufacturer name

	FleetManager() = default;										// Default constructor
    FleetManager(const json& AircraftData, const std::size_t sNo);	// Parametrized constructor

protected:
	void readInputData();											// Read input data
	std::string generateSerialNumber() const;						// Generate serial number
	void assignCapacity(const std::size_t& fleetSize);				// Assign capacity to the fleet
	void constructFleet(const std::size_t& numVehicles);			// Construct the fleet

private:
	FleetManager(const FleetManager& other) = delete;				// Copy constructor
	FleetManager& operator= (const FleetManager& other) = delete;	// Copy assignment operator
	FleetManager(FleetManager&& other) = delete;					// Move constructor
	FleetManager& operator= (FleetManager&& other) = delete;		// Move assignment operator

	std::string modelNumber;									// Name of the manufacturer

	static std::unique_ptr<FleetManager> instance;					// Unique pointer to the FleetManager instance
	
	static std::once_flag initialized;									// Flag to ensure that the fleet is initialized only once
	static std::size_t numManufacturers;								// Number of manufacturers
	static std::vector<std::thread> fleetThreads;						// Vector of threads to manage the fleet
	static std::unordered_map<std::string, json> fleetData;				// Map to record fleet json data
	static std::unordered_map<std::string, std::size_t> fleetSizes;		// Map to record fleet sizes
};

