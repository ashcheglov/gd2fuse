#pragma once

#include "IPathManager.h"

/*
 * Abstract class to ease implementation IPathManager interface
 * You should implement only 'readDirNames' method to get full
 * implementation
 * */

class AbstractPathManager : public IPathManager
{
public:
	virtual fs::path getDir(Type type);
	virtual bool isExists(Type type);
	virtual bool create(Type type);
	virtual void setAutocreation(bool ac);

protected:
	virtual fs::path readDirName(Type type) =0;

private:
	bool _autoCreate=false;
	fs::path _dataPath;
	fs::path _confPath;
	fs::path _cachePath;
	fs::path _runtimePath;

};
