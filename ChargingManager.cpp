#include "ChargingManager.h"

void ChargingManager::ChargingStation::stopCharging() {
	ChargingStation* station = this;

	this->IsOccupied = false;
	this->curUser = nullptr;
}


void ChargingManager::ChargingStation::startCharging(const int& durationInSeconds, eVTOL* User) { 
	ChargingStation* station = this;

	this->IsOccupied = true;
	this->curUser = User;

	std::this_thread::sleep_for(std::chrono::seconds(durationInSeconds));

	stopCharging();
}


ChargingManager::ChargingManager(int numChargers) {
	for (int i = 0; i < numChargers; ++i) {
		ChargingStations.emplace_back(new ChargingStation(i + 1));
	}
}


void ChargingManager::chargeAircraft(eVTOL* aircraft) {
	for (ChargingStation* station : ChargingStations) {
		if (!station->IsOccupied) {
			station->startCharging(aircraft->getTimeToCharge(), aircraft);
			return;
		}
	}
}
