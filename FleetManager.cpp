#pragma once

#include <ctime>
#include <memory>
#include <cstdlib>
#include <algorithm>

#include "DataManager.h"
#include "FleetManager.h"


FleetManager* FleetManager::FleetMgrInstance = nullptr;
bool FleetManager::isInitialized = false;


FleetManager::FleetManager(const std::size_t& numAircrafts) {
    if (!isInitialized) {
        Aircrafts.reserve(numAircrafts);
        aircraftThreads.reserve(numAircrafts);
    }
}


std::vector<int> FleetManager::generateRandomCapacityForFleets(const std::size_t& totalAircrafts) {
	// Seed the random number generator
	std::srand(static_cast<unsigned int>(std::time(NULL)));

	// We will distribute (totalAircrafts - count) among the numbers
	std::size_t count = DataManager::getInstance(std::nullopt)->getNumberOfManufacturers();
	std::size_t adjusted_sum = totalAircrafts - count;

	// Generate 4 random cut points between 0 and adjusted_sum
	std::vector<int> cut_points;
	for (std::size_t i = 0; i < count - 1; ++i) cut_points.emplace_back((std::rand() % adjusted_sum) + 1);

	// Sort the cut points
	std::sort(cut_points.begin(), cut_points.end());

	// Add the adjusted_sum as the last boundary
	cut_points.emplace_back(adjusted_sum);

	// Calculate the differences and add 1 to each to ensure positivity
	std::vector<int> numbers;

	int previous = 0;
	
	for (std::size_t i = 0; i < count; ++i) {
		numbers.emplace_back(cut_points[i] - previous + 1); // +1 ensures each is > 0
		previous = cut_points[i];
	}

	return numbers;
}


FleetManager::FleetManager() {}

void FleetManager::InitializeFleet(const std::size_t& numAircrafts) {
	if (!FleetManager::isInitialized && DataManager::getInstance(std::nullopt)->getNumberOfManufacturers() > 0) {

		// Initialize the FleetMgrInstance so that the vectors may be reserved to expect the aircrafts
		FleetMgrInstance = new FleetManager(numAircrafts);
		
		// Generate the random number of aircrafts that each manufacturer gets to deploy in the fleet
		std::vector<int> eachFleetSize = FleetMgrInstance->generateRandomCapacityForFleets(numAircrafts);

		// Get the list of names of all manufacturers
		std::vector<std::string> manufacturerList = DataManager::getInstance(std::nullopt)->getAllManufacturers();

		for (std::size_t i = 0; i < manufacturerList.size(); ++i) {
			DataManager* Instance = DataManager::getInstance(manufacturerList[i]);
			json aircraftConfigurationData = Instance->getInputdata(manufacturerList[i]);
			Fleet::constructAircrafts(eachFleetSize[i], aircraftConfigurationData);
		}
		if (FleetMgrInstance->Aircrafts.size() == numAircrafts) {
			for (std::shared_ptr<Fleet>& fleet : FleetMgrInstance->Aircrafts) {
				FleetMgrInstance->aircraftThreads.emplace_back([&fleet] {
					fleet->startSimulation(fleet);
					});
			}
		}
		
		FleetManager::isInitialized = true;
	}
    return;
}


FleetManager* FleetManager::getInstance() {
    if (!FleetManager::isInitialized) nullptr;
    return FleetMgrInstance;
}


void FleetManager::fillFleet(std::shared_ptr<Fleet>& aircraft) {
    Aircrafts.emplace_back(aircraft);
}


void FleetManager::retireSimulation() {
    /*
    * This function will collect all data at the end of the simulation
    * and call the logger to summarize all data points.
    */

	std::size_t joinedThreads = 0;
	bool check_Fleet = false;
	bool check_ChargingManager = false;

	while (!check_Fleet) check_Fleet = Fleet::completeSimulation();
    while (!check_ChargingManager) check_ChargingManager = ChargingManager::completeSimulation();

	while (joinedThreads == FleetMgrInstance->aircraftThreads.size()) {
		for (std::thread& _thread : FleetMgrInstance->aircraftThreads) {
			if (_thread.joinable()) {
				_thread.join();
				++joinedThreads;
			}
		}
	}

	return;
}


FleetManager::~FleetManager() {
    delete FleetMgrInstance;
    FleetMgrInstance = nullptr;
    FleetManager::isInitialized = false;

    FleetManager::aircraftThreads.clear();
}