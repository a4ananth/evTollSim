#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <condition_variable>

#include "evTOL.h"
#include "RequestManager.h"


class RequestManager;
 
class ChargingStation {
public:
	static void InitializeChargers(std::size_t numChargers);			// Initialize the charging stations
	static void stopSimulation();										// Stop the simulation

	static std::condition_variable requestManagerNotification;			// Condition variable to notify the charging station of incoming requests

protected:
	// ChargingStation Class object control methods
	ChargingStation(ChargingStation&& other) = delete;						// Move constructor
	ChargingStation& operator= (ChargingStation&& other) = delete;			// Move assignment operator
	ChargingStation(const ChargingStation& other) = delete;					// Copy constructor
	ChargingStation& operator= (const ChargingStation& other) = delete;		// Copy assignment operator

	void lookForRequests();													// Look for incoming requests
	int randomChargeTimeGenerator();										// Generate random charging time

private:
	ChargingStation(const std::size_t chargingStationID);			// Parametrized constructor
	
	std::thread chargingThread;						// Thread object that would manage the charging process
	std::atomic<bool> isCharging;					// Flag to indicate if the charging station is in use
	std::size_t chargingStationID;					// Unique ID for each charging station	
	
	// Static data members
	static std::mutex chargerMtx;											// Mutex to lock the charging station
	static std::once_flag initialized;										// Flag to ensure that the charging station is initialized only once
	static std::atomic<bool> simulationComplete;							// Flag to indicate that the simulation is complete
	static std::vector<std::unique_ptr<ChargingStation>> chargerInstances;	// Vector of unique pointers to charging stations

	// Template function to create unique pointer instance of ChargingStation class
	template <typename... Args>
	static std::unique_ptr<ChargingStation> createInstance(Args && ...args);
};