#include "ContentManager.h"
#include "error/G2FException.h"
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace
{
	fs::path id2fileName(const std::string &id)
	{
		if(id.empty())
			return fs::path();

		fs::path ret;
		// split up to 2 level subdirs
		if(id.size()>2)
		{
			ret=id.substr(0,1);
			ret/=id.substr(1,1);
			ret/=id.substr(2);
		}
		else
			ret=id;
		return ret;
	}
}


void ContentManager::init(const boost::filesystem::path &workDir, const IDataProviderPtr &dp)
{
	_workDir=workDir;
	_dp=dp;
}

uint64_t _open(const fs::path &path,int flags)
{
	int fileDesc=open(path.c_str(),flags);
	if(fileDesc==-1)
		throw err::system_error(errno,err::system_category(),path.string());
	return fileDesc;
}

uint64_t ContentManager::openFile(const std::string &id,int flags)
{
	// TODO MD5 check
	fs::path fileName=_workDir/id2fileName(id);
	if(fs::exists(fileName))		// throws boost::filesystem_error
		return _open(fileName,flags);

	IDataReaderPtr reader=_dp->createContentReader(id,IConvertFormatPtr());
	if(!reader->done())
	{
		fs::create_directories(fileName.parent_path());
		std::unique_ptr<FILE,int(&)(FILE*)> f(fopen(fileName.c_str(),"w"),fclose);
		if(!f)
			throw err::system_error(errno,err::system_category(),fileName.string());

		std::array<char,1024> buf;
		while(!reader->done())
		{
			int64_t rdd=reader->read(buf.data(),buf.size());
			if(rdd)
				fwrite(buf.data(),rdd,1,f.get());
		}
		const G2FError &e=reader->error();
		if(e)
			G2FExceptionBuilder().throwIt(e);
		return _open(fileName,flags);
	}
	return 0;
}

int ContentManager::closeFile(uint64_t fd)
{
	return close(fd);
}

int ContentManager::readContent(uint64_t fd,char *buf, size_t len, off_t offset)
{
	//return pread(fd,buf,len,offset);
	ssize_t ret=0,readed=0;
	while (len!=0 && (readed=pread(fd,buf,len,offset))!=0)
	{
		if(readed==-1)
		{
			if(errno==EINTR)
				continue;
			break;
		}
		offset += readed;
		len -= readed;
		buf += readed;
		ret += readed;
	}
	return ret;
}
