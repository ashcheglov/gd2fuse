#include "PropertyTypeUint.h"
#include "PropertyTypesRegistry.h"
#include "utils/Enum.h"
#include <boost/lexical_cast.hpp>


Enum<IPropertyType::Type> PropertyTypeUint::getType()
{
	return IPropertyType::UINT;
}

bool PropertyTypeUint::checkValidity(const std::string &value)
{
	try
	{
		boost::lexical_cast<uint32_t>(value);
	}
	catch(...)
	{
		return false;
	}
	return true;
}

G2F_REGISTER_REGULAR_TYPE(PropertyTypeUint);
