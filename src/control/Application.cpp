#include "Application.h"
#include "error/G2FException.h"
#include "providers/google/Auth.h"
#include "presentation/FuseGate.h"
#include <googleapis/client/util/status.h>
#include <googleapis/client/transport/curl_http_transport.h>
#include <googleapis/client/transport/http_request_batch.h>
#include <googleapis/client/util/status.h>
//#include <googleapis/strings/strcat.h>
#include <googleapis/client/transport/http_authorization.h>
#include <unistd.h>
#include <sys/types.h>

Application::Application(const std::string &email, int argc, char *argv[])
	: _email(email),
	  _argc(argc),
	  _argv(argv)
{
	if(_email.empty())
		G2F_EXCEPTION("e-mail id hasn't set").throwIt(G2FErrorCodes::WrongAppArguments);
}

Application &Application::setDebug(bool value)
{
	_debug=value;
	return *this;
}

Application &Application::setReadOnly(bool value)
{
	_readOnly=value;
	return *this;
}

Application &Application::setSSLVerificationDisabled(bool value)
{
	_disableSSLverify=value;
}

Application &Application::setPathManager(const IPathManagerPtr &p)
{
	_pm=p;
}

Application &Application::setSSLCApath(const boost::filesystem::path &p)
{
	_SSLCApath=p;
}

IPathManager& Application::pathManager()
{
	if(!_pm)
		_pm=createDefaultPathMapping();
	return *_pm;
}

int Application::fuseStart(const FUSEOpts &opts)
{
	Auth a(transport(),pathManager().secretFile());
	a.setErrorCallback();
	OAuth2CredentialPtr oa2c=a.auth(_email,pathManager().credentialHomeDir());

	FuseGate drive(driveService(),oa2c.get(),pathManager().cacheDir(_email));
	return drive.run(opts);
}

int Application::fuseHelp()
{
	FuseGate::fuseHelp();
	return 0;
}

int Application::createAuthFile(const std::string& authCode)
{
	Auth a(transport(),pathManager().secretFile());
	a.setShellCallback(authCode);
	a.auth(_email,pathManager().credentialHomeDir());
	return 0;
}

uid_t Application::getUID()
{
	return getuid();
}

gid_t Application::getGID()
{
	return getgid();
}

namespace
{
	uptr<Application> theApp;
}

Application *Application::create(const std::string &email, int argc, char *argv[])
{
	theApp.reset(new Application(email,argc,argv));
	return theApp.get();
}

Application *Application::instance()
{
	return theApp.get();
}

g_drv::DriveService &Application::driveService()
{
	if(!_service)
	{
		g_utl::Status status;
		uptr<g_cli::HttpTransport> t(transportConfig().NewDefaultTransport(&status));
		if (!status.ok())
			throw G2F_EXCEPTION("Error creating HTTP transport").arg(status.error_code()).arg(status.ToString());

		_service.reset(new g_drv::DriveService(t.release()));
	}
	return *_service;
}

g_cli::HttpTransport *Application::transport()
{
	g_utl::Status status;
	uptr<g_cli::HttpTransport> t(transportConfig().NewDefaultTransport(&status));
	if (!status.ok())
		throw G2F_EXCEPTION("Error creating HTTP transport").arg(status.error_code()).arg(status.ToString());
	return t.release();
}

g_cli::HttpTransportLayerConfig &Application::transportConfig()
{
	if(!_config)
	{
		_config.reset(new g_cli::HttpTransportLayerConfig);
		g_cli::HttpTransportFactory* factory =
				new g_cli::CurlHttpTransportFactory(_config.get());
		_config->ResetDefaultTransportFactory(factory);
		g_cli::HttpTransportOptions* opts=_config->mutable_default_transport_options();
		if(_disableSSLverify)
			opts->set_cacerts_path(g_cli::HttpTransportOptions::kDisableSslVerification);
		else
		if(!_SSLCApath.empty())
			opts->set_cacerts_path(_SSLCApath.string());
	}
	return *_config;
}



