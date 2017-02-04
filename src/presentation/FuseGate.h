#pragma once

#include "utils/decls.h"
#include "control/FuseOpts.h"
#include "fs/IFileSystem.h"
#include "cache/Cache.h"
#include "providers/IProviderSession.h"


class GoogleSource;

class FuseGate
{
public:
	FuseGate(IProviderSession &ps);
	int run(const FUSEOpts &fuseOpts);

	IFileSystem& getFS();

	INode *getINode(const char *path);
	posix_error_code openContent(const char *path,int flags, IContentHandle *&outChn);
	posix_error_code createFile(const char *fileName, INode *&f);
	posix_error_code removeINode(const char *path);
	posix_error_code createDir(const char *dirName,mode_t mode,INode *&f);
	posix_error_code rename(const char *oldName,const char *newName);

	static int fuseHelp();

private:
	IProviderSession &_ps;
	IFileSystemPtr _fs;
};

