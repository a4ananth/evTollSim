#pragma once

#include "ChargingManager.h"


// Declaring static member variables for the class

ChargingManager* ChargingManager::ChargingMgrInstance = nullptr;
std::atomic<bool> ChargingManager::simulationComplete = false;
std::atomic<int> ChargingManager::chargingTicketNumber = 0;
bool ChargingManager::IsInitialized = false;


void ChargingManager::ChargingStation::stopCharging(std::unique_ptr<eVTOL>& User, std::unique_ptr<ChargingStation>& station) {
	station->IsOccupied = false;
	User = std::move(station->curUser);
}


void ChargingManager::ChargingStation::startCharging(std::unique_ptr<eVTOL>& User, std::unique_ptr<ChargingStation>& station) {
	station->IsOccupied = true;
	station->curUser = std::move(User);

	std::this_thread::sleep_for(std::chrono::seconds(station->curUser->getTimeToCharge()));

	stopCharging(User, station);
}


inline ChargingManager::ChargingManager(const std::size_t& numChargers) {
	if (!IsInitialized) {
		Operations.reserve(numChargers + 1);
		for (std::size_t i = 0; i < numChargers; ++i) {
			std::unique_ptr<ChargingStation> station = std::make_unique<ChargingStation>(i + 1);
			Operations.emplace_back(&ChargingManager::ChargingStationManager, this, std::move(station));
		}

		Operations.emplace_back(&ChargingManager::MonitorChargingRequests, this);
		IsInitialized = true;
	}
}


void ChargingManager::ChargingStationManager(std::unique_ptr<ChargingStation>& station) {
	while (!simulationComplete.load()) {
		std::unique_lock<std::mutex> lock(charger_mtx);

		// Wait until the station is unoccupied and a candidate is waiting in queue OR simulation completes
		ChargerManager.wait(lock, [&] {
			return (!AircraftsInLine.empty() && !station->IsOccupied);
			});


		if (!AircraftsInLine.empty()) {
			Request request = std::move(AircraftsInLine.front());
			std::unique_ptr<eVTOL> candidate = std::move(request.candidate);
			int ticketNumber = request.assignedTicketNumber;

			AircraftsInLine.pop();

			lock.unlock();

			station->startCharging(candidate, station);

			{
				std::lock_guard<std::mutex> requestLock(requests_mtx);
				completedRequests[ticketNumber] = std::move(candidate);
			}

			if (!station->IsOccupied) ChargingStatus.notify_one();

			// Notify other threads that a station is free
			ChargerManager.notify_all();
		}
	}

	{
		std::unique_lock<std::mutex> listLock(requests_mtx);
		if (station->IsOccupied) {
			std::unique_ptr<eVTOL> candidate = std::move(station->curUser);
			DataManager* logger = DataManager::getInstance(candidate->getManufacturerName());
			logger->createLogData(candidate);
		}
	}

	ChargerManager.notify_all();
	completeSimulation();
}


void ChargingManager::clearQueue() {
	DataManager* logger = nullptr;
	std::unique_lock<std::mutex> queueLock(requests_mtx);
	while (!AircraftsInLine.empty()) {
		Request request = std::move(AircraftsInLine.front());
		std::unique_ptr<eVTOL> candidate = std::move(request.candidate);
		int ticketNumber = request.assignedTicketNumber;

		completedRequests.erase(ticketNumber);

		logger = DataManager::getInstance(candidate->getManufacturerName());
		logger->createLogData(candidate);

		AircraftsInLine.pop();
	}
}


void ChargingManager::clearList() {
	DataManager* logger = nullptr;
	std::unique_lock<std::mutex> listLock(requests_mtx);
	while (!requests.empty()) {
		Request request = std::move(AircraftsInLine.front());
		std::unique_ptr<eVTOL> candidate = std::move(request.candidate);
		int ticketNumber = request.assignedTicketNumber;

		completedRequests.erase(ticketNumber);

		logger = DataManager::getInstance(candidate->getManufacturerName());
		logger->createLogData(candidate);
	}
}


void ChargingManager::MonitorChargingRequests() {
	while (!simulationComplete.load()) {

		if (!requests.empty()) {
			std::unique_lock<std::mutex> listLock(requests_mtx);
			while (!requests.empty()) {
				std::unique_lock<std::mutex> chargerLock(charger_mtx);
				AircraftsInLine.push(requests.front());
				requests.pop_front();
			}

			ChargerManager.notify_all();
		}
	}

	ChargerManager.notify_all();
	completeSimulation();
}


int ChargingManager::generateTicketNumber() {
	if (!simulationComplete.load()) {
		std::lock_guard<std::mutex> lock(requests_mtx);
		++ChargingManager::chargingTicketNumber;

		return ChargingManager::chargingTicketNumber.load();
	}

	return -1;
}


void ChargingManager::InitializeChargers(const std::size_t& numChargers) {
	if (!ChargingManager::IsInitialized) ChargingManager::ChargingMgrInstance = new ChargingManager(numChargers);
	return;
}


ChargingManager* ChargingManager::getInstance() {
	if (!ChargingManager::IsInitialized) return nullptr;
	return ChargingManager::ChargingMgrInstance;
}


void ChargingManager::completeSimulation() {
	if (!ChargingManager::simulationComplete.load()) ChargingManager::simulationComplete = true;

	else {
		ChargingMgrInstance->clearQueue();
		ChargingMgrInstance->clearList();

		ChargingMgrInstance->ChargerManager.notify_all();
		ChargingMgrInstance->ChargingStatus.notify_all();

		for (std::thread& StationThreads : ChargingMgrInstance->Operations) {
			if (StationThreads.joinable()) StationThreads.join();
		}
	}
}


void ChargingManager::requestCharger(std::unique_ptr<eVTOL>& aircraft) {
	if (!simulationComplete.load()) {
		int ticketNumber = generateTicketNumber();
		{
			std::lock_guard<std::mutex> lock(requests_mtx);
			Request request(aircraft, ticketNumber);
			requests.emplace_back(request);
			completedRequests[ticketNumber] = nullptr;
		}

		{
			std::unique_lock<std::mutex> lock(requests_mtx);
			ChargingStatus.wait(lock, [&] { return completedRequests[ticketNumber] != nullptr; });
		}

		if (completedRequests[ticketNumber]) aircraft = std::move(completedRequests[ticketNumber]);
	}

	completeSimulation();
}


ChargingManager::~ChargingManager() {
	delete ChargingMgrInstance;
	ChargingMgrInstance = nullptr;

	ChargingManager::IsInitialized = false;
	ChargingManager::simulationComplete = false;
}