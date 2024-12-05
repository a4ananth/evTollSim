// SimpleSimulator.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "evTOL.h"
#include "DataLogger.h"
#include "FleetManager.h"
#include "RequestManager.h"
#include "ChargingStation.h"

#include <iostream>


/*
* This is the main function that initializes the simulation.
* The parameters that are passed to the functions are the number of chargers and the number of aircrafts,
* which can be defined below.
* 
* The input given to this assignment has been formatted into a json file that lies within the "Manufacturer.json" file.
* As of now, the location of the file is made local, but can be changed as per requirement.
* 
* The simulation is set to run for 1 minute, but this time can be scaled as per preference.
* The aircrafts are initialized on individual threads that log data during operation and charging.
* At the end of each airborne session, the aircrafts also log the performance summary.
* 
* All the relevant files can be found under the "Logs" and "Summary" folder.
*/


// Assign the number of chargers needed in the 
std::size_t numberOfChargers = 3;

// Assign the number of aircrafts needed in the fleet
std::size_t numberOfAircrafts = 20;

int main() {    

    ChargingStation::InitializeChargers(numberOfChargers);
	FleetManager::InitializeFleet(numberOfAircrafts);
	std::this_thread::sleep_for(std::chrono::minutes(10));
	FleetManager::stopSimulation();

    std::cout<< "Simulation for evTOLs has been stopped" << "\n";
    
	
    return 0;
}