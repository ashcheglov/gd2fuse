#include "Application.h"
#include "presentation/FuseGate.h"
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include "Configuration.h"

Application::Application(const std::string &email, int argc, char *argv[], const fs::path &manualConf)
	: _email(email),
	  _argc(argc),
	  _argv(argv)
{
	_conf=createGlobalConfiguration(manualConf);
	if(_email.empty())
		G2F_EXCEPTION("e-mail id hasn't set").throwIt(G2FErrorCodes::WrongAppArguments);
	const auto& factory=ProvidersRegistry::getInstance().detectByAccountName(_email);
	_provider=factory->create(_conf);
	if(!_provider)
		G2F_EXCEPTION(_email).throwIt(G2FErrorCodes::CouldntDetermineProvider);
	clock_gettime(CLOCK_REALTIME,&_startTime);
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

Application &Application::setProviderArgs(const Application::ProviderArgs &prArgs)
{
	_prArgs=prArgs;
}

const IConfigurationPtr &Application::getConfiguration()
{
	return _conf;
}

int Application::fuseStart(const FUSEOpts &opts)
{
	const IProviderSessionPtr &sess=_provider->createSession(_email,_prArgs);
	FuseGate drive(*sess);
	return drive.run(opts);
}

int Application::fuseHelp()
{
	FuseGate::fuseHelp();
	return 0;
}

int Application::createAuthFile(const std::string& authCode)
{
	int ret=0;
	const IProviderSessionPtr &sess=_provider->createSession(_email,_prArgs);
	const IOAuth2ProcessPtr &o2p=sess->createOAuth2Process();
	if(authCode.empty())
		o2p->startNativeApp();
	else
		ret=o2p->finishNativeApp(authCode);
	return ret;
}

uid_t Application::getUID()
{
	return getuid();
}

gid_t Application::getGID()
{
	return getgid();
}

std::string Application::applicationName()
{
	return G2F_APP_NAME;
}

timespec Application::startTime()
{
	return this->_startTime;
}

namespace
{
	uptr<Application> theApp;
}

Application *Application::create(const std::string &email, int argc, char *argv[], const fs::path &manualConf)
{
	theApp.reset(new Application(email,argc,argv,manualConf));
	return theApp.get();
}

Application *Application::instance()
{
	return theApp.get();
}




