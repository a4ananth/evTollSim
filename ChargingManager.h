#pragma once

#include <list>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>
#include <vector>
#include <memory>
#include <thread>
#include <unordered_map>
#include <condition_variable>

#include "Fleet.h"


class ChargingManager
{
	// Charging manager handles all charging stations. Hence its Singleton
	static ChargingManager* ChargingMgrInstance;

	// Flag to end simulation
	static std::atomic<bool> simulationComplete;

	// Flag to denote if system has already initialized. This will reset upon destruction
	static bool IsInitialized;

	// Ticket number that will be assigned to each incoming evTOL object
	static std::atomic<int> chargingTicketNumber;

	ChargingManager();

	ChargingManager(const std::size_t& numChargers);

	struct ChargingStation {
		std::size_t stationID;
		bool IsOccupied;
		int chargingTicket;
		std::shared_ptr<Fleet> curUser;

		ChargingStation(const std::size_t& id) : stationID(id), IsOccupied(false), chargingTicket(0), curUser(nullptr) {}

		void stopCharging(std::shared_ptr<Fleet>& User, std::unique_ptr<ChargingStation>& station);
		void startCharging(std::shared_ptr<Fleet>& User, std::unique_ptr<ChargingStation>& station);
	};

	struct Request {
		std::shared_ptr<Fleet> candidate;
		int assignedTicketNumber;

		Request(std::shared_ptr<Fleet> requestor, int& ticketNumber) : candidate(std::move(requestor)), assignedTicketNumber(ticketNumber) {}
	};

	std::mutex charger_mtx;
	std::mutex requests_mtx;

	std::condition_variable ChargingStatus;
	std::condition_variable ChargerManager;

	// vector of threads deployed for all charging related operations
	std::vector<std::thread> Operations;

	std::list<Request> requests;
	std::queue<Request> AircraftsInLine;
	std::unordered_map<int, std::shared_ptr<Fleet>> completedRequests;

	// Thread function that would be invoked for each charging station
	void ChargingStationManager(std::unique_ptr<ChargingStation>& station);

	// Thread function to continuously accept aircraft charging requests
	void MonitorChargingRequests();

	// This function generates a ticket for each charging request
	int generateTicketNumber();

public:
	static void InitializeChargers(const std::size_t& numChargers);
	static ChargingManager* getInstance();

	// This function calls the simulation to complete for the Charging manager
	static bool completeSimulation();
	
	// This function clears the queue at the end of simulation
	void clearQueue();

	// This function clears the list at the end of simulation
	void clearList();

	void requestCharger(std::shared_ptr<Fleet> aircraft);

	~ChargingManager();
};