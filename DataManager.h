#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "Fleet.h"


using json = nlohmann::json;


class DataManager {
	// Flag to denote if system has already initialized. This will reset upon destruction
	static bool IsInitialized;

	// Maps that store the configuration data and the reference to the logger instance
	static std::unordered_map<std::string, json> manufacturerData;
	static std::unordered_map<std::string, std::shared_ptr<DataManager>> LoggerInstances;

	// Flag to enforce the readInputData function only once
	std::once_flag readOnce;

	void readInputData();
	void SaveLog(const json& LogData, const std::string& LogFileName);


	DataManager();

public:

	static void InitializeDataManager();
	static std::shared_ptr<DataManager> getInstance(const std::string& ManufacturerName);

	static std::size_t getNumberOfManufacturers();
	static std::vector<std::string> getAllManufacturers();

	json getInputdata(const std::string& ManufacturerName);
	
	void createLogData(const std::shared_ptr<Fleet>& aircraft);
	
	~DataManager() = default;
};

