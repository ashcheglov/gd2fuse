#include "PropertyTypesRegistry.h"
#include "PropertyTypeEnum.h"

IPropertyTypePtr PropertyTypesRegistry::getType(IPropertyType::Type type, const std::string &enumCode)
{
	auto it=std::find_if(_regularTypes.begin(),_regularTypes.end(),[&](auto e){return e->getType()==type;});
	if(it!=_regularTypes.end())
		return *it;
	return IPropertyTypePtr();
}

IPropertyTypePtr PropertyTypesRegistry::registerEnum(const std::string &code, const PropertyTypesRegistry::EnumEntries &es)
{
	auto it=_enumTypes.find(code);
	if(it!=_enumTypes.end())
		return it->second;
	IPropertyTypePtr ret=std::make_shared<PropertyTypeEnum>(es);
	_enumTypes.insert(std::make_pair(code,ret));
	return ret;
}

// static
PropertyTypesRegistry &PropertyTypesRegistry::getInstance()
{
	static PropertyTypesRegistry registry;
	return registry;
}
