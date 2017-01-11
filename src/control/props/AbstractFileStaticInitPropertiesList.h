#pragma once

#include "utils/decls.h"
#include "AbstractStaticInitPropertiesList.h"

/*
 * Initialisation from static data structures.
 *
 * ***********************************************************/
class AbstractFileStaticInitPropertiesList : public AbstractStaticInitPropertiesList
{
public:
	virtual fs::path getPropertiesFileName() =0;

protected:
	virtual IPropertyDefinitionPtr decorate(const IPropertyDefinitionPtr &p);
};
