#pragma once
#include <chrono>
#include <atomic>


class eVTOL
{
	// Constant parameters that are pre-set by manufacturer
	int CruiseSpeed;												// Maximum speed at which aircraft can cruise
	int maxPassengerCount;											// Maximum passengers airccraft can transport
	int BatteryCapacity;											// Net capacity of the 
	double CruisingPowerConsumtion;									// power used while cruising at cruise speed
	double FaultsPerHour;											// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
	std::chrono::duration<double, std::ratio<3600>> TimeToCharge;	// Time in hours required to charge the battery back to 100%

	// Metrics to track 
	int numFaults;								// No. of faults during current 
	double PassengerMiles;						// Based on formula => derived from calculatePassengerMiles()
	double mileagePerCharge;					// Total available miles when battery is at 100%
	std::atomic<double> currentBatteryLevel;	// % charge remaining in the battery => 100% when object is created and resets to 100% after every charge

	// Internal metrics for operations
	std::chrono::time_point<std::chrono::high_resolution_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
	std::chrono::time_point<std::chrono::high_resolution_clock> EndOperationTime;	// Timestamp of ending of flight in seconds
	std::chrono::duration<double> airTime;											// Total airtime in seconds for aircraft

	enum class BATTERY_NOTIFICATIONS {
		POWER_AVAILABLE = 0,
		LOW_POWER = 1,
		CRITICAL_MINIMUM = 2
	};

	double BatteryDrainRate();
	void updateBatteryLevel();
	void restartAircraft();

public:

	//eVTOL() = default;

	eVTOL(int CruiseSpeed, int maxPassengerCount, int BatteryCapacity, double CruisingPowerConsumtion, double FaultsPerHour, double ChargeDuration);

	void startSimulation();
	void resetCharge();
	BATTERY_NOTIFICATIONS trackRemainingCharge();

	std::chrono::seconds getTimeToCharge();
	BATTERY_NOTIFICATIONS getCurrentBatteryLevel();

	double calculatePassengerMiles(int& factor);

	~eVTOL() = default;
};

