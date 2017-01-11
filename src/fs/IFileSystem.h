#pragma once

#include "utils/decls.h"
#include "INode.h"

class IFileSystem
{

public:
	G2F_DECLARE_PTR(IFileSystem);

	virtual INode* getRoot() =0;
	virtual INode* get(const fs::path &p) =0;
	virtual bool mount(const fs::path &mountPoint,IFileSystemPtr fs) =0;

	virtual ~IFileSystem() {}
};

typedef IFileSystem::IFileSystemPtr IFileSystemPtr;
typedef IFileSystem::IFileSystemUPtr IFileSystemUPtr;
