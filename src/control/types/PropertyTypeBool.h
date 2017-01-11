#pragma once

#include "IPropertyType.h"

class PropertyTypeBool : public IPropertyType
{

	// IPropertyType interface
public:
	virtual Enum<Type> getType() override;
	virtual bool checkValidity(const std::string &value) override;
};
