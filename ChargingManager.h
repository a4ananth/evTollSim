#pragma once
#include "eVTOL.h"
#include <thread>
#include <chrono>
#include <vector>
#include <queue>
#include <memory>


class ChargingManager
{
	struct ChargingStation {
		int stationID;
		bool IsOccupied;
		eVTOL* curUser;

		ChargingStation(int id) : stationID(id), IsOccupied(false), curUser(nullptr) {}

		void stopCharging(eVTOL* User);
		void startCharging(const std::chrono::seconds& durationInSeconds, eVTOL* User);
	};

	std::vector<ChargingStation*> ChargingStations;
	std::queue<eVTOL*> AircraftsInLIne;

public:
	ChargingManager(std::size_t numChargers);
	void chargeAircraft(eVTOL* aircraft);
	~ChargingManager();
};

