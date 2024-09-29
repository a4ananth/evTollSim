#pragma once
#include <chrono>
#include <atomic>


class eVTOL
{
	// Constant parameters that are pre-set by manufacturer
	const int CruiseSpeed;												// Maximum speed at which aircraft can cruise
	const int maxPassengerCount;										// Maximum passengers aircraft can transport
	const int BatteryCapacity;											// Net capacity of the battery 
	const double CruisingPowerConsumtion;								// power used while cruising at cruise speed
	const double FaultsPerHour;											// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
	const std::chrono::duration<double, std::ratio<3600>> TimeToCharge;	// Time in hours required to charge the battery back to 100%

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

	double BatteryDrainRate() const;
	void updateBatteryLevel();
	void restartAircraft();
	std::chrono::seconds TimeToTravelOneMile() const;


public:
	eVTOL(const int CruiseSpeed, const int maxPassengerCount, const int BatteryCapacity, const double CruisingPowerConsumtion, const double FaultsPerHour, const double ChargeDuration);

	void startSimulation();
	void resetCharge();
	void trackRemainingCharge();

	std::chrono::seconds getTimeToCharge() const;
	BATTERY_NOTIFICATIONS getCurrentBatteryLevel();

	double calculatePassengerMiles(int& factor);

	~eVTOL() = default;
};

