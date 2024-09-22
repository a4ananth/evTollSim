#pragma once
#include "eVTOL.h"
#include <thread>
#include <chrono>
#include <vector>


class ChargingManager
{
	struct ChargingStation {
		int stationID;
		bool IsOccupied;
		eVTOL* curUser;

		ChargingStation(int id) : stationID(id), IsOccupied(false) {}

		void stopCharging();
		void startCharging(const int& durationInSeconds, eVTOL* User);
	};

	std::vector<ChargingStation*> ChargingStations;

public:
	ChargingManager(int numChargers);
	void chargeAircraft(eVTOL* aircraft)
};

