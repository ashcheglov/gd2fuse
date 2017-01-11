#pragma once

#include <string>
#include "utils/decls.h"
#include "IPropertyTypeEnumDef.h"
#include "utils/Enum.h"

class IPropertyType
{

public:
	enum Type
	{
		ENUM,
		UINT,
		OINT,
		BOOL,
		PATH
	};

	virtual Enum<Type> getType() =0;
	virtual bool checkValidity(const std::string &value) =0;
	virtual IPropertyTypeEnumDefPtr getEnumDef();

	virtual ~IPropertyType() {}
};

G2F_DECLARE_PTR(IPropertyType);

