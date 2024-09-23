#pragma once
#include <chrono>


class eVTOL
{
	// Constant parameters that are pre-set by manufacturer
	int CruiseSpeed;						// Maximum speed at which aircraft can cruise
	int maxPassengerCount;					// Maximum passengers airccraft can transport
	int BatteryCapacity;					// Net capacity of the battery
	double TimeToCharge;					// Time in hours required to charge the battery back to 100%
	double CruisingPowerConsumtion;			// power used while cruising at cruise speed
	double FaultsPerHour;					// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime

	// Metrics to track 
	int numFaults;							// No. of faults during current 
	double PassengerMiles;					// Based on formula => derived from calculatePassengerMiles()
	double mileagePerCharge;				// Total available miles when battery is at 100%
	double airtimePerCharge;				// Total air time per 
	double currentBatteryLevel;				// % charge remaining in the battery => 100% when object is created and resets to 100% after every charge

	// Internal metrics for operations
	std::chrono::time_point<std::chrono::high_resolution_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
	std::chrono::time_point<std::chrono::high_resolution_clock> EndOperationTime;	// Timestamp of ending of flight in seconds

	enum class BATTERY_NOTIFICATIONS {
		POWER_AVAILABLE = 0,
		LOW_POWER = 1,
		CRITICAL_MINIMUM = 2
	};

public:
	eVTOL() = default;

	void resetCharge();
	BATTERY_NOTIFICATIONS trackRemainingCharge();
	virtual int getTimeToCharge() = 0;
	
	double calculatePassengerMiles(int& factor);

	virtual ~eVTOL() = 0;
};

