#include "PropertyTypeBool.h"
#include "PropertyTypesRegistry.h"
#include <boost/lexical_cast.hpp>

Enum<IPropertyType::Type> PropertyTypeBool::getType()
{
	return IPropertyType::BOOL;
}

bool PropertyTypeBool::checkValidity(const std::string &value)
{
	try
	{
		boost::lexical_cast<bool>(value);
	}
	catch(...)
	{
		return false;
	}
	return true;
}


G2F_REGISTER_REGULAR_TYPE(PropertyTypeBool);
