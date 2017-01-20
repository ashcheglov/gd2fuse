#pragma once

#include <string>
#include "utils/decls.h"
#include <sys/stat.h>
#include <time.h>
#include "utils/assets.h"
#include "IDirectoryIterator.h"
#include "IContentHandle.h"

class INode
{

public:

	virtual void setId(const std::string &id) =0;
	virtual bool isFolder() =0;
	virtual void fillAttr(struct stat& statbuf) =0;
	virtual std::string getId() =0;
	virtual void setName(const fs::path &name) =0;
	virtual fs::path getName() =0;
	virtual void setSize(size_t size) =0;
	virtual size_t getSize() =0;
	virtual void setTime(TimeAttrib what, timespec value) =0;
	virtual timespec getTime(TimeAttrib what) =0;
	virtual void setMD5(const MD5Signature &md5) =0;
	virtual MD5Signature getMD5() =0;
	virtual IDirectoryIteratorPtr getDirectoryIterator() =0;
	virtual IContentHandle* openContent(int flags) =0;
	virtual posix_error_code truncate(off_t newSize) =0;

	virtual ~INode() {}
};

G2F_DECLARE_PTR(INode);
