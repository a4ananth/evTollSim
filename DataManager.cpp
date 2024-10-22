#pragma once

#include <fstream>
#include <iostream>

#include "DataManager.h"


// Declaring static member variables for the class

json DataManager::InputData = {};
bool DataManager::IsInitialized = false;
std::unordered_map<std::string, json> DataManager::manufacturerData = {};
std::unordered_map<std::string, DataManager*> DataManager::LoggerInstances = {};


void DataManager::readInputData() {
	std::ifstream InputDataFile("Manufacturer.json");
	if (!InputDataFile.is_open()) throw std::runtime_error("Unable to open input data json");

	InputDataFile >> InputData;

	for (json& record : InputData["Manufacturers"]) manufacturerData[record["Name"]] = record;

	IsInitialized = true;
}


DataManager::DataManager() {
	for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
		DataManager* instance = new DataManager();
		LoggerInstances[manufacturer.first] = instance;
	}
}


std::size_t DataManager::getNumberOfManufacturers() {
	return DataManager::manufacturerData.size();
}

void DataManager::InitializeDataManager() {
	if (!IsInitialized) {
		readInputData();
		DataManager();
	}
}


DataManager* DataManager::getInstance(const std::string& ManufacturerName) {
	if (LoggerInstances.find(ManufacturerName) != LoggerInstances.end()) return LoggerInstances[ManufacturerName];
	return nullptr;
}


json DataManager::getInputdata(const std::string& ManufacturerName) {
	json VehicleData = json(nullptr);
	
	for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
		if (manufacturer.first == ManufacturerName) VehicleData = manufacturer.second;
	}

	return VehicleData;
}


void DataManager::createLogData(std::unique_ptr<eVTOL>& aircraft) {
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


void DataManager::SaveLog(const json& LogData, const std::string& LogFileName) {
	std::ofstream file(LogFileName);
	
	if (!file.is_open()) throw std::runtime_error("Unable to open log file");

	file << LogData.dump(4);
}