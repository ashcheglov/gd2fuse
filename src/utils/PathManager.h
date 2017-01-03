#pragma once

#include "decls.h"
#include <boost/filesystem.hpp>

namespace fs=boost::filesystem;

// TODO Implements XDG Base Directory Specification (see https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)
class IPathManager
{
public:
	virtual fs::path rootPath() =0;
	virtual fs::path secretFile() =0;
	virtual fs::path credentialHomeDir(const std::string &id=std::string()) =0;
	virtual fs::path cacheDir(const std::string &id) =0;

	virtual ~IPathManager(){}
};

G2F_DECLARE_PTR(IPathManager);

IPathManagerPtr createDefaultPathMapping(const fs::path *rootDir=0);
