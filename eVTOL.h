#pragma once
class eVTOL
{
	int CruiseSpeed;
	int PassengerCount;
	int BatteryCapacity;
	
	double TimeToCharge;
	double CruiseEnergy;
	double FaultsPerHour;
	double currentOperatingTime;
public:
	eVTOL() = default;

	virtual void resetCharge() = 0;
	virtual void trackRemainingCharge() = 0;
	virtual int getTimeToCharge() = 0;
	virtual void start() = 0;

	virtual ~eVTOL() = 0;
};

