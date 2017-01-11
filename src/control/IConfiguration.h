#pragma once

#include <string>
#include <boost/optional.hpp>
#include "utils/decls.h"
#include "error/G2FException.h"
#include "props/IPropertiesList.h"
#include "paths/IPathManager.h"

class IConfiguration
{

public:

	virtual IPathManagerPtr getPaths() =0;
	virtual boost::optional<std::string> getProperty(const std::string &name) =0;
	virtual G2FError setProperty(const std::string &name,const std::string &value) =0;
	virtual IPropertiesListPtr getProperiesList() =0;

	virtual ~IConfiguration() {}
};

G2F_DECLARE_PTR(IConfiguration);
