#pragma once

#include "IConfiguration.h"

class ChainedConfiguration : public IConfiguration
{

public:
	ChainedConfiguration(const IConfigurationPtr &upstream,const IConfigurationPtr &current);

	// IConfiguration interface
public:
	virtual IPathManagerPtr getPaths() override;
	virtual boost::optional<std::string> getProperty(const std::string &name) override;
	virtual G2FError setProperty(const std::string &name, const std::string &value) override;
	virtual IPropertiesListPtr getProperiesList() override;

private:
	IConfigurationPtr _upstream;
	IConfigurationPtr _current;
};
