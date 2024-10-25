#pragma once

#include "ChargingManager.h"


// Declaring static member variables for the class

ChargingManager* ChargingManager::ChargingMgrInstance = nullptr;
std::atomic<bool> ChargingManager::simulationComplete = false;
std::atomic<int> ChargingManager::chargingTicketNumber = 0;
bool ChargingManager::IsInitialized = false;


void ChargingManager::ChargingStation::stopCharging(std::shared_ptr<Fleet>& User, std::unique_ptr<ChargingStation>& station) {
	station->IsOccupied = false;
	User = std::move(station->curUser);
}


void ChargingManager::ChargingStation::startCharging(std::shared_ptr<Fleet>& User, std::unique_ptr<ChargingStation>& station) {
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
			Operations.emplace_back(&ChargingManager::ChargingStationManager, ChargingMgrInstance, std::ref(station));
		}

		Operations.emplace_back(&ChargingManager::MonitorChargingRequests, ChargingMgrInstance);
		
		IsInitialized = true;
	}
}


void ChargingManager::ChargingStationManager(std::unique_ptr<ChargingStation>& station) {
	while (station && !simulationComplete.load()) {
		std::unique_lock<std::mutex> lock(charger_mtx);

		// Wait until the station is unoccupied and a candidate is waiting in queue OR simulation completes
		ChargerManager.wait(lock, [&] {
			return (!AircraftsInLine.empty() && !station->IsOccupied) || simulationComplete.load();
			});

		if (simulationComplete.load()) break;

		if (!AircraftsInLine.empty()) {
			Request request = std::move(AircraftsInLine.front());
			std::shared_ptr<Fleet> candidate = std::move(request.candidate);
			station->chargingTicket = request.assignedTicketNumber;

			AircraftsInLine.pop();

			lock.unlock();

			station->startCharging(candidate, station);

			{
				std::lock_guard<std::mutex> requestLock(requests_mtx);
				completedRequests[station->chargingTicket] = std::move(candidate);
			}

			if (!station->IsOccupied) ChargingStatus.notify_one();

			// Notify other threads that a station is free
			ChargerManager.notify_all();
		}
	}

	if (station->IsOccupied) {
		station->IsOccupied = false;
		station->chargingTicket = 0;
		std::lock_guard<std::mutex> requestLock(requests_mtx);
		completedRequests[station->chargingTicket] = std::move(station->curUser);
		ChargingStatus.notify_one();
	}

	ChargerManager.notify_all();
	completeSimulation();
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


bool ChargingManager::completeSimulation() {
	if (!ChargingManager::simulationComplete.load()) {
		ChargingManager::simulationComplete.store(true);

		ChargingMgrInstance->ChargerManager.notify_all();
		ChargingMgrInstance->ChargingStatus.notify_all();
	}

	else {
		static int threadsJoined = 0;
		ChargingMgrInstance->clearList();
		ChargingMgrInstance->clearQueue();

		ChargingMgrInstance->ChargerManager.notify_all();
		ChargingMgrInstance->ChargingStatus.notify_all();

		for (std::thread& ChargingMgrThreads : ChargingMgrInstance->Operations) {
			if (ChargingMgrThreads.joinable()) {
				ChargingMgrThreads.join();
				++threadsJoined;
			}
		}

		return (threadsJoined == ChargingMgrInstance->Operations.size());
	}

	return false;
}


void ChargingManager::clearQueue() {
	std::unique_lock<std::mutex> queueLock(ChargingMgrInstance->charger_mtx);
	ChargingMgrInstance->ChargerManager.wait(queueLock, [&] {
		return simulationComplete.load() && !AircraftsInLine.empty();
		});

	while (!ChargingMgrInstance->AircraftsInLine.empty()) {
		Request request = std::move(ChargingMgrInstance->AircraftsInLine.front());
		std::shared_ptr<Fleet> candidate = std::move(request.candidate);
		int ticketNumber = request.assignedTicketNumber;

		ChargingMgrInstance->completedRequests[ticketNumber] = std::move(candidate);
		ChargingStatus.notify_one();

		ChargingMgrInstance->AircraftsInLine.pop();
	}	
}


void ChargingManager::clearList() {
	std::unique_lock<std::mutex> listLock(ChargingMgrInstance->requests_mtx);
	ChargingMgrInstance->ChargerManager.wait(listLock, [&] {
		return simulationComplete.load() && !requests.empty();
		});

	while (!ChargingMgrInstance->requests.empty()) {
		Request request = std::move(ChargingMgrInstance->requests.front());
		std::shared_ptr<Fleet> candidate = std::move(request.candidate);
		int ticketNumber = request.assignedTicketNumber;

		ChargingMgrInstance->completedRequests[ticketNumber] = std::move(candidate);
		ChargingStatus.notify_one();
	}
}


void ChargingManager::requestCharger(std::shared_ptr<Fleet> aircraft) {
	if (!simulationComplete.load()) {
		int ticketNumber = generateTicketNumber();
		if(ticketNumber > 0) {
			std::lock_guard<std::mutex> lock(requests_mtx);
			Request request(aircraft, ticketNumber);
			requests.emplace_back(request);
			completedRequests[ticketNumber] = nullptr;
		}

		std::unique_lock<std::mutex> lock(requests_mtx);
		ChargingStatus.wait(lock, [&] { return completedRequests[ticketNumber] != nullptr; });

		if (completedRequests[ticketNumber]) {
			aircraft = std::move(completedRequests[ticketNumber]);
			completedRequests.erase(ticketNumber);
		}
	}

	completeSimulation();
}


ChargingManager::~ChargingManager() {
	delete ChargingMgrInstance;
	ChargingMgrInstance = nullptr;

	ChargingManager::IsInitialized = false;
	ChargingManager::simulationComplete = false;
	ChargingManager::Operations.clear();
}