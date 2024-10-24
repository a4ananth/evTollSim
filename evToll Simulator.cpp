// evToll Simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Fleet.h"
#include "DataManager.h"
#include "FleetManager.h"
#include "ChargingManager.h"


using json = nlohmann::json;


int main()
{
	DataManager::InitializeDataManager();
	/*ChargingManager::InitializeChargers(3);
	FleetManager::InitializeFleet(20);*/

	return 0;
}
