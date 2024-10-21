#pragma once
#include <chrono>
#include <atomic>
#include <string>
#include <nlohmann/json.hpp>

#include "ChargingManager.h"

using json = nlohmann::json;

class eVTOL
{
	// Flag to end simulation
	static bool simulationCompleted;

	// Constant parameters that are pre-set by manufacturer
	const int CruiseSpeed;												// Maximum speed at which aircraft can cruise
	const int maxPassengerCount;										// Maximum passengers aircraft can transport
	const int BatteryCapacity;											// Net capacity of the battery 
	const double CruisingPowerConsumption;								// power used while cruising at cruise speed
	const double FaultsPerHour;											// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
	const std::chrono::duration<double, std::ratio<3600>> TimeToCharge;	// Time in hours required to charge the battery back to 100%

	// Internal metrics for operations
	double BatteryDischargeRate;											// Rate at which the battery drains per second
	std::atomic<int> currentBatteryLevel;									// % charge remaining in the battery => 100% when object is created and resets to 100% after every charge
	std::chrono::time_point<std::chrono::system_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
	std::chrono::time_point<std::chrono::system_clock> EndOperationTime;	// Timestamp of ending of flight in seconds
	std::chrono::duration<double> airTime;									// Total airtime in seconds for aircraft


	void startAircraft();									// Starts the aircraft and records the starting time of flight
	void updateBatteryLevel();								// Keeps track of the rate of drain in battery and updates the remaining charge
	void calculateBatteryDrainRate();						// Calculates the rate of drain of battery per second
	void requestCharge(std::unique_ptr<eVTOL>& aircraft);	// Sends the aircraft to the Charging manager to get charged

	void retireSimulation();								// Terminates the entire simulation

public:
	eVTOL(const json& InputData);

	void startSimulation(std::unique_ptr<eVTOL>& aircraft);

	std::chrono::time_point<std::chrono::system_clock> getEndOperationTime() const;
	std::chrono::time_point<std::chrono::system_clock> getStartOperationTime() const;
	std::chrono::duration<double> getAirTime() const;
	std::chrono::seconds getTimeToCharge() const;

	std::string getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const;

	virtual double calculatePassengerMiles(const std::size_t& factor) const = 0;
	virtual std::string getManufacturerName(const eVTOL* aircraft) const = 0;
	virtual double getMilesPerSession() = 0;
	virtual double getFaultsPerSession() = 0;
	virtual double getPassengerMiles() = 0;

	~eVTOL() = default;
};