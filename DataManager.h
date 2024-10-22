#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "eVTOL.h"


using json = nlohmann::json;


class DataManager {
	// Flag to denote if system has already initialized. This will reset upon destruction
	static bool IsInitialized;
		
	static std::unordered_map<std::string, json> manufacturerData;
	static std::unordered_map<std::string, DataManager*> LoggerInstances;
	static void readInputData();

	static json InputData;

	void SaveLog(const json& LogData, const std::string& LogFileName);

	DataManager();
public:
	static std::size_t getNumberOfManufacturers();
	static void InitializeDataManager();
	static DataManager* getInstance(const std::string& ManufacturerName);

	json getInputdata(const std::string& ManufacturerName);
	void createLogData(std::unique_ptr<eVTOL>& aircraft);
	
	~DataManager() = default;
};

