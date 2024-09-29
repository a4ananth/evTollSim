#include "ChargingManager.h"
#include "eVTOL.h"

void ChargingManager::ChargingStation::stopCharging(eVTOL* User) {
	ChargingStation* station = this;

	this->IsOccupied = false;
	this->curUser = nullptr;
	User->resetCharge();
}


void ChargingManager::ChargingStation::startCharging(const std::chrono::seconds& durationInSeconds, eVTOL* User) {
	ChargingStation* station = this;

	this->IsOccupied = true;
	this->curUser = User;

	std::this_thread::sleep_for(durationInSeconds);

	stopCharging(User);
}


ChargingManager::ChargingManager(std::size_t numChargers) {
	ChargingStations.reserve(numChargers);
	for (std::size_t i = 0; i < numChargers; ++i) ChargingStations.emplace_back(new ChargingStation(static_cast<int>(i + 1)));
}


void ChargingManager::chargeAircraft(eVTOL* aircraft) {
	/*
	* This function is a manager interface. It checks for the next available 
	* charging slot and allocates it to the first aircraft that requested
	* the spot. 
	* 
	* For the current aircraft that is requesting a spot, if no spots are available
	* then the aircraft is pushed to a queue which is maintained to send the next 
	* aircraft in line to charge.
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
				this->AircraftsinLIne.pop();				// Remove the aircraft from the front of the line

				this->AircraftsinLIne.push(aircraft);		// Add the current aircraft to the queue
			}
		}
	}
}


ChargingManager::~ChargingManager() {
	std::size_t station = 0;
	while (station < ChargingStations.size()) delete ChargingStations[station++];
	ChargingStations.clear();
}