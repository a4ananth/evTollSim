#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "eVTOL.h"

using json = nlohmann::json;


class DataParser {
public:
	DataParser() = default;

	json getInputdata();
	void createLogData(const eVTOL* aircraft);
	void SaveLog(const json& LogData, const std::string& LogFileName);
};

