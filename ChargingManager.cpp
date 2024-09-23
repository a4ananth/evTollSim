#include "ChargingManager.h"

void ChargingManager::ChargingStation::stopCharging(eVTOL* User) {
	ChargingStation* station = this;

	this->IsOccupied = false;
	this->curUser = nullptr;
	User->resetCharge();
}


void ChargingManager::ChargingStation::startCharging(const int& durationInSeconds, eVTOL* User) { 
	ChargingStation* station = this;

	this->IsOccupied = true;
	this->curUser = User;

	std::this_thread::sleep_for(std::chrono::seconds(durationInSeconds));

	stopCharging(User);
}


ChargingManager::ChargingManager(std::size_t numChargers) {
	ChargingStations.reserve(numChargers);
	for (std::size_t i = 0; i < numChargers; ++i) {
		ChargingStations.emplace_back(new ChargingStation(i + 1));
	}
}


void ChargingManager::chargeAircraft(eVTOL* aircraft) {
	/*
	* This function is a manager interface. It checks for the next available 
	* charging slot and allocates it to the first aircraft that requested
	* the spot. 
	* 
	* For the current aircraft that is requesting a spot, 
	* 
	*/
	for (ChargingStation* station : ChargingStations) {
		if (!station->IsOccupied) {
			if (this->AircraftsinLIne.empty()) {
				station->startCharging(aircraft->getTimeToCharge(), aircraft);
				return;
			}
			else {
				eVTOL* nextAircraft = this->AircraftsinLIne.front();
				station->startCharging(nextAircraft->getTimeToCharge(), nextAircraft);
				this->AircraftsinLIne.pop();

				this->AircraftsinLIne.push(aircraft);
			}
		}
	}
}

ChargingManager::~ChargingManager() {
	std::size_t station = 0;
	while (!ChargingStations.empty()) delete ChargingStations[station++];
}


