#include "AbstractPathManager.h"
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include "error/G2FException.h"


namespace
{

	fs::path _readHomeDir()
	{
		const char* home=getenv("HOME");
		if(!home)
			home=getpwuid(getuid())->pw_dir;
		if(!home)
			G2F_EXCEPTION("Can't determine 'HOME' directory").throwIt(G2FErrorCodes::UnknownEnvVariable);
		return home;
	}
}

/*
Notes about type dirs:
- If $XDG_CACHE_HOME is either not set or empty, a default equal to $HOME/.cache should be used.
- If $XDG_CONFIG_HOME is either not set or empty, a default equal to $HOME/.config should be used.
- f $XDG_DATA_HOME is either not set or empty, a default equal to $HOME/.local/share should be used.
- $XDG_RUNTIME_DIR defines the base directory relative to which user-specific non-essential runtime files
  and other file objects (such as sockets, named pipes, ...) should be stored. The directory MUST be owned
  by the user, and he MUST be the only one having read and write access to it. Its Unix access mode MUST be 0700.
  The lifetime of the directory MUST be bound to the user being logged in. It MUST be created when the user
  first logs in and if the user fully logs out the directory MUST be removed. If the user logs in more than
  once he should get pointed to the same directory, and it is mandatory that the directory continues to exist
  from his first login to his last logout on the system, and not removed in between. Files in the directory
  MUST not survive reboot or a full logout/login cycle.
  The directory MUST be on a local file system and not shared with any other system. The directory MUST by
  fully-featured by the standards of the operating system. More specifically, on Unix-like operating systems
  AF_UNIX sockets, symbolic links, hard links, proper permissions, file locking, sparse files, memory mapping,
  file change notifications, a reliable hard link count must be supported, and no restrictions on the file name
  character set should be imposed. Files in this directory MAY be subjected to periodic clean-up. To ensure that
  your files are not removed, they should have their access time timestamp modified at least once every 6 hours
  of monotonic time or the 'sticky' bit should be set on the file.
  If $XDG_RUNTIME_DIR is not set applications should fall back to a replacement directory with similar capabilities
  and print a warning message. Applications should use this directory for communication and synchronization purposes
  and should not place larger files in it, since it might reside in runtime memory and cannot necessarily be swapped
  out to disk.
*/

class XDGPathManager : public AbstractPathManager
{
protected:
	virtual fs::path readDirName(Type type)
	{
		fs::path ret;
		std::string tag;
		switch(type)
		{
		case IPathManager::CONFIG:
			{
				tag="CONFIG";
				const char* p=getenv("XDG_CONFIG_HOME");
				if(p)
					ret=p;
				else
					ret=_readHomeDir()/".config";
			}
			break;
		case IPathManager::DATA:
			{
				tag="DATA";
				const char* p=getenv("XDG_DATA_HOME");
				if(p)
					ret=p;
				else
					ret=_readHomeDir()/".local/share";
			}
			break;
		case IPathManager::CACHE:
			{
				tag="CACHE";
				const char* p=getenv("XDG_CACHE_HOME");
				if(p)
					ret=p;
				else
					ret=_readHomeDir()/".cache";
			}
			break;
		case IPathManager::RUNTIME:
			{
				tag="RUNTIME";
				const char* p=getenv("XDG_RUNTIME_HOME");
				if(!p)
					G2F_EXCEPTION("Can't read '%1' directory").arg("XDG_RUNTIME_HOME").throwIt(G2FErrorCodes::UnknownEnvVariable);
				ret=p;
			}
			break;
		}

		if(!ret.is_absolute())
			G2F_EXCEPTION("Path to '%1' directory '%2' must be absolute!").arg(tag).arg(ret).throwIt(G2FErrorCodes::PathError);

		return ret/G2F_APP_NAME;
	}
};







class SinglePointPathManager : public AbstractPathManager
{
public:
	SinglePointPathManager(const fs::path &root)
		: _root(root)
	{}

protected:
	virtual fs::path readDirName(Type type)
	{
		fs::path ret(_root);

		switch(type)
		{
		case IPathManager::CONFIG:
			{
				ret/="config";
			}
			break;
		case IPathManager::DATA:
			{
				ret/="data";
			}
			break;
		case IPathManager::CACHE:
			{
				ret/="cache";
			}
			break;
		case IPathManager::RUNTIME:
			{
				ret/="runtime";
			}
			break;
		}

		return ret;
	}
private:
	fs::path _root;
};




class NextLevelPathManager : public AbstractPathManager
{
public:
	NextLevelPathManager(const IPathManagerPtr &parent, const fs::path &nextLevel)
		: _parent(parent),
		  _nextLevel(nextLevel)
	{}

protected:
	virtual fs::path readDirName(Type type)
	{
		return _parent->getDir(type)/_nextLevel;
	}
private:
	IPathManagerPtr _parent;
	fs::path _nextLevel;
};



IPathManagerPtr createXDGPathManager()
{
	return std::make_shared<XDGPathManager>();
}

IPathManagerPtr createSinglePointPathManager(const fs::path &root)
{
	assert(!root.empty());
	return std::make_shared<SinglePointPathManager>(root);
}

IPathManagerPtr createNextLevelWrapperPathManager(const IPathManagerPtr &parent, const fs::path &nextLevel)
{
	return std::make_shared<NextLevelPathManager>(parent,nextLevel);
}
