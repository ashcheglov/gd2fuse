#pragma once

#include "utils/decls.h"
#include "control/FuseOpts.h"
#include "data/Data.h"
#include "cache/Cache.h"
#include "data/ContentManager.h"
#include "providers/IProviderSession.h"
//#include <boost/utility/string_ref.hpp>


class GoogleSource;

class FuseGate
{
public:
	FuseGate(IProviderSession &ps,const fs::path& workDir);
	int run(const FUSEOpts &fuseOpts);

	Node *getMeta(const char *path);
	Tree& metaTree();
	Tree::IDirectoryIteratorUPtr getDirectoryIterator(Node *meta);

	static int fuseHelp();

	int openContent(Node *meta, int flags, uint64_t &fd);
	int readContent(uint64_t fd,char *buf, size_t len, off_t offset);
	int closeContent(uint64_t fd);

private:
	IProviderSession &_ps;
	fs::path _workDir;

	Tree _tree;
	ContentManager _cManager;
};

