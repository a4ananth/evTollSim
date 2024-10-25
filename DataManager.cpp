#pragma once

#include <fstream>
#include <iostream>

#include "Fleet.h"
#include "DataManager.h"

// Declaring static member variables for the class

bool DataManager::IsInitialized = false;
std::unordered_map<std::string, json> DataManager::manufacturerData = {};
std::unordered_map<std::string, std::shared_ptr<DataManager>> DataManager::LoggerInstances = {};


void DataManager::readInputData() {
	if (!IsInitialized) {
		json InputData = {};

		std::ifstream InputDataFile("Manufacturer.json");
		if (!InputDataFile.is_open()) throw std::runtime_error("Unable to open input data json");

		InputDataFile >> InputData;

		for (json& record : InputData["Manufacturers"]) manufacturerData[record["Name"]] = record;
	}
}


DataManager::DataManager() {
	std::call_once(readOnce, &DataManager::readInputData, this);
}


std::size_t DataManager::getNumberOfManufacturers() {
	if(IsInitialized) return manufacturerData.size();
	return 0;
}


void DataManager::InitializeDataManager() {
	if (!IsInitialized) {
		DataManager dataHandler;

		for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
			DataManager* loggerInstance = new DataManager();
			std::shared_ptr<DataManager> instance(loggerInstance);
			LoggerInstances[manufacturer.first] = instance;

			std::cout << "Created a new DataManager instance for " << manufacturer.first << std::endl;
		}

		IsInitialized = true;
	}

	return;
}


std::shared_ptr<DataManager> DataManager::getInstance(const std::string& ManufacturerName) {
	if (ManufacturerName.empty()) {
		if (LoggerInstances.find(ManufacturerName) != LoggerInstances.end()) 
			return LoggerInstances[ManufacturerName];

		return nullptr;
	}
	
	return nullptr;
}


json DataManager::getInputdata(const std::string& ManufacturerName) {
	json VehicleData = json(nullptr);
	
	for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
		if (manufacturer.first == ManufacturerName) VehicleData = manufacturer.second;
	}

	return VehicleData;
}


std::vector<std::string> DataManager::getAllManufacturers() {
	std::vector<std::string> AllManufacturers;

	for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
		AllManufacturers.emplace_back(manufacturer.first);
	}

	return AllManufacturers;
}


void DataManager::createLogData(const std::shared_ptr<Fleet>& aircraft) {
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