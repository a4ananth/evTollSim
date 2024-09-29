#include "DataParser.h"
#include <fstream>


json DataParser::getInputdata() {
	json InputData{};
	std::ifstream InputDataFile("Manufacturer.json");

	if (!InputDataFile.is_open()) throw std::runtime_error("Unable to open input data json");
	
	InputDataFile >> InputData;

	return InputData;
}


void DataParser::createLogData(const eVTOL* aircraft) {
	json AircraftLog{};
	std::string fileName = aircraft->getManufacturerName() + ".json";
	std::ofstream AircraftLogFile(fileName);

	if (AircraftLogFile.is_open()) {
		json SessionData{};
		SessionData["Start_Time"] = aircraft->getStartTime();
		SessionData["End_Time"] = aircraft->getEndTime();
		SessionData["Miles_Travelled"] = aircraft->MilesPerSession();
		SessionData["Faults"] = aircraft->FaultsPerSession();
		SessionData["Passenger_Miles"] = aircraft->getPassengerMiles();

		AircraftLog["Sessions"].emplace_back(SessionData);

		SaveLog(AircraftLog, fileName);
	}

	else AircraftLog[aircraft->getManufacturerName()]["Sessions"] = json::array();
}


void DataParser::SaveLog(const json& LogData, const std::string& LogFileName) {
	std::ofstream file(LogFileName);
	
	if (!file.is_open()) throw std::runtime_error("Unable to open log file");

	file << LogData.dump(4);
}
