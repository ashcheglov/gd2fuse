#pragma once

#include "utils/decls.h"
#include "error/G2FException.h"

class IDataReader
{

public:

	virtual bool done() =0;
	virtual int64_t read(char* buffer, int64_t bufSize) =0;
	virtual G2FError error() =0;

	virtual ~IDataReader() {}
};
G2F_DECLARE_PTR(IDataReader);
