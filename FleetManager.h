#pragma once

#include <thread>
#include <vector>
#include <memory>

#include "Fleet.h"
#include "ChargingManager.h"


class FleetManager {
    static FleetManager* FleetMgrInstance;
    static bool isInitialized;

	std::vector<std::shared_ptr<Fleet>> Aircrafts;
    std::vector<std::thread> aircraftThreads;

    
    FleetManager(const std::size_t& numAircrafts);
    std::vector<int> generateRandomCapacityForFleets(const std::size_t& totalAircrafts);

public:
    FleetManager();
    static void InitializeFleet(const std::size_t& numAircrafts);
    static FleetManager* getInstance();
    void fillFleet(std::shared_ptr<Fleet>& aircraft);
	
	static void retireSimulation();	// Terminates the entire simulation

    ~FleetManager();
};



