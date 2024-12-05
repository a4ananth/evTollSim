#include <chrono>
#include <random>
#include <algorithm>

#include "DataLogger.h"
#include "ChargingStation.h"


std::mutex ChargingStation::chargerMtx;
std::once_flag ChargingStation::initialized;
std::atomic<bool> ChargingStation::simulationComplete{ false };
std::condition_variable ChargingStation::requestManagerNotification;
std::vector<std::unique_ptr<ChargingStation>> ChargingStation::chargerInstances = {};


template<typename ...Args>
inline std::unique_ptr<ChargingStation> ChargingStation::createInstance(Args && ...args)
{
	class make_unique_enabler : public ChargingStation {
	public:
		make_unique_enabler(Args &&... args) : ChargingStation(std::forward<Args>(args)...) {}
	};

	return std::make_unique<make_unique_enabler>(std::forward<Args>(args)...);
}


void ChargingStation::InitializeChargers(std::size_t numChargers) {
	ChargingStation::chargerInstances.reserve(numChargers);

	std::call_once(ChargingStation::initialized, [&numChargers] {
		for (std::size_t charger = 0; charger < numChargers; charger++) {
			ChargingStation::chargerInstances.emplace_back(ChargingStation::createInstance(charger));
		}
		});
}


void ChargingStation::stopSimulation() {
	ChargingStation::simulationComplete.store(true);

	ChargingStation::requestManagerNotification.notify_all();

	for (std::unique_ptr<ChargingStation>& charger : ChargingStation::chargerInstances) {
		if (charger->chargingThread.joinable()) charger->chargingThread.join();
	}
}


void ChargingStation::lookForRequests() { 
	while (!ChargingStation::simulationComplete.load()) {
		std::shared_ptr<RequestManager> request = nullptr;

		{
			std::unique_lock<std::mutex> lock(ChargingStation::chargerMtx);
			ChargingStation::requestManagerNotification.wait(lock, [&] {
				bool found = false;
				std::size_t newRequests = RequestManager::newRequestAvailable();
				if (newRequests > 0 && !isCharging.load()) found = true;

				return (found || ChargingStation::simulationComplete.load());

				});

			if (!isCharging.load() && (RequestManager::newRequestAvailable() > 0)) {
				request = RequestManager::fetchFirstInLIne();
				std::shared_ptr<DataLogger> logger = DataLogger::getInstance(request->getAircraft());
				logger->logData("Charger " + std::to_string(chargingStationID)
					+ " has received a request for ticket number: " + request->getTicketNumber());

				isCharging.store(true);
			}
		}

		if (ChargingStation::simulationComplete.load()) {
			if (request) request->getAircraft()->getAircraftCV().notify_all();
			break;
		}

		else if (isCharging.load() && request != nullptr) {
			std::shared_ptr<DataLogger> logger = DataLogger::getInstance(request->getAircraft());
			logger->logData("Charger " + std::to_string(chargingStationID) + " is now charging ticket number: " + request->getTicketNumber());

			std::chrono::microseconds chargingTime = request->getAircraft()->getTimeToCharge();
			logger->logData("Charging time for ticket number: " + request->getTicketNumber() + " is: " + std::to_string(chargingTime.count()) + " microseconds.");
			std::this_thread::sleep_for(chargingTime);

			request->updateEndTime();
			logger->logData("Time at charger has expired for ticket number: " + request->getTicketNumber());

			RequestManager::reportChargingStatus(request);
			logger->logData("Charging status for ticket number: " + request->getTicketNumber() + " has been reported.");

			isCharging.store(false);
			logger->logData("Charger " + std::to_string(chargingStationID) + " is now free.");
		}
	}
}


int ChargingStation::randomChargeTimeGenerator() {
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> dis(1, 20);
	int random_number = dis(gen);

	return random_number;
}


ChargingStation::ChargingStation(const std::size_t chargingStationID) : 
	chargingStationID(chargingStationID) 
{
	isCharging.store(false);
	chargingThread = std::thread(&ChargingStation::lookForRequests, this);
}