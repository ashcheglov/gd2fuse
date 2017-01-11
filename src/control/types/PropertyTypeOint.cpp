#include "PropertyTypeOint.h"
#include "PropertyTypesRegistry.h"


Enum<IPropertyType::Type> PropertyTypeOint::getType()
{
	return IPropertyType::OINT;
}

bool PropertyTypeOint::checkValidity(const std::string &value)
{
	for(auto c : value)
	{
		if(c<'0' || c>'9')
			return false;
	}
	return true;
}

G2F_REGISTER_REGULAR_TYPE(PropertyTypeOint);
