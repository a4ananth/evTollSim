#include <filesystem>
#include <nlohmann/json.hpp>

#include "DataLogger.h"


std::mutex DataLogger::instancesMtx;
std::unordered_map<std::string, std::shared_ptr<DataLogger>> DataLogger::instances = {};


void DataLogger::logData(const std::string& data) {
	std::string timeStamp = "[" + aircraft->getTimeForLogs(std::chrono::system_clock::now()) + "]";
	std::string logData = timeStamp + " : " + data;

	if (this->isFilePresent(logFile)) {
		std::lock_guard<std::mutex> lock(fileMtx);
		writeToFile(logData);
	}
}


void DataLogger::performanceSummary(const std::shared_ptr<evTOL>& aircraft) {
	json AircraftLog{};
	
	if (isFilePresent(summaryFile)) {
		std::ifstream AircraftLogFile(summaryFile);
		
		if (!isFileEmpty(summaryFile)) AircraftLogFile >> AircraftLog;
		else AircraftLog["Sessions"] = json::array();
		
		AircraftLogFile.close();
	}
	
	json SessionData{};

	SessionData["Start_Time"] = aircraft->getTimeForLogs(aircraft->getStartOperationTime());
	SessionData["End_Time"] = aircraft->getTimeForLogs(aircraft->getEndOperationTime());
	SessionData["Miles_Travelled"] = aircraft->getMilesPerSession();
	SessionData["Faults"] = aircraft->getFaultsPerSession();
	SessionData["Passenger_Miles"] = aircraft->getPassengerMiles();

	AircraftLog["Sessions"].emplace_back(SessionData);

	if (isFilePresent(summaryFile)) {
		std::lock_guard<std::mutex> lock(fileMtx);
		writeToFile(AircraftLog);
	}
}


std::shared_ptr<DataLogger> DataLogger::getInstance(const std::shared_ptr<evTOL>& aircraft) {
	std::string aircraftName = aircraft->getManufacturerName();
	std::unordered_map<std::string, std::shared_ptr<DataLogger>>::iterator locate;
	std::pair< std::unordered_map<std::string, std::shared_ptr<DataLogger>>::iterator, bool> inserter;
	
	std::lock_guard<std::mutex> lock(DataLogger::instancesMtx);
	locate = DataLogger::instances.find(aircraftName);
	if (locate == DataLogger::instances.end()) {
		std::shared_ptr<DataLogger> instance = createInstance(aircraft);
			
		std::pair<std::string, std::shared_ptr<DataLogger>> instanceMapData(aircraftName, std::move(instance));
		inserter = DataLogger::instances.insert(instanceMapData);
			
		if (inserter.second) {
			inserter.first->second->logData("Log file created");
			return inserter.first->second;
		}
	}

	return locate->second;
}


bool DataLogger::isFilePresent(const std::filesystem::path& filepath) const {
	return std::filesystem::exists(filepath);
}


void DataLogger::writeToFile(const std::string& data) const {
	std::ofstream file;

	file.open(logFile, std::ios::out | std::ios::app);

	if (!file) throw std::runtime_error("Unable to open file for writing");

	file << data << "\n";

	file.close();
}


bool DataLogger::isFileEmpty(const std::filesystem::path& filepath) const {
	return (std::filesystem::file_size(filepath) == 0);
}


void DataLogger::writeToFile(const json& LogData) const {
	std::ofstream LogFile(summaryFile, std::ios::out | std::ios::trunc);

	if (LogFile.is_open()) {
		LogFile << LogData.dump(4);
		LogFile.close();
	}
}


DataLogger::DataLogger(const std::shared_ptr<evTOL>& aircraft) : aircraft(aircraft)
{
	std::filesystem::create_directory("Logs");
	logFile = "Logs/" + (aircraft->getManufacturerName() + "_DataLogger.txt");
    std::ofstream log(logFile, std::ios::out | std::ios::trunc);

	std::filesystem::create_directory("Summary");
	summaryFile = "Summary/" + (aircraft->getManufacturerName() + "_Summary.json");
	std::ofstream summary(summaryFile, std::ios::out | std::ios::trunc);
}


template<typename ...Args>
inline std::shared_ptr<DataLogger> DataLogger::createInstance(Args && ...args)
{
	class make_shared_enabler : public DataLogger {
	public:
		make_shared_enabler(Args &&... args) : DataLogger(std::forward<Args>(args)...) {}
	};

	return std::make_shared<make_shared_enabler>(std::forward<Args>(args)...);
}