#pragma once

#include "IFileSystem.h"

class AbstractMountFileSystem : public IFileSystem
{


	// IFileSystem interface
public:
	virtual bool mount(const boost::filesystem::path &mountPoint, IFileSystemPtr fs) override;
	INode *findInMounts(const fs::path &path);

private:
	std::vector<std::pair<fs::path,IFileSystemPtr>> _mounts;
};
