#pragma once
#include "eVTOL.h"

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Logger {
public:
	void info(const json& Manufacturer, const std::string& ManufacturerName);
	void write(const eVTOL* aircraft, const json& aircraftLogData);
};

