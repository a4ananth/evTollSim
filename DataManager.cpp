#pragma once

#include <fstream>
#include <iostream>

#include "Fleet.h"
#include "DataManager.h"

// Declaring static member variables for the class

bool DataManager::IsInitialized = false;
DataManager* DataManager::DataMgrInstance = nullptr;
std::unordered_map<std::string, json> DataManager::manufacturerData = {};
std::unordered_map<std::string, DataManager*> DataManager::LoggerInstances = {};


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
	if (!IsInitialized) {
		std::call_once(readOnce, &DataManager::readInputData, this);
		
		for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
			DataManager* instance = new DataManager();
			LoggerInstances[manufacturer.first] = instance;
		}

		IsInitialized = true;
	}
}


std::size_t DataManager::getNumberOfManufacturers() {
	if(!DataMgrInstance) return DataMgrInstance->manufacturerData.size();
	return 0;
}


void DataManager::InitializeDataManager() {
	if (!IsInitialized) DataMgrInstance = new DataManager();
	return;
}


DataManager* DataManager::getInstance(const std::optional<std::string>& ManufacturerName) {
	if (ManufacturerName) {
		if (LoggerInstances.find(*ManufacturerName) != LoggerInstances.end()) 
			return LoggerInstances[*ManufacturerName];

		return nullptr;
	}
	
	return DataMgrInstance;
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


DataManager::~DataManager() {
	for (const std::pair<std::string, json>& manufacturer : manufacturerData) {
		DataManager* Instance = LoggerInstances[manufacturer.first];
		json ManufacturerData = manufacturerData[manufacturer.first];
		
		ManufacturerData.clear();
		delete Instance;
		Instance = nullptr;

		manufacturerData.erase(manufacturer.first);
		LoggerInstances.erase(manufacturer.first);
	}
}