#pragma once

#include <string>
#include <vector>
#include "utils/decls.h"
#include "IConvertFormat.h"

class IConversionDescription
{

public:
	virtual std::string getSource() =0;
	virtual std::vector<IConvertFormatPtr> getSupportedDestinations() =0;

	virtual ~IConversionDescription(){}

};

G2F_DECLARE_PTR(IConversionDescription);
