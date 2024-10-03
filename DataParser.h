#pragma once
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

#include "eVTOL.h"


using json = nlohmann::json;


class DataParser {
	json InputData{};
	std::vector<std::string> ManufacturersList;
public:
	DataParser();

	json getInputdata(const std::string& ManufacturerName);
	std::vector<std::string> getManufacturersList();
	void createLogData(eVTOL* aircraft);
	void SaveLog(const json& LogData, const std::string& LogFileName);

	~DataParser() = default;
};

