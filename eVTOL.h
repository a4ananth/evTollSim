#pragma once
#include <chrono>
#include <atomic>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class eVTOL
{
	// Constant parameters that are pre-set by manufacturer
	const int CruiseSpeed;												// Maximum speed at which aircraft can cruise
	const int maxPassengerCount;										// Maximum passengers aircraft can transport
	const int BatteryCapacity;											// Net capacity of the battery 
	const double CruisingPowerConsumption;								// power used while cruising at cruise speed
	const double FaultsPerHour;											// Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
	const std::chrono::duration<double, std::ratio<3600>> TimeToCharge;	// Time in hours required to charge the battery back to 100%

	// Metrics to track 
	int numFaults;								// No. of faults during current 
	double PassengerMiles;						// Based on formula => derived from calculatePassengerMiles()
	double mileagePerCharge;					// Total available miles when battery is at 100%
	std::atomic<double> currentBatteryLevel;	// % charge remaining in the battery => 100% when object is created and resets to 100% after every charge

	// Internal metrics for operations
	std::chrono::time_point<std::chrono::system_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
	std::chrono::time_point<std::chrono::system_clock> EndOperationTime;	// Timestamp of ending of flight in seconds
	std::chrono::duration<double> airTime;									// Total airtime in seconds for aircraft
	double BatteryDrainRate;

	enum class BATTERY_NOTIFICATIONS {
		POWER_AVAILABLE = 0,
		LOW_POWER = 1,
		CRITICAL_MINIMUM = 2
	};

	void retireSimulation();
	void resetCharge();
	void trackRemainingCharge();
	void updateBatteryLevel();
	void restartAircraft();
	void calculateBatteryDrainRate();
	

	std::chrono::seconds TimeToTravelOneMile() const;
	BATTERY_NOTIFICATIONS getCurrentBatteryLevel();

public:
	eVTOL(const json& InputData);

	void startSimulation();
	
	double getBatteryDrainRate() const;
	double calculatePassengerMiles(const std::size_t& factor) const;

	std::chrono::time_point<std::chrono::system_clock> getEndOperationTime() const;
	std::chrono::time_point<std::chrono::system_clock> getStartOperationTime() const;
	std::chrono::duration<double> getAirTime() const;
	std::chrono::seconds getTimeToCharge() const;

	std::string getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const;
	
	virtual std::string getManufacturerName(const eVTOL* aircraft) const = 0;
	virtual double getMilesPerSession() = 0;
	virtual double getFaultsPerSession() = 0;
	virtual double getPassengerMiles() = 0;

	~eVTOL() = default;
};

