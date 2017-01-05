#pragma once

#include <string>
#include <vector>
#include "utils/decls.h"
#include "IProvider.h"


class ProvidersRegistry
{

public:
	typedef std::vector<std::string> ProviderNames;

	ProviderNames getAvailableProviders() const;
	IProviderPtr find(const std::string &name) const;
	IProviderPtr detectByAccountName(const std::string &accName) const;
	static ProvidersRegistry& getInstance();

private:
	ProvidersRegistry() =default;
	ProvidersRegistry(const ProvidersRegistry&) =delete;
	ProvidersRegistry& operator=(const ProvidersRegistry&) =delete;

	// unordered_map? May be later...
	std::vector<IProviderPtr> _providersList;

	template<typename T> friend class ProviderRegistrar;
};

G2F_DECLARE_PTR(ProvidersRegistry);


// Autoregistrar (convinience)
template<typename T>
struct ProviderRegistrar
{
	ProviderRegistrar(ProvidersRegistry & reg)
	{
		reg._providersList.push_back(std::make_shared<T>());
	}
};
