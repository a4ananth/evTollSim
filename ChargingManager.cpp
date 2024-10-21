#include "ChargingManager.h"
#include "eVTOL.h"


ChargingManager* ChargingManager::ChargingMgrInstance = nullptr;
bool ChargingManager::simulationComplete = false;
int ChargingManager::chargingTicketNumber = 0;
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
	while (true) {
		std::unique_lock<std::mutex> lock(charger_mtx);
		ChargerManager.wait(lock, [&] {
			return !AircraftsInLine.empty() || simulationComplete;
			});

		// Exit the loop if all candidates have been processed & simulation is complete
		if (simulationComplete && AircraftsInLine.empty()) break;

		if (!AircraftsInLine.empty()) {
			Request request = std::move(AircraftsInLine.front());
			std::unique_ptr<eVTOL> candidate = std::move(request.candidate);
			int ticketNumber = request.assignedTicketNumber;
			bool chargingComplete = false;

			AircraftsInLine.pop();

			lock.unlock();

			station->startCharging(candidate, station);

			{
				std::lock_guard<std::mutex> requestLock(requests_mtx);
				if (completedRequests.find(ticketNumber) != completedRequests.end()) completedRequests[ticketNumber] = std::move(candidate);
				if (!station->IsOccupied) ChargingStatus.notify_one();
			}

			// Notify other threads that a station is free
			ChargerManager.notify_all();
		}
	}
}


void ChargingManager::MonitorChargingRequests() {
	while (!simulationComplete) {

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
}

int ChargingManager::generateTicketNumber() {
	std::lock_guard<std::mutex> lock(requests_mtx);
	++chargingTicketNumber;

	return chargingTicketNumber;
}


void ChargingManager::InitializeChargers(const std::size_t& numChargers) {
	if (!IsInitialized) ChargingMgrInstance = new ChargingManager(numChargers);
	return;
}


ChargingManager* ChargingManager::getInstance() {
	if (!IsInitialized) return nullptr;
	return ChargingMgrInstance;
}


void ChargingManager::requestCharger(std::unique_ptr<eVTOL>& aircraft) {
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


ChargingManager::~ChargingManager() {
	ChargerManager.notify_all();

	for (std::thread& StationThreads : Operations) {
		if (StationThreads.joinable()) StationThreads.join();
	}

	delete ChargingMgrInstance;
	ChargingMgrInstance = nullptr;

	IsInitialized = false;
}