#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "Fleet.h"


using json = nlohmann::json;


class DataManager {
	// Flag to denote if system has already initialized. This will reset upon destruction
	static bool IsInitialized;

	// Data manager handles data for all aircrafts. Hence its Singleton
	static DataManager* DataMgrInstance;

	// Flag to enforce the readInputData function only once
	std::once_flag readOnce;

	
	// Maps that store the configuration data and the reference to the logger instance
	static std::unordered_map<std::string, json> manufacturerData;
	static std::unordered_map<std::string, DataManager*> LoggerInstances;

	void SaveLog(const json& LogData, const std::string& LogFileName);
	void readInputData();

	DataManager();

public:

	static void InitializeDataManager();
	static DataManager* getInstance(const std::optional<std::string>& ManufacturerName);

	std::size_t getNumberOfManufacturers();
	std::vector<std::string> getAllManufacturers();

	json getInputdata(const std::string& ManufacturerName);
	
	void createLogData(const std::shared_ptr<Fleet>& aircraft);
	
	~DataManager();
};

