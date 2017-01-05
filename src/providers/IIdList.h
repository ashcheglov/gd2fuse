#pragma once

#include <string>
#include "utils/decls.h"

class IIdList
{

public:

	virtual bool hasNext() =0;
	virtual std::string next() =0;

	virtual ~IIdList() {}
};

G2F_DECLARE_PTR(IIdList);
