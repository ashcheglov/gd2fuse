#ifndef CONTENTMANAGER_H
#define CONTENTMANAGER_H

#include "utils/decls.h"
#include "error/appError.h"

class ContentManager
{
public:
	class IContentSource
	{
	public:
		class IDataReader
		{
		public:
			virtual bool done() =0;
			virtual int64_t read(char *buffer,int64_t bufSize) =0;
			virtual G2FError error() =0;
			virtual ~IDataReader() {}
		};
		G2F_DECLARE_PTR(IDataReader);

		virtual IDataReader *contentReader(const std::string &id) =0;
		virtual ~IContentSource() {}
	};
	G2F_DECLARE_PTR(IContentSource);

	void init(const fs::path &workDir,IContentSource *cSource);
	uint64_t openFile(const std::string &id, int flags);
	int closeFile(uint64_t fd);
	int readContent(uint64_t fd,char *buf, size_t len, off_t offset);

private:
	fs::path _workDir;
	IContentSource *_cSource=nullptr;
};

#endif // CONTENTMANAGER_H
