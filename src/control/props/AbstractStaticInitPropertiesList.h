#pragma once

#include "utils/decls.h"
#include "IPropertiesList.h"

/*
 * Initialisation from static data structures.
 *
 * ***********************************************************/
class AbstractStaticInitPropertiesList : public IPropertiesList
{
public:
	struct PropDefi
	{
		const char *name;
		IPropertyType::Type type;
		const char *enumCode;
		const char *defaultValue;
		bool changeRt;
		const char *description;
	};

	struct EnumValues
	{
		const char *entry;
		const char *description;
	};

	struct EnumDefi
	{
		const char *code;
		std::vector<EnumValues> entries;
	};

	AbstractStaticInitPropertiesList();

	// IPropertiesList interface
public:
	virtual IPropertyDefinitionPtr findProperty(const std::string &name) override;
	virtual bool isEmpty() override;
	virtual size_t getPropertiesSize() override;
	virtual IPropertyIteratorPtr getProperties() override;

	static std::string& normalizeValue(std::string& value);

protected:
	virtual std::pair<const PropDefi*,size_t> getPropertiesDefinition() =0;
	virtual std::pair<const EnumDefi*,size_t> getEnumsDefinition() =0;
	virtual IPropertyDefinitionPtr decorate(const IPropertyDefinitionPtr &p);

private:
	class Impl;
	sptr<Impl> _impl;
};
