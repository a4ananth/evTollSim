#pragma once

#include <cmath>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <nlohmann/json.hpp>

//#include "DataManager.h"
//#include "FleetManager.h"
//#include "ChargingManager.h"

using json = nlohmann::json;

class Fleet
{
	// Flag to end simulation
	static std::atomic<bool> simulationCompleted;

	// Denotes the number of aircrafts that are spawned at the start of the simulation
	static std::size_t NumberOfAircraftsSpawned;

	// Mutex variable used for interface control to Charging Manager
	static std::mutex sendToCharger;

	// Constant parameters that are pre-set by manufacturer
	const std::string ManufacturerName;									// Name of the manufacturer
	const int CruiseSpeed;												// Maximum speed at which aircraft can cruise
	const int maxPassengerCount;										// Maximum passengers aircraft can transport
	const int BatteryCapacity;											// Net capacity of the battery 
	const double CruisingPowerConsumption;								// power used while cruising at cruise speed
	const double FaultsPerHour;											// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
	const std::chrono::duration<double, std::ratio<3600>> TimeToCharge;	// Time in hours required to charge the battery back to 100%

	// Internal metrics for operations
	bool isCharging;														// Flag to denote if the current aircraft is charging
	int FaultsPerSession;													// Number of faults generated during each simulation run
	double PassengerMiles;													// Total passenger miles accumulated for this aircraft
	double MilesTravelled;													// Total number of miles travelled during each session
	int currentBatteryLevel;												// % charge remaining in the battery => 100% when object is created and resets to 100% after every charge
	double BatteryDischargeRate;											// Rate at which the battery drains per second

	std::chrono::duration<double> airTime;									// Total airtime in seconds for aircraft
	std::chrono::time_point<std::chrono::system_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
	std::chrono::time_point<std::chrono::system_clock> EndOperationTime;	// Timestamp of ending of flight in seconds
	
	// Private construct defined to let the FleetManager manage object creation
	
	
	void startAircraft();										// Starts the aircraft and records the starting time of flight
	void updateBatteryLevel();									// Keeps track of the rate of drain in battery and updates the remaining charge
	void calculateBatteryDrainRate();							// Calculates the rate of drain of battery per second
	void requestCharge(std::shared_ptr<Fleet>& aircraft);		// Sends the aircraft to the Charging manager to get charged

public:
	Fleet();
	Fleet(const json& InputData);
	static void constructAircrafts(const int& numAircrafts, const json& InputData);

	static bool completeSimulation();							// Marks the flag to trigger the end of simulation
	
	void startSimulation(std::shared_ptr<Fleet>& aircraft);		// Starts the simulation for each aircraft	

	double getPassengerMiles() const;
	std::chrono::seconds getTimeToCharge() const;
	std::chrono::time_point<std::chrono::system_clock> getEndOperationTime() const;
	std::chrono::time_point<std::chrono::system_clock> getStartOperationTime() const;
	std::string getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const;

	double getMilesPerSession() const;
	double getFaultsPerSession() const;
	double calculatePassengerMiles() const;
	std::string getManufacturerName() const;

	~Fleet() = default;
};