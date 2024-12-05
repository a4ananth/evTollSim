#pragma once

#include <mutex>
#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include <unordered_map>

#include "evTOL.h"	


class DataLogger {
public:	
	DataLogger(DataLogger&& other) = delete;							// Move constructor
	DataLogger(const DataLogger& other) = delete;						// Copy constructor
	DataLogger& operator= (const DataLogger& other) = delete;			// Copy assignment operator

	DataLogger& operator= (DataLogger&& other) noexcept = default;		// Default move assignment operator
	
	// DataLogger public APIs
	void logData(const std::string& data);								// Log data to the file
	void performanceSummary(const std::shared_ptr<evTOL>& aircraft);	// Log performance summary to the file

	// Static member functions
	static std::shared_ptr<DataLogger> getInstance(const std::shared_ptr<evTOL>& aircraft);	// Get the instance of the DataLogger	

protected:
	void writeToFile(const json& LogData) const;						// Write data to the file
	void writeToFile(const std::string& data) const;					// Write data to the file
	bool isFileEmpty(const std::filesystem::path& filepath) const;		// Check if the file is empty
	bool isFilePresent(const std::filesystem::path& filepath) const;	// Check if the file exists

private:	
	// Static data members
	static std::mutex instancesMtx;													// Mutex to lock the instances map
	static std::unordered_map<std::string, std::shared_ptr<DataLogger>> instances;	// Map to store instances of the DataLogger

	std::mutex fileMtx;								// Mutex to lock the file
	std::filesystem::path logFile;					// File stream object to write data to the file
	std::filesystem::path summaryFile;				// File stream object to write data to the file

	std::shared_ptr<evTOL> aircraft;				// Aircraft object to log data
	
	// DataLogger Class object control methods
	DataLogger(const std::shared_ptr<evTOL>& aircraft);		// Parametrized constructor

	// Template function to create shared pointer instance of RequestManager class
	template <typename... Args>
	static std::shared_ptr<DataLogger> createInstance(Args &&... args);
};

