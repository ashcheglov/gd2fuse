#include "AbstractMountFileSystem.h"

bool superpath(const fs::path &basic,fs::path::iterator &it, const fs::path::iterator &end)
{
	for(const auto& b : basic)
	{
		if(it==end || *it!=b)
			return false;
	}
	return true;
}

bool AbstractMountFileSystem::mount(const fs::path &mountPoint, IFileSystem::IFileSystemPtr fs)
{
	_mounts.push_back(std::make_pair(mountPoint,fs));
	return true;
}

INode *AbstractMountFileSystem::findInMounts(const fs::path &path)
{
	for(const auto &m : _mounts)
	{
		if(path==m.first)
			return m.second->getRoot();

		fs::path::iterator it=path.begin();
		if(superpath(m.first,it,path.end()))
		{
			fs::path p=fromPathIt(it,path.end());
			if(!p.is_absolute())
				p="/"/p;
			return m.second->get(p);
		}
	}
	return nullptr;
}
