#include "ContentManager.h"
#include "error/G2FException.h"
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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



class AutocloseableHandler
{
public:
	AutocloseableHandler(int fd):_fd(fd)
	{}
	~AutocloseableHandler()
	{
		if(_fd!=-1)
			close(_fd);
	}
	AutocloseableHandler& operator=(int val)
	{
		_fd=val;
		return *this;
	}
	operator int() const
	{
		return _fd;
	}
private:
   int _fd=-1;
};


ContentManager::ContentManager(const boost::filesystem::path &workDir)
	: _workDir(workDir)
{}

bool ContentManager::is(const std::string &id)
{
	fs::path fileName=_workDir/id2fileName(id);
	return fs::exists(fileName);
}

int64_t ContentManager::openFile(const std::string &id,int flags)
{
	fs::path fileName=_workDir/id2fileName(id);
	//if(!(flags&O_CREAT) && !fs::exists(fileName))
	//	G2FExceptionBuilder("Media manager: attempt to open non existent file '%1'").arg(fileName).throwItSystem(ENOENT);

	int fileDesc=open(fileName.c_str(),flags,S_IRUSR|S_IWUSR);
	if(fileDesc==-1)
	{
		int err=errno;
		G2FExceptionBuilder("Media manager: can not open file '%1'").arg(fileName).throwItSystem(err);
	}
	return fileDesc;
}

void ContentManager::closeFile(int64_t fd)
{
	close(fd);
}

int ContentManager::readContent(int64_t fd, char *buf, size_t len, off_t offset)
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

int ContentManager::writeContent(int64_t fd, const char *buf, size_t len, off_t offset)
{
	int ret=pwrite(fd,buf,len,offset);
	if(ret<0)
	{
		int err=errno;
		G2FExceptionBuilder("Media manager: error writing content to open file").throwItSystem(err);
	}
	return ret;
}

void ContentManager::truncateFile(const std::string &id, size_t size, off_t newSize)
{
	fs::path fileName=_workDir/id2fileName(id);

	if(!fs::exists(fileName))
		G2FExceptionBuilder("Media manager: attempt to open non existent file '%1'").arg(fileName).throwItSystem(ENOENT);

	if(truncate(fileName.c_str(),newSize)<0)
	{
		int err=errno;
		G2FExceptionBuilder("Media manager: error truncate file '%1'").arg(fileName).throwItSystem(err);
	}
}

void ContentManager::createFile(const std::string &id,IReader* content)
{
	fs::path fileName=_workDir/id2fileName(id);
	if(!fs::exists(fileName.parent_path()))
		fs::create_directories(fileName.parent_path());

	AutocloseableHandler fd=creat(fileName.c_str(),S_IRUSR|S_IWUSR);
	if(fd<0)
	{
		int err=errno;
		G2FExceptionBuilder("Media manager: Error creating file '%1'").arg(fileName).throwItSystem(err);
	}
	if(content)
	{
		char buf[4096];

		while(!content->done())
		{
			int64_t readed=content->read(buf,sizeof(buf));
			int written=write(fd,buf,readed);
			if(written<0)
			{
				int err=errno;
				G2FExceptionBuilder("Media manager: Error writing content into file '%1'").arg(fileName).throwItSystem(err);
			}
		}
		G2FError e=content->error();
		if(e!=err::errc::success)
			G2FExceptionBuilder("Media manager: Error writing content into file '%1'").arg(fileName).throwIt(e);
	}
}

bool ContentManager::deleteFile(const std::string &id)
{
	fs::path fileName=_workDir/id2fileName(id);

	if(fs::exists(fileName))
	{
		G2FError e;
		fs::remove(fileName,e);
		if(e!=err::errc::success)
			G2FExceptionBuilder("Media manager: Error of deleting file '%1'").arg(fileName).throwIt(e);
		return true;
	}
	return false;
}


class ContentReader : public ContentManager::IReader
{
public:
	typedef std::unique_ptr<FILE,decltype(&fclose)> AutoFileHandle;

	ContentReader(AutoFileHandle &&fh)
		: _fh(std::move(fh))
	{}
	// IReader interface
public:
	virtual bool done() override
	{
		return feof(_fh.get())!=0;
	}
	virtual int64_t read(char *buffer, int64_t bufSize) override
	{
		int ret=fread(buffer,1,bufSize,_fh.get());
		_err=errno;
		return ret;
	}
	virtual G2FError error() override
	{
		return G2FError(0,err::generic_category());
	}

private:
	AutoFileHandle _fh;
	int _err=0;
};

ContentManager::IReaderPtr ContentManager::readContent(const std::string &id)
{
	fs::path fileName=_workDir/id2fileName(id);
	if(!fs::exists(fileName))
		G2FExceptionBuilder("Media manager: attempt to read content of non existent file '%1'").arg(fileName).throwItSystem(ENOENT);

	ContentReader::AutoFileHandle f(fopen(fileName.c_str(),"r"),fclose);
	if(!f)
	{
		int err=errno;
		G2FExceptionBuilder("Media manager: can not open file '%1' for reading").arg(fileName).throwItSystem(err);
	}
	return std::make_shared<ContentReader>(std::move(f));
}

/*
bool ContentManager::_fetchFile(const fs::path &fileName,const std::string &id)
{
	// TODO MD5 check
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
			G2FExceptionBuilder("Error saving file '%1' in cache").arg(fileName).throwIt(e);
		return true;
	}
	return false;
}
*/
