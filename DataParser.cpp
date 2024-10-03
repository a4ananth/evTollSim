#include "DataParser.h"
#include <fstream>
#include <iostream>


DataParser::DataParser() {
	std::ifstream InputDataFile("Manufacturer.json");
	if (!InputDataFile.is_open()) throw std::runtime_error("Unable to open input data json");

	InputDataFile >> InputData;

	for (json& record : InputData["Manufacturers"]) ManufacturersList.emplace_back(record["Name"]);
}


json DataParser::getInputdata(const std::string& ManufacturerName) {
	json VehicleData = json(nullptr);
	
	for (json& data : InputData["Manufacturers"]) {
		if (data["Name"] == ManufacturerName) VehicleData = data;
	}

	return VehicleData;
}


std::vector<std::string> DataParser::getManufacturersList() {
	return ManufacturersList;
}


void DataParser::createLogData(eVTOL* aircraft) {
	json AircraftLog{};
	std::string fileName = aircraft->getManufacturerName() + ".json";
	std::ofstream AircraftLogFile(fileName);

	if (AircraftLogFile.is_open()) {
		json SessionData{};
		SessionData["Start_Time"] = aircraft->getTimeForLogs(aircraft->getStartOperationTime());
		SessionData["End_Time"] = aircraft->getTimeForLogs(aircraft->getEndOperationTime());
		SessionData["Miles_Travelled"] = aircraft->getMilesPerSession();
		SessionData["Faults"] = aircraft->getFaultsPerSession();
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
