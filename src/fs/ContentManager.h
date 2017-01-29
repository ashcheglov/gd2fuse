#pragma once

#include "utils/decls.h"
#include "error/appError.h"

class ContentManager
{
public:
	class IReader
	{
	public:
		virtual bool done() =0;
		virtual int64_t read(char* buffer, int64_t bufSize) =0;
		virtual G2FError error() =0;

		virtual ~IReader() {}
	};
	G2F_DECLARE_PTR(IReader);

	ContentManager(const fs::path &workDir);

	bool is(const std::string &id);
	uint64_t openFile(const std::string &id, int flags);
	void closeFile(uint64_t fd);
	int readContent(uint64_t fd,char *buf, size_t len, off_t offset);
	int writeContent(uint64_t fd,const char *buf, size_t len, off_t offset);
	void truncateFile(const std::string &id, size_t size, off_t newSize);
	void createFile(const std::string &id,IReader* content);
	bool deleteFile(const std::string &id);
	IReaderPtr readContent(const std::string &id);

private:
	//bool _fetchFile(const fs::path &fileName,const std::string &id);

	fs::path _workDir;
};
G2F_DECLARE_PTR(ContentManager);

