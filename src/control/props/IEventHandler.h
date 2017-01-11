#pragma once

#include <string>
#include "utils/decls.h"
#include "IPropertyDefinition.h"

class IEventHandler
{

public:

	virtual void changed(const IPropertyDefinitionPtr &prop, const std::string &oldValue, const std::string &newValue) =0;

	virtual ~IEventHandler() {}
};

G2F_DECLARE_PTR(IEventHandler);
