#pragma once

#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <condition_variable>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


class evTOL : public std::enable_shared_from_this<evTOL> {
private:
	// Static data members
    static std::atomic<bool> simulationComplete;		        	// Flag to indicate that the simulation is complete
    static std::condition_variable aircraftCV;                      // Condition variable to notify the aircraft

    // Constant parameters that are pre-set by manufacturer
    std::string ManufacturerName;                                    // Name of the manufacturer
    int CruiseSpeed;                                                 // Maximum speed at which aircraft can cruise
    int maxPassengerCount;                                           // Maximum passengers aircraft can transport
    int BatteryCapacity;                                             // Net capacity of the battery 
    double CruisingPowerConsumption;                                 // power used while cruising at cruise speed
    double FaultsPerHour;                                            // Probability of faults = > % increase in the consumption of battery kWh value per hour => reduced airtime
    std::chrono::duration<double, std::ratio<3600>> TimeToCharge;    // Time in hours required to charge the battery back to 100%

    // Metrics and flags for craft operations
    int currentBatteryLevel;                                                // Indicator for the curent battery level. Starts at 100%
    std::atomic<bool> chargingStatus;                                       // Flag for the current charging status of the aircraft
    std::chrono::duration<double> airTime;									// Total airtime in seconds for aircraft
    std::chrono::time_point<std::chrono::system_clock> StartOperationTime;	// Timestamp of beginning of flight in seconds
    std::chrono::time_point<std::chrono::system_clock> EndOperationTime;	// Timestamp of ending of flight in seconds
	
	std::mutex aircraftMtx;                                                 // Mutex to lock the aircraft
	std::thread chargerThread;                                              // Thread object that would manage the receiving of aircraft from the charger

protected:
    // Internal functionalities that all aircrafts can and must perform
    void startAircraft();										    // Starts the aircraft and records the starting time of flight
    void updateBatteryLevel();									    // Keeps track of the rate of drain in battery and updates the remaining charge
    void receiveFromCharger(const std::string& ticketNumber);       // Receives the aircraft from the charging stations
    std::string requestCharge(std::shared_ptr<evTOL>& aircraft);	// Sends the aircraft to the Charging manager to get charged

public:
    /* ----------------- Constructors ----------------- */
	evTOL() = default;                                      // Default constructor
    evTOL(const json& InputData);                           // Parametrized constructor 
	evTOL(evTOL&& other) noexcept = default;                // Move constructor
    evTOL& operator=(evTOL&& other) noexcept = default;     // Move Assignment
    

    /* ----------------- Assignment Operators ----------------- */
    evTOL(const evTOL& other) = delete;                              // Copy constructor 
    evTOL& operator=(const evTOL& other) = delete;                   // Copy Assignment
    

    /* --------------- All public APIs ---------------- */
    void startSimulation();		                            // Starts the simulation for each aircraft	
    static void retireSimulation();					        // Marks the flag to trigger the end of simulation

    int getCruiseSpeed() const;                             // Get the cruise speed for the aircraft
    int getMaxPassengerCount() const;                       // Get the maximum passenger count for the aircraft
	std::string get_manufacturer() const;				    // Get the manufacturer name for the aircraft
    std::condition_variable& getAircraftCV();			    // Get the condition variable for the aircraft
    std::chrono::microseconds getTimeToCharge() const;		// Get the time required to charge the aircraft
	
    std::chrono::duration<double> getAirTime() const;
    std::chrono::time_point<std::chrono::system_clock> getEndOperationTime() const;
    std::chrono::time_point<std::chrono::system_clock> getStartOperationTime() const;
    std::string getTimeForLogs(const std::chrono::time_point<std::chrono::system_clock>& timePoint) const;

    virtual double getPassengerMiles() const = 0;
    virtual double getMilesPerSession() const = 0;
    virtual double getFaultsPerSession() const = 0;
    virtual std::string getManufacturerName() const = 0;

	~evTOL() = default;                                     // Destructor   
};