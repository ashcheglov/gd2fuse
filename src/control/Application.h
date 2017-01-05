#pragma once

#include "utils/decls.h"
#include "utils/PathManager.h"
#include "FuseOpts.h"
#include "providers/ProvidersRegistry.h"

class Application
{
public:

	typedef std::vector<std::string> ProviderArgs;

	Application& setDebug(bool value=true);
	Application& setReadOnly(bool value=true);
	Application& setProviderArgs(const ProviderArgs &prArgs);
	Application& setPathManager(const IPathManagerPtr &p);

	IPathManager &pathManager();

	// Start fuse event cycle
	int fuseStart(const FUSEOpts& opts);

	// Print out fuse help
	static int fuseHelp();

	// Authenticate in Google, create file
	int createAuthFile(const std::string &authCode);

	static uid_t getUID();
	static gid_t getGID();

	static Application *create(const std::string &email,int argc, char *argv[]);
	static Application *instance();

private:
	Application(const std::string &email,int argc, char *argv[]);

	int _argc;
	char **_argv;

	bool _debug=false;
	bool _readOnly=false;

	IPathManagerPtr _pm;
	std::string _email;
	IProviderPtr _provider;
	ProviderArgs _prArgs;

};

