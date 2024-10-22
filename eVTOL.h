#pragma once

#include <cmath>
#include <atomic>
#include <chrono>
#include <string>
#include <nlohmann/json.hpp>

#include "DataManager.h"
#include "ChargingManager.h"

using json = nlohmann::json;

class eVTOL
{
	// Flag to end simulation
	static std::atomic<bool> simulationCompleted;

	// Constant parameters that are pre-set by manufacturer
	const int CruiseSpeed;												// Maximum speed at which aircraft can cruise
	const int maxPassengerCount;										// Maximum passengers aircraft can transport
	const int BatteryCapacity;											// Net capacity of the battery 
	const double CruisingPowerConsumption;								// power used while cruising at cruise speed
	const double FaultsPerHour;											// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
	const std::chrono::duration<double, std::ratio<3600>> TimeToCharge;	// Time in hours required to charge the battery back to 100%

	// Internal metrics for operations
	bool isCharging;														// Flag to denote if the current aircraft is charging
	double PassengerMiles;													// Total passenger miles accumulated for this aircraft
	double MilesTravelled;													// Total number of miles travelled during each session
	int currentBatteryLevel;												// % charge remaining in the battery => 100% when object is created and resets to 100% after every charge
	double BatteryDischargeRate;											// Rate at which the battery drains per second
	std::chrono::duration<double> airTime;									// Total airtime in seconds for aircraft
	std::chrono::time_point<std::chrono::system_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
	std::chrono::time_point<std::chrono::system_clock> EndOperationTime;	// Timestamp of ending of flight in seconds


	void startAircraft();										// Starts the aircraft and records the starting time of flight
	void updateBatteryLevel();									// Keeps track of the rate of drain in battery and updates the remaining charge
	void calculateBatteryDrainRate();							// Calculates the rate of drain of battery per second
	void requestCharge(std::unique_ptr<eVTOL>& aircraft);		// Sends the aircraft to the Charging manager to get charged

	void retireSimulation(std::unique_ptr<eVTOL>& aircraft);	// Terminates the entire simulation

public:
	eVTOL(const json& InputData);

	void startSimulation(std::unique_ptr<eVTOL>& aircraft);

	double getPassengerMiles() const;
	double getMilesTravelled() const;
	std::chrono::seconds getTimeToCharge() const;
	std::chrono::duration<double> getAirTime() const;
	std::chrono::time_point<std::chrono::system_clock> getEndOperationTime() const;
	std::chrono::time_point<std::chrono::system_clock> getStartOperationTime() const;
	std::string getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const;

	virtual double getMilesPerSession() const = 0;
	virtual double getFaultsPerSession() const = 0;
	virtual double calculatePassengerMiles() const = 0;
	virtual std::string getManufacturerName() const = 0;

	~eVTOL() = default;
};