#include "AbstractPathManager.h"
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include "error/G2FException.h"

namespace
{
	bool _createDir(IPathManager::Type type, const fs::path &path)
	{
		if(fs::exists(path))
			return false;
		bool res=fs::create_directories(path);
		if(type!=IPathManager::RUNTIME)
			return res;
		if(res && type==IPathManager::RUNTIME)
			fs::permissions(path,fs::owner_all);
		return res;
	}
}

fs::path AbstractPathManager::getDir(Type type)
{
	fs::path *p=nullptr;
	switch(type)
	{
	case IPathManager::CONFIG:
			p=&_confPath;
		break;
	case IPathManager::DATA:
		p=&_dataPath;
		break;
	case IPathManager::CACHE:
		p=&_cachePath;
		break;
	case IPathManager::RUNTIME:
		p=&_runtimePath;
		break;
	}
	if(p)
	{
		if(p->empty())
			*p=readDirName(type);
		if(_autoCreate)
			_createDir(type,*p);
		return *p;
	}
	G2F_EXCEPTION("Unknown directory type").throwIt(G2FErrorCodes::NotSupported);
	return fs::path();
}

bool AbstractPathManager::isExists(Type type)
{
	return fs::exists(getDir(type));
}

bool AbstractPathManager::create(Type type)
{
	const fs::path &p=getDir(type);
	return _createDir(type,p);
}

void AbstractPathManager::setAutocreation(bool ac)
{
	_autoCreate=ac;
}


