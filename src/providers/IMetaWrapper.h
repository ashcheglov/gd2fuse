#pragma once

#include <string>
#include <time.h>
#include "utils/decls.h"
#include "utils/assets.h"
#include "IConvertFormat.h"


class IMetaWrapper
{

public:
	enum class FileType
	{
		Directory,
		Exported,
		Binary,
		Shortcut
	};

	virtual void setFileType(FileType value) =0;
	virtual void setId(const std::string &id) =0;
	virtual void setFileName(const std::string &id) =0;
	virtual void setSize(size_t size) =0;
	virtual void setMD5(const MD5Signature &value) =0;
	virtual void setTime(TimeAttrib what,const timespec &value) =0;
	virtual void setMime(const std::string &value) =0;
	virtual void isEditable(bool value) =0;
	virtual void setSupportedConversions(const IConvertFormatPtr &convs) =0;

	virtual ~IMetaWrapper() {}
};

G2F_DECLARE_PTR(IMetaWrapper);
