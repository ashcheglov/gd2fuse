#pragma once

#include "utils/decls.h"
#include "AbstractStaticInitPropertiesList.h"

/*
 * Initialisation from static data structures.
 *
 * ***********************************************************/
class AbstractMemoryStaticInitPropertiesList : public AbstractStaticInitPropertiesList
{
protected:
	virtual IPropertyDefinitionPtr decorate(const IPropertyDefinitionPtr &p);
};
