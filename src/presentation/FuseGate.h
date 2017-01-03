#pragma once

#include "utils/decls.h"
#include "control/FuseOpts.h"
#include "data/Data.h"
#include "cache/Cache.h"
#include "data/ContentManager.h"
#include <googleapis/client/transport/http_authorization.h>
#include <google/drive_api/drive_service.h>
#include "error/appError.h"
//#include <boost/utility/string_ref.hpp>

G2F_GOOGLE_NS_SHORTHANDS
namespace g_drv=google_drive_api;

class GoogleSource;

class FuseGate
{
public:
	FuseGate(g_drv::DriveService &service,g_cli::AuthorizationCredential *authCred,const fs::path& workDir);
	int run(const FUSEOpts &fuseOpts);

	Node *getMeta(const char *path);
	Tree& metaTree();
	Tree::IDirectoryIteratorUPtr getDirectoryIterator(Node *meta);

	static int fuseHelp();

	int openContent(Node *meta, int flags, uint64_t &fd);
	int readContent(uint64_t fd,char *buf, size_t len, off_t offset);
	int closeContent(uint64_t fd);

private:
	g_drv::DriveService &_service;
	g_cli::AuthorizationCredential *_authCred;
	fs::path _workDir;

	Tree _tree;
	ContentManager _cManager;
	sptr<GoogleSource> _googleSource;
};

