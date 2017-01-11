#pragma once

#include "utils/decls.h"

class INode;

class IContentHandle
{

public:
	virtual INode* getMeta() =0;
	virtual int getError() =0;
	virtual int read(char *buf, size_t len, off_t offset) =0;
	virtual int close() =0;
	virtual ~IContentHandle() {}
};

G2F_DECLARE_PTR(IContentHandle);
