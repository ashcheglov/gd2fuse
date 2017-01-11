#pragma once

#include "utils/decls.h"
#include "control/paths/IPathManager.h"
#include "IConfiguration.h"
#include "FuseOpts.h"
#include "providers/ProvidersRegistry.h"

class Application
{
public:

	typedef std::vector<std::string> ProviderArgs;

	Application& setDebug(bool value=true);
	Application& setReadOnly(bool value=true);
	Application& setProviderArgs(const ProviderArgs &prArgs);

	const IConfigurationPtr &getConfiguration();

	// Start fuse event cycle
	int fuseStart(const FUSEOpts& opts);

	// Print out fuse help
	static int fuseHelp();

	// Authenticate in Google, create file
	int createAuthFile(const std::string &authCode);

	static uid_t getUID();
	static gid_t getGID();
	static std::string applicationName();
	timespec startTime();

	static Application *create(const std::string &email,int argc, char *argv[], const fs::path &manualConf);
	static Application *instance();

private:
	Application(const std::string &email, int argc, char *argv[], const boost::filesystem::path &manualConf);

	int _argc;
	char **_argv;

	bool _debug=false;
	bool _readOnly=false;

	IConfigurationPtr _conf;
	IProviderPtr _provider;

	std::string _email;
	ProviderArgs _prArgs;
	timespec _startTime;
};

