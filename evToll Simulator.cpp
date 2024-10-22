// evToll Simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "eVTOL.h"
#include "Fleet.h"
#include "DataManager.h"


using json = nlohmann::json;


int main()
{
	DataManager data;
	
	std::vector<std::string> eVTOLManufacturers = data.getManufacturersList();
	std::vector<std::unique_ptr<eVTOL>> vehicles{};
	
	for (std::string& company : eVTOLManufacturers) {
		json VehicleData = data.getInputdata(company);
		std::unique_ptr<eVTOL> vehicle = std::make_unique<Fleet>(company, VehicleData);
	}
	
	return 0;
}
