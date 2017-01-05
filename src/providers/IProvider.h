#pragma once

#include <string>
#include "utils/decls.h"
#include "IProviderSession.h"
#include "ISupportedConversion.h"

class IProvider
{

public:
	typedef std::vector<std::string> PropList;

	static const int NEED_AUTHORIZATION= 1 << 0;

	virtual std::string getName() =0;
	virtual int getProperties() =0;
	// TODO Reporting about allowable properties
	virtual IProviderSessionPtr createSession(const std::string &accountId,const PropList &props) =0;
	virtual ISupportedConversionPtr getSupportedConversion() =0;

	virtual ~IProvider() {}
};

G2F_DECLARE_PTR(IProvider);

