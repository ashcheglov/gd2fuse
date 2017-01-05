#pragma once

#include <string>
#include "utils/decls.h"

class IConvertFormat
{

public:

	virtual std::string getName() =0;
	virtual std::string getDescription() =0;
	virtual std::string getFileExtension() =0;

	virtual ~IConvertFormat() {}
};

G2F_DECLARE_PTR(IConvertFormat);
