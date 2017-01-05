#pragma once

#include <string>
#include "utils/decls.h"
#include "IConversionDescription.h"
#include "IConversionIterator.h"

class ISupportedConversion
{

public:

	virtual size_t getSize() =0;
	virtual IConversionDescriptionPtr find(const std::string &name) =0;
	virtual IConversionIteratorPtr getIterator() =0;

	virtual ~ISupportedConversion() {}
};

G2F_DECLARE_PTR(ISupportedConversion);
