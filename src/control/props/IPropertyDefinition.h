#pragma once

#include <string>
#include <time.h>
#include "utils/decls.h"
#include "control/types/IPropertyType.h"
#include "error/G2FException.h"

class IPropertyDefinition
{

public:

	virtual std::string getName() =0;
	virtual std::string getValue() =0;
	virtual G2FError setValue(const std::string &val) =0;
	virtual std::string getDescription() =0;
	virtual IPropertyTypePtr getType() =0;
	virtual bool isRuntimeChange() =0;
	virtual std::string getDefaultValue() =0;
	virtual timespec getLastChangeTime() =0;

	virtual ~IPropertyDefinition() {}
};

G2F_DECLARE_PTR(IPropertyDefinition);
