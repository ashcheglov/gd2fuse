#pragma once

#include <string>
#include "utils/decls.h"
#include "IDataReader.h"
#include "IConvertFormat.h"
#include "IIdList.h"
#include "IMetaWrapper.h"

class IDataProvider
{

public:

	virtual IDataReaderPtr createContentReader(const std::string &id, IConvertFormatPtr targetFormat) =0;
	virtual IIdListPtr fetchList(const std::string &id) =0;
	virtual void fetchMeta(const std::string &id, IMetaWrapper &dest) =0;

	virtual ~IDataProvider() {}
};

G2F_DECLARE_PTR(IDataProvider);
