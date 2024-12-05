#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <memory>
#include <thread>
#include <unordered_map>
#include <condition_variable>

#include "evTOL.h"


class RequestManager {
public:
	// RequestManager public APIs
	void updateEndTime();							// Update end time of charging event
	bool thankyou() const;							// Check if charging process is completed	
	void updateStartTime();							// Update start time of charging event

	std::string getTicketNumber() const;			// Get ticket number of charging request
	std::shared_ptr<evTOL> getAircraft() const;		// Get aircraft associated with charging request

	// Static member functions
	static void stopSimulation();															// Stop the simulation
	static std::size_t newRequestAvailable();												// Check if new request is available
	static std::shared_ptr<RequestManager> fetchFirstInLIne();								// Fetch the first request in the queue
	static void reportChargingStatus(std::shared_ptr<RequestManager>& thisRequest);			// Report the status of charging
	static std::string createChargingRequest(const std::shared_ptr<evTOL>& aircraft);		// Create a new charging request
	static std::shared_ptr<RequestManager> getRequest(const std::string& ticketNumber);		// Get the request object for charging
	

protected:
	// RequestManager class internal operations
	void addToStatusMonitor() const;
	std::string generateTicketNumber() const;
	void monitorChargingRequest(const std::string& ticketNumber) const;
	void markChargingProcessCompleted(const std::string& ticketNumber) const;
	void addToRequestQueue(const std::shared_ptr<RequestManager>& thisRequest) const;
	
	static std::string createNewRequest(const std::shared_ptr<evTOL>& aircraft);

private:
	// RequestManager Class initialization
	RequestManager(const std::shared_ptr<evTOL>& aircraft);			// Parametrized constructor
	
	// RequestManager Class object control methods
	RequestManager(RequestManager&& other) noexcept = default;				// Move constructor
	RequestManager& operator= (RequestManager&& other) noexcept = default;	// Move assignment operator

	// RequestManager Class object prohibit methods
	RequestManager(const RequestManager& other) = delete;					// Copy constructor
	RequestManager& operator= (const RequestManager& other) = delete;		// Copy assignment operator

	// Class object data members
	static std::atomic<bool> simulationComplete; 					// Flag to indicate that the simulation is complete

	std::string ticketNumber;										// Ticket number assigned to each charging request
	std::atomic<bool> status;										// Completion status of the ticket
	std::thread statusThread;										// Thread object that would manage the update from chargers
	std::shared_ptr<evTOL> aircraft;								// Aircraft that is raising the request to be charged
	
	std::chrono::time_point<std::chrono::system_clock> endTime;		// Timestamp of completion of charging event
	std::chrono::time_point<std::chrono::system_clock> startTime;	// Timestamp of beginning of charging event

	// Static data members
	static std::mutex requestsMtx;											// Mutex to control access to queue for incoming requests
	static std::queue<std::shared_ptr<RequestManager>> incomingRequests;	// Queue to store incoming requests

	static std::mutex updatesMtx;													// Mutex to control access to map for status updates
	static std::unordered_map<std::string, std::atomic<bool>> processedRequests;	// Map to indicate the status of charging

	static std::mutex instancesMtx;														// Mutex to control access to map for status updates
	static std::unordered_map<std::string, std::shared_ptr<RequestManager>> instances;	// Map to record all instances created for charging request

	static std::condition_variable chargingComplete;		// Condition variable to receive notification from chargers

	// Template function to create shared pointer instance of RequestManager class
	template <typename... Args>
	static std::shared_ptr<RequestManager> createInstance(Args &&... args);
};