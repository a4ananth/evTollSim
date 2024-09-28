// evToll Simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "eVTOL.h"

int main()
{
	std::shared_ptr<eVTOL> myCraft = std::make_shared<eVTOL>(120, 4, 320, 1.6, 0.25, 0.6);
	myCraft->startSimulation();
	return 0;
}
