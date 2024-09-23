#pragma once
#include "eVTOL.h"
#include <string>
#include <vector>
#include <memory>



class Alpha : public eVTOL {
	std::vector<std::shared_ptr<eVTOL*>> inUse;
};

