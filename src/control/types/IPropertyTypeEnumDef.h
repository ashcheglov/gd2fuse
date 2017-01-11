#pragma once

#include <string>
#include "utils/decls.h"

class IPropertyTypeEnumDef
{

public:

	virtual size_t getSize() =0;
	virtual bool is(const std::string &name) =0;
	virtual std::string getEntry(size_t offset) =0;
	virtual std::string getDescription(size_t offset) =0;

	virtual ~IPropertyTypeEnumDef() {}
};

G2F_DECLARE_PTR(IPropertyTypeEnumDef);
