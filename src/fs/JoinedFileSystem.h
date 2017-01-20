#pragma once

#include "IFileSystem.h"
#include <tuple>

class JoinedFileSystemFactory
{
public:
	JoinedFileSystemFactory(int mode);
	bool mount(const boost::filesystem::path &mountPoint, const IFileSystemPtr &fs, int mode);
	IFileSystemPtr build() const;

private:
	int _mode=0;
	std::vector<std::tuple<fs::path,int,IFileSystemPtr>> _mounts;
};
