#pragma once

#include <string>
#include <time.h>
#include "control/types/IPropertyType.h"
#include "IEventHandler.h"

class Property
{

public:
	Property();

	//void onChanged(IEventHandler *handler);

private:
	IPropertyTypePtr type;
	std::string value;
	bool runtime;
	std::string name;

	timespec lastChanged;
	//std::vector<IEventHandler*> _listeners;
	std::string defaultValue;

};
