#pragma once

#include "utils/decls.h"
#include "IOAuth2Process.h"
#include "IDataProvider.h"

class IProvider;

class IProviderSession
{

public:

	virtual IDataProviderPtr createDataProvider() =0;
	virtual IOAuth2ProcessPtr createOAuth2Process() =0;
	virtual const IProvider* getProvider() =0;
	virtual bool isAuthorized() =0;

	virtual ~IProviderSession() {}
};

G2F_DECLARE_PTR(IProviderSession);

