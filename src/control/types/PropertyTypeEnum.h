#pragma once

#include "IPropertyType.h"
#include "PropertyTypesRegistry.h"

class PropertyTypeEnum : public IPropertyType
{

public:
	PropertyTypeEnum(const PropertyTypesRegistry::EnumEntries &entries);

	// IPropertyType interface
	virtual Enum<Type> getType() override;
	virtual bool checkValidity(const std::string &value) override;
	virtual IPropertyTypeEnumDefPtr getEnumDef() override;

private:
	IPropertyTypeEnumDefPtr _def;
};
