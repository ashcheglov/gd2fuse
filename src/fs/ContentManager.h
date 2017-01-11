#pragma once

#include "utils/decls.h"
#include "error/appError.h"
#include "providers/IDataProvider.h"

class ContentManager
{
public:
	void init(const fs::path &workDir,const IDataProviderPtr &dp);
	uint64_t openFile(const std::string &id, int flags);
	int closeFile(uint64_t fd);
	int readContent(uint64_t fd,char *buf, size_t len, off_t offset);

private:
	fs::path _workDir;
	IDataProviderPtr _dp;
};

