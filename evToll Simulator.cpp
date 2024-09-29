// evToll Simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "eVTOL.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{

	json j;
	j["name"] = "Ananth";
	j["age"] = 30;
	j["city"] = "Omaha";

	std::cout << j.dump(4) << std::endl;

	std::string json_string = R"({"name": "Alice", "age": 25, "city": "London"})";
	json j2 = json::parse(json_string);

	// Access values from the JSON object
	std::cout << "Name: " << j2["name"] << std::endl;
	std::cout << "Age: " << j2["age"] << std::endl;
	std::cout << "City: " << j2["city"] << std::endl;


	/*std::shared_ptr<eVTOL> myCraft = std::make_shared<eVTOL>(120, 4, 320, 1.6, 0.25, 0.6);
	myCraft->startSimulation();*/
	return 0;
}
