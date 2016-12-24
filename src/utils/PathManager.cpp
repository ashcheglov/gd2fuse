#include "PathManager.h"
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include "error/G2FException.h"

// TODO Should create Manager by fabric because some impl. detail depends from impl. Google Client API
class PathManager : public IPathManager
{
public:
	PathManager(const fs::path &rootPath)
		: _rootPath(rootPath)
	{}

	// IPathManager interface
	fs::path rootPath()
	{
		return _rootPath;
	}

	fs::path secretFile()
	{
		return _rootPath/ G2F_APP_NAME "_secret.json";
	}

	fs::path credentialHomeDir(const std::string &)
	{
		// Google Client API creates account subfolders itself
		return _rootPath/"auth.info";
	}

	fs::path cacheDir(const std::string &id)
	{
		fs::path ret(_rootPath/"cache");
		if(!id.empty())
			ret/=id;
		return ret;
	}

private:
	fs::path _rootPath;
};

IPathManagerPtr createDefaultPathMapping(const fs::path *rootDir)
{
	fs::path p;
	if(!rootDir || rootDir->empty())
	{
		const char* home=getenv("XDG_CONFIG_HOME");
		if(!home)
			home=getenv("HOME");
		if(!home)
			home=getpwuid(getuid())->pw_dir;
		if(!home)
			throw G2F_EXCEPTION("Cannot determine HOME directory");

		p=home;
		p/="." G2F_APP_NAME;
	}
	else
		p=*rootDir;
	return std::make_shared<PathManager>(p);
}
