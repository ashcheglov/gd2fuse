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

	INode *getINode(const char *path);
	INode *createFile(const char *fileName, int flags);
	IFileSystem& getFS();

	static int fuseHelp();

private:
	IProviderSession &_ps;
	IFileSystemPtr _fs;
};

