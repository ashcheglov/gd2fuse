#pragma once

#include "utils/decls.h"

class INode;

class IContentHandle
{

public:
	virtual INode* getMeta() =0;

	// Calls to new files
	virtual void fillAttr(struct stat &statbuf);
	virtual bool useDirectIO();
	virtual posix_error_code flush() =0;
	virtual posix_error_code getError() =0;
	virtual int read(char *buf, size_t len, off_t offset) =0;
	virtual posix_error_code close() =0;
	virtual ~IContentHandle() {}
};

G2F_DECLARE_PTR(IContentHandle);
