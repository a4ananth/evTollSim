#include <ctime>
#include <chrono>
#include <utility>
#include <algorithm>

#include "DataLogger.h"
#include "RequestManager.h"
#include "ChargingStation.h"


std::mutex RequestManager::updatesMtx;
std::mutex RequestManager::requestsMtx;
std::mutex RequestManager::instancesMtx;

std::condition_variable RequestManager::chargingComplete;

std::atomic<bool> RequestManager::simulationComplete{ false };
std::queue<std::shared_ptr<RequestManager>> RequestManager::incomingRequests = {};
std::unordered_map<std::string, std::atomic<bool>> RequestManager::processedRequests = {};
std::unordered_map<std::string, std::shared_ptr<RequestManager>> RequestManager::instances = {};


void RequestManager::updateEndTime() {
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->getAircraft());
	this->endTime = std::chrono::system_clock::now();
	logger->logData("Charging process has ended for ticket number: " + this->getTicketNumber() + ".");
}


bool RequestManager::thankyou() const {
	bool complete = false;
	std::string servingTicketNumber = this->getTicketNumber();
	std::unordered_map<std::string, std::shared_ptr<RequestManager>>::iterator locate;
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->getAircraft());

	std::lock_guard<std::mutex> lock(RequestManager::instancesMtx);
	locate = RequestManager::instances.find(servingTicketNumber);
	if (locate != RequestManager::instances.end()) {
		if (locate->second->status.load()) {
			logger->logData("Charging process has been completed for ticket number: " + servingTicketNumber + ".");
			complete = locate->second->status.load();
		}

		/*if (complete) {
			RequestManager::instances.erase(locate);
			logger->logData("Request Manager instance for ticket number: " + servingTicketNumber + " has been removed.");
		}*/
	}

	return complete;
}


void RequestManager::updateStartTime() {
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->getAircraft());
	logger->logData("Charging process has started for ticket number: " + this->ticketNumber + ".");
	this->startTime = std::chrono::system_clock::now();
}


std::string RequestManager::getTicketNumber() const {
	return this->ticketNumber;
}


std::shared_ptr<evTOL> RequestManager::getAircraft() const {
	return aircraft;
}


void RequestManager::stopSimulation() {
	RequestManager::simulationComplete.store(true);
	RequestManager::chargingComplete.notify_all();	

	std::lock_guard<std::mutex> lock(RequestManager::instancesMtx);
	for (std::pair<std::string, std::shared_ptr<RequestManager>> instance : RequestManager::instances) {
		if (instance.second->statusThread.joinable()) instance.second->statusThread.join();
	}
}


std::size_t RequestManager::newRequestAvailable() {
	std::lock_guard<std::mutex> lock(RequestManager::requestsMtx);
	return (RequestManager::incomingRequests.empty()) ? 0 : RequestManager::incomingRequests.size();
}


std::shared_ptr<RequestManager> RequestManager::fetchFirstInLIne() {
	std::shared_ptr<RequestManager> firstInLine;

	{
		std::lock_guard<std::mutex> lock(RequestManager::requestsMtx);
		if (RequestManager::incomingRequests.empty()) throw std::runtime_error("No requests in the queue.");
		firstInLine = RequestManager::incomingRequests.front();
		RequestManager::incomingRequests.pop();
		ChargingStation::requestManagerNotification.notify_all();
	}

	firstInLine->updateStartTime();

	return firstInLine;
}


void RequestManager::reportChargingStatus(std::shared_ptr<RequestManager>& thisRequest) {
	bool complete = false;
	std::unordered_map<std::string, std::atomic<bool>>::iterator locate;
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(thisRequest->getAircraft());
	 
	{
		std::lock_guard<std::mutex> lock(RequestManager::updatesMtx);
		locate = RequestManager::processedRequests.find(thisRequest->getTicketNumber());
		if (locate != RequestManager::processedRequests.end()) {
			complete = true;
			locate->second.store(true);
			logger->logData("Charger has returned aircraft assigned to ticket number: " + thisRequest->getTicketNumber() + ".");

			// Notify the waiting threads
			RequestManager::chargingComplete.notify_all();
		}
	}

	if (complete) {
		logger->logData("Closing the charging process for ticket number: " + thisRequest->getTicketNumber() + ".");
		thisRequest->markChargingProcessCompleted(thisRequest->getTicketNumber());
	}
}


std::shared_ptr<RequestManager> RequestManager::getRequest(const std::string& ticketNumber) {
	std::shared_ptr<RequestManager> thisRequest = nullptr;
	std::unordered_map<std::string, std::shared_ptr<RequestManager>>::iterator locate;

	{
		std::lock_guard<std::mutex> lock(RequestManager::instancesMtx);
		locate = RequestManager::instances.find(ticketNumber);
		if (locate != RequestManager::instances.end()) {
			thisRequest = locate->second;		
		}
	}

	return thisRequest;
}


std::string RequestManager::createChargingRequest(const std::shared_ptr<evTOL>& aircraft) {
	std::string ticketNumber = RequestManager::createNewRequest(aircraft);
	std::unordered_map<std::string, std::shared_ptr<RequestManager>>::iterator locate;
	
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(aircraft);

	{
		std::lock_guard<std::mutex> lock(RequestManager::instancesMtx);
		locate = RequestManager::instances.find(ticketNumber);
		if (locate != RequestManager::instances.end()) {
			locate->second->addToStatusMonitor();
			locate->second->addToRequestQueue(locate->second);
			locate->second->statusThread = std::thread(&RequestManager::monitorChargingRequest, locate->second, std::ref(locate->second->ticketNumber));
			logger->logData("The thread to monitor the ticket number: " + ticketNumber + " has been created.");
		}
	}

	return ticketNumber;
}


void RequestManager::addToRequestQueue(const std::shared_ptr<RequestManager>& thisRequest) const {
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(thisRequest->getAircraft());

	{
		std::lock_guard<std::mutex> lock(RequestManager::requestsMtx);
		RequestManager::incomingRequests.push(thisRequest);
		logger->logData("Request with ticket number: " + this->ticketNumber + " has been added to the queue.");
		ChargingStation::requestManagerNotification.notify_all();
		logger->logData("Notification sent to the charging station.");
	}
}


void RequestManager::addToStatusMonitor() const {
	std::string ticketNumber = this->getTicketNumber();		//TODO: define enum class for states 
	std::unordered_map<std::string, std::atomic<bool>>::iterator locate;
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->getAircraft());

	{
		std::lock_guard<std::mutex> lock(RequestManager::updatesMtx);
		locate = RequestManager::processedRequests.find(ticketNumber);
		if (locate == RequestManager::processedRequests.end()) {
			logger->logData("A new tracker flag for ticket number: " + ticketNumber + " has been created.");
			RequestManager::processedRequests.emplace(std::move(ticketNumber), false);
		}
	}
}


std::string RequestManager::generateTicketNumber() const {
	std::string prefix = this->aircraft->getManufacturerName();
	for_each(prefix.begin(), prefix.end(), [](char& ch) {
		ch = std::toupper(ch);
		});

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
	std::string ticketNumber = prefix + '-' + std::to_string(now_time_t);

	return ticketNumber;
}


void RequestManager::monitorChargingRequest(const std::string& ticketNumber) const {
	bool complete = false;
	std::unordered_map<std::string, std::atomic<bool>>::iterator locate;
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->getAircraft());
		
	while (!complete) {
		std::unique_lock<std::mutex> lock(RequestManager::updatesMtx);
		RequestManager::chargingComplete.wait(lock, [&] {
			locate = RequestManager::processedRequests.find(ticketNumber);
			return (locate != RequestManager::processedRequests.end() && locate->second.load()) ||
				simulationComplete.load();
			});
		
		locate = RequestManager::processedRequests.find(ticketNumber);

		if (simulationComplete.load()) break;
		else if (locate != RequestManager::processedRequests.end()) {
			complete = locate->second.load();
			logger->logData("Aircraft associated with ticket number: " + ticketNumber
				+ "'s service status is now " + (complete ? "Complete" : "Charging"));
		}
	}	
}


void RequestManager::markChargingProcessCompleted(const std::string& ticketNumber) const {
	std::unordered_map<std::string, std::shared_ptr<RequestManager>>::iterator locate;
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(this->getAircraft());

	std::lock_guard<std::mutex> lock(RequestManager::instancesMtx);
	locate = RequestManager::instances.find(ticketNumber);

	if (locate != RequestManager::instances.end()) {
		locate->second->status.store(true);
		if (locate->second->statusThread.joinable()) {
			locate->second->statusThread.join();
			logger->logData("The thread spawned to monitor the status from the charger is now joined.");
		}

		logger->logData("Tracker flag for ticket number: " + ticketNumber + " has been marked as completed.");
		locate->second->getAircraft()->getAircraftCV().notify_all();
		logger->logData("Notification sent to the aircraft.");
	}
}


std::string RequestManager::createNewRequest(const std::shared_ptr<evTOL>& aircraft) {
	std::shared_ptr<DataLogger> logger = DataLogger::getInstance(aircraft);

	// Create a new request
	std::shared_ptr<RequestManager> newRequest = RequestManager::createInstance(aircraft);

	logger->logData("A new request has been created for the aircraft: " + aircraft->getManufacturerName() + ".");
	logger->logData("The ticket number assigned to the request is: " + newRequest->getTicketNumber() + ".");

	std::unordered_map<std::string, std::shared_ptr<RequestManager>>::iterator locate;

	std::lock_guard<std::mutex> lock(RequestManager::instancesMtx);
	locate = RequestManager::instances.find(newRequest->getTicketNumber());
	if (locate == RequestManager::instances.end()) {
		RequestManager::instances.emplace(newRequest->getTicketNumber(), newRequest);
		logger->logData("The request has been added to the instances map.");
	}

	return newRequest->getTicketNumber();
}


RequestManager::RequestManager(const std::shared_ptr<evTOL>& aircraft) : aircraft(aircraft) {
	status.store(false);
	ticketNumber = this->generateTicketNumber();
	endTime = std::chrono::system_clock::time_point();
	startTime = std::chrono::system_clock::time_point();
}


template<typename ...Args>
inline std::shared_ptr<RequestManager> RequestManager::createInstance(Args && ...args)
{
	class make_shared_enabler : public RequestManager {
	public:
		make_shared_enabler(Args &&... args) : RequestManager(std::forward<Args>(args)...) {}
	};

	return std::make_shared<make_shared_enabler>(std::forward<Args>(args)...);
}