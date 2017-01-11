#include "PropertyTypeEnum.h"
#include "PropertyTypesRegistry.h"


class EnumDef : public IPropertyTypeEnumDef
{

public:
	EnumDef(const PropertyTypesRegistry::EnumEntries &entries)
		: _entries(entries)
	{
		std::sort(_entries.begin(),_entries.end(),[](auto left,auto right){return left.entry<right.entry;});
	}

	// IPropertyTypeEnumDef interface
	virtual size_t getSize() override
	{
		return _entries.size();
	}
	virtual bool is(const std::string &name) override
	{
		PropertyTypesRegistry::EnumEntry e;
		e.entry=name;
		auto cmp=[](auto left,auto right){return left.entry<right.entry;};
		return std::binary_search(_entries.begin(),_entries.end(),e,cmp);
	}
	virtual std::string getEntry(size_t offset) override
	{
		return _entries[offset].entry;
	}
	virtual std::string getDescription(size_t offset) override
	{
		return _entries[offset].description;
	}

private:
	PropertyTypesRegistry::EnumEntries _entries;
};



//
//	PropertyTypeEnum definition
//
////////////////////////////////////////////
PropertyTypeEnum::PropertyTypeEnum(const PropertyTypesRegistry::EnumEntries &entries)
{
	_def=std::make_shared<EnumDef>(entries);
}

Enum<IPropertyType::Type> PropertyTypeEnum::getType()
{
	return IPropertyType::ENUM;
}

bool PropertyTypeEnum::checkValidity(const std::string &value)
{
	return _def->is(value);
}

IPropertyTypeEnumDefPtr PropertyTypeEnum::getEnumDef()
{
	return _def;
}

