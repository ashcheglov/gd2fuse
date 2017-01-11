#include "PropertyTypePath.h"
#include "PropertyTypesRegistry.h"


Enum<IPropertyType::Type> PropertyTypePath::getType()
{
	return IPropertyType::PATH;
}

bool PropertyTypePath::checkValidity(const std::string &value)
{
	return fs::native(value);
}

G2F_REGISTER_REGULAR_TYPE(PropertyTypePath);
