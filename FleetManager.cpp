#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "FleetManager.h"


std::once_flag FleetManager::initialized;
std::size_t FleetManager::numManufacturers = 0;
std::vector<std::thread> FleetManager::fleetThreads = {};
std::unique_ptr<FleetManager> FleetManager::instance = nullptr;
std::unordered_map<std::string, json> FleetManager::fleetData = {};
std::unordered_map<std::string, std::size_t> FleetManager::fleetSizes = {};


void FleetManager::InitializeFleet(const std::size_t& numAircrafts) {
    std::call_once(FleetManager::initialized, [&numAircrafts] {
        instance = std::make_unique<FleetManager>();
        
        instance->readInputData();
        instance->assignCapacity(numAircrafts);
        
        FleetManager::fleetThreads.reserve(numAircrafts);
		
		instance->constructFleet(numAircrafts);
        });
}


void FleetManager::stopSimulation() {
	evTOL::retireSimulation();
    RequestManager::stopSimulation();
    ChargingStation::stopSimulation();

    for (std::thread& _thread : FleetManager::fleetThreads) {
		if (_thread.joinable()) _thread.join();
	}
}


void FleetManager::readInputData() {
    json InputData = {};

    std::ifstream InputDataFile("Manufacturer.json");
    
    if (!InputDataFile.is_open()) throw std::runtime_error("Unable to open input data json");

    InputDataFile >> InputData;

    FleetManager::numManufacturers = InputData["Manufacturers"].size();

    FleetManager::fleetData.reserve(FleetManager::numManufacturers);
    FleetManager::fleetSizes.reserve(FleetManager::numManufacturers);

    for (std::size_t i = 0; i < FleetManager::numManufacturers; ++i) {
		json manufacturerData = InputData["Manufacturers"][i];
		std::string key = manufacturerData.at("Name").get<std::string>();
        
        FleetManager::fleetSizes.emplace(key, 0);
        FleetManager::fleetData.emplace(key, manufacturerData);
        
	}

    InputDataFile.close();
}


void FleetManager::constructFleet(const std::size_t& numVehicles) {
    for_each(FleetManager::fleetData.begin(), FleetManager::fleetData.end(), [&](const std::pair<const std::string, json>& data) {
		std::string manufacturerName = data.first;
        std::size_t fleetSize = FleetManager::fleetSizes.at(manufacturerName);

        for (std::size_t j = 0; j < fleetSize; ++j) {
            std::shared_ptr<evTOL> newAircraft = std::make_shared<FleetManager>(data.second, (j + 1));
            FleetManager::fleetThreads.emplace_back(&evTOL::startSimulation, newAircraft);
        }

        });
}


std::string FleetManager::generateSerialNumber() const {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);

    const char* months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    const char* days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

    std::ostringstream oss;

    oss << months[tm.tm_mon]
        << std::setw(2) << std::setfill('0') << tm.tm_mday
        << days[tm.tm_wday] 
        << std::setw(2) << std::setfill('0') << tm.tm_hour
        << std::setw(2) << std::setfill('0') << tm.tm_min
        << std::setw(2) << std::setfill('0') << tm.tm_sec;

    return oss.str();
}


void FleetManager::assignCapacity(const std::size_t& fleetSize) {
	std::random_device rd;
	std::mt19937 gen(rd());

	std::size_t remainingCapacity = fleetSize;
    std::unordered_map<std::string, std::size_t>::iterator position = FleetManager::fleetSizes.begin();

	for (std::size_t i = 0; i < numManufacturers && remainingCapacity > 0; ++i) {
		std::uniform_int_distribution<std::size_t> dist(1, remainingCapacity - (numManufacturers - 1 - i));
		std::size_t capacity = (i == (numManufacturers - 1)) ? remainingCapacity : dist(gen);
        position->second = capacity;
		remainingCapacity -= capacity;

		++position;
	}
}


void FleetManager::setManufacturerName(const std::size_t sNo) {
    std::string serialNumber = (sNo < 10) ? "0" + std::to_string(sNo) : std::to_string(sNo);
	modelNumber = evTOL::get_manufacturer() + serialNumber + "_" + generateSerialNumber();
}


double FleetManager::getPassengerMiles() const {
    std::string manufacturerName = evTOL::get_manufacturer();
    std::size_t fleetSize = FleetManager::fleetSizes.at(manufacturerName);
	
    return (getAirTime().count() * getCruiseSpeed() * getMaxPassengerCount() * fleetSize) / 3600.0;
}


double FleetManager::getMilesPerSession() const {
    /*
    * This function is written for logging the Miles travelled per session.
    * The result is rounded off and then truncated to 2 decimal places
    */

    double MilesTravelled = getCruiseSpeed() * (getAirTime().count() / 3600.0);
    double factor = std::pow(10, 2);

    return std::round(MilesTravelled * factor) / factor;
}


double FleetManager::getFaultsPerSession() const {
    return getAirTime().count() / 3600.0;
}


std::string FleetManager::getManufacturerName() const {
    return modelNumber;
}

FleetManager::FleetManager::FleetManager(const json& AircraftData, const std::size_t sNo) :
    evTOL(AircraftData)
{
	setManufacturerName(sNo);
}

