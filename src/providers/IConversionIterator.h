#pragma once

#include "utils/decls.h"
#include "IConversionDescription.h"

class IConversionIterator
{

public:
	virtual bool hasNext() =0;
	virtual IConversionDescriptionPtr next() =0;

	virtual ~IConversionIterator() {}

};

G2F_DECLARE_PTR(IConversionIterator);

