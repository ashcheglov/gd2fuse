#pragma once

#include <string>
#include <vector>
#include "utils/decls.h"
#include "IProvider.h"


/**
 * @brief Factory to producing providers instance
 */
class IProviderFactory
{
public:
	virtual std::string getName() =0;
	virtual IProviderPtr create(const IConfigurationPtr &globalConf) =0;
};
G2F_DECLARE_PTR(IProviderFactory);




/**
 * @brief Global Providers' factory registry
 */
class ProvidersRegistry
{

public:
	typedef std::vector<std::string> ProviderNames;

	ProviderNames getAvailableProviders() const;
	IProviderFactoryPtr find(const std::string &name) const;
	IProviderFactoryPtr detectByAccountName(const std::string &accName) const;
	static ProvidersRegistry& getInstance();

private:
	ProvidersRegistry() =default;
	ProvidersRegistry(const ProvidersRegistry&) =delete;
	ProvidersRegistry& operator=(const ProvidersRegistry&) =delete;

	// unordered_map? May be later...
	std::vector<IProviderFactoryPtr> _pfList;

	template<typename T> friend class ProviderRegistrar;
};

G2F_DECLARE_PTR(ProvidersRegistry);





// Autoregistrar (for convenience)
template<typename T>
struct ProviderRegistrar
{
	ProviderRegistrar(ProvidersRegistry & reg)
	{
		reg._pfList.push_back(std::make_shared<T>());
	}
};
