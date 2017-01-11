#include "ProvidersRegistry.h"
#include <boost/algorithm/string.hpp>

ProvidersRegistry::ProviderNames ProvidersRegistry::getAvailableProviders() const
{
	ProvidersRegistry::ProviderNames ret;
	for(auto p : _pfList)
		ret.push_back(p->getName());
	return ret;
}

IProviderFactoryPtr ProvidersRegistry::find(const std::string &name) const
{
	std::string loName(name);

	boost::to_lower(loName);
	for(auto p : _pfList)
	{
		std::string pName=p->getName();
		boost::to_lower(pName);
		if(loName==pName)
			return p;
	}
	return IProviderFactoryPtr();
}

IProviderFactoryPtr ProvidersRegistry::detectByAccountName(const std::string &accName) const
{
	if(accName.find("gmail")!=std::string::npos)
		return find("gdrive");
	return IProviderFactoryPtr();
}

ProvidersRegistry &ProvidersRegistry::getInstance()
{
	static ProvidersRegistry reg;
	return reg;
}
