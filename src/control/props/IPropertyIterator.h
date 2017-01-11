#pragma once

#include "utils/decls.h"
#include "IPropertyDefinition.h"

class IPropertyIterator
{

public:

	virtual bool hasNext() =0;
	virtual IPropertyDefinitionPtr next() =0;

	virtual ~IPropertyIterator() {}
};

G2F_DECLARE_PTR(IPropertyIterator);
