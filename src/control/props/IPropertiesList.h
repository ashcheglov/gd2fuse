#pragma once

#include <string>
#include "utils/decls.h"
#include "IPropertyDefinition.h"
#include "IPropertyIterator.h"
#include "fs/IFileSystem.h"

class IPropertiesList
{

public:

	virtual std::string getName() =0;
	virtual IPropertyDefinitionPtr findProperty(const std::string &name) =0;
	virtual bool isEmpty() =0;
	virtual size_t getPropertiesSize() =0;
	virtual IPropertyIteratorPtr getProperties() =0;

	virtual ~IPropertiesList() {}
};

G2F_DECLARE_PTR(IPropertiesList);
