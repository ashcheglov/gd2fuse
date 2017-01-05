#include "Application.h"
#include "error/G2FException.h"
#include "presentation/FuseGate.h"
#include <unistd.h>
#include <sys/types.h>

Application::Application(const std::string &email, int argc, char *argv[])
	: _email(email),
	  _argc(argc),
	  _argv(argv)
{
	if(_email.empty())
		G2F_EXCEPTION("e-mail id hasn't set").throwIt(G2FErrorCodes::WrongAppArguments);
	_provider=ProvidersRegistry::getInstance().detectByAccountName(_email);
	if(!_provider)
		G2F_EXCEPTION(_email).throwIt(G2FErrorCodes::CouldntDetermineProvider);
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

Application &Application::setPathManager(const IPathManagerPtr &p)
{
	_pm=p;
}

IPathManager& Application::pathManager()
{
	if(!_pm)
		_pm=createDefaultPathMapping();
	return *_pm;
}

int Application::fuseStart(const FUSEOpts &opts)
{
	const IProviderSessionPtr &sess=_provider->createSession(_email,_prArgs);
	FuseGate drive(*sess,pathManager().cacheDir(_email));
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




