#pragma once

#include "utils/decls.h"
#include <boost/filesystem.hpp>

namespace fs=boost::filesystem;

class IPathManager
{
public:
	enum Type
	{
		CONFIG,
		DATA,
		CACHE,
		RUNTIME
	};

	virtual fs::path getDir(Type type) =0;
	virtual bool isExists(Type type) =0;
	virtual bool create(Type type) =0;
	virtual void setAutocreation(bool ac) =0;

	virtual ~IPathManager(){}
};

G2F_DECLARE_PTR(IPathManager);
