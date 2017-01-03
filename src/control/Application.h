#pragma once

#include "utils/decls.h"
#include "utils/PathManager.h"
#include "FuseOpts.h"
#include <googleapis/client/transport/http_transport.h>
#include <google/drive_api/drive_service.h>

G2F_GOOGLE_NS_SHORTHANDS
namespace g_drv=google_drive_api;

class Application
{
public:

	Application& setDebug(bool value=true);
	Application& setReadOnly(bool value=true);
	Application& setSSLVerificationDisabled(bool value=true);
	Application& setPathManager(const IPathManagerPtr &p);
	Application& setSSLCApath(const fs::path& p);

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
	bool _disableSSLverify=false;

	IPathManagerPtr _pm;
	fs::path _SSLCApath;	// path to the SSL certificate authority validation data
	std::string _email;

	g_drv::DriveService& driveService();
	g_cli::HttpTransportLayerConfig& transportConfig();
	g_cli::HttpTransport *transport();

	uptr<g_drv::DriveService> _service;
	uptr<g_cli::HttpTransportLayerConfig> _config;

};

