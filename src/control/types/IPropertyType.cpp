#include "IPropertyType.h"

BEGIN_ENUM_MAP_C(IPropertyType,Type)
	ENUM_MAP_ITEM_STR(IPropertyType::ENUM,	"ENUM")
	ENUM_MAP_ITEM_STR(IPropertyType::UINT,	"UINT")
	ENUM_MAP_ITEM_STR(IPropertyType::OINT,	"OINT")
	ENUM_MAP_ITEM_STR(IPropertyType::BOOL,	"BOOL")
	ENUM_MAP_ITEM_STR(IPropertyType::PATH,	"PATH")
END_ENUM_MAP_C


IPropertyTypeEnumDefPtr IPropertyType::getEnumDef()
{
	return IPropertyTypeEnumDefPtr();
}
