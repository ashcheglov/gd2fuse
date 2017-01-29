#pragma once

#include "utils/decls.h"
#include "IOAuth2Process.h"
#include "fs/IFileSystem.h"
#include "control/IConfiguration.h"

class IProvider;

class IProviderSession
{

public:

	virtual IFileSystemPtr getFileSystem() =0;
	virtual IOAuth2ProcessPtr createOAuth2Process() =0;
	virtual IProvider *getProvider() =0;
	virtual bool isAuthorized() =0;
	virtual IConfigurationPtr getConfiguration() =0;

	virtual ~IProviderSession() {}
};

G2F_DECLARE_PTR(IProviderSession);

