#pragma once

#include "utils/decls.h"
#include "IPropertyType.h"
#include <boost/unordered_map.hpp>

class PropertyTypesRegistry
{
public:
	struct EnumEntry
	{
		std::string entry;
		std::string description;
	};
	typedef std::vector<EnumEntry> EnumEntries;

	IPropertyTypePtr getType(IPropertyType::Type type,const std::string& enumCode=std::string());
	IPropertyTypePtr registerEnum(const std::string &code,const EnumEntries &es);

	static PropertyTypesRegistry &getInstance();

private:
	PropertyTypesRegistry() =default;
	PropertyTypesRegistry& operator=(const PropertyTypesRegistry &) =delete;

	template<typename T> friend class RegularPropertyTypeRegistrar;

	std::vector<IPropertyTypePtr> _regularTypes;
	typedef boost::unordered_map<std::string,IPropertyTypePtr> EnumMap;
	EnumMap _enumTypes;
};


template<typename T>
struct RegularPropertyTypeRegistrar
{
	RegularPropertyTypeRegistrar()
	{
		PropertyTypesRegistry::getInstance()._regularTypes.push_back(std::make_shared<T>());
	}
};

#define G2F_REGISTER_REGULAR_TYPE(T) RegularPropertyTypeRegistrar<T> _registrar##T

G2F_DECLARE_PTR(PropertyTypesRegistry);
