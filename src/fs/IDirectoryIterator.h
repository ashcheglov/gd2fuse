#pragma once

#include "utils/decls.h"

class INode;

class IDirectoryIterator
{

public:

	virtual bool hasNext() =0;
	virtual INode* next() =0;

	virtual ~IDirectoryIterator() {}
};

G2F_DECLARE_PTR(IDirectoryIterator);
