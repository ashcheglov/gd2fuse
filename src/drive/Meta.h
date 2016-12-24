#ifndef META_H
#define META_H

#include "utils/decls.h"
#include "utils/assets.h"
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <set>
//#include <boost/graph/adjacency_list.hpp>

class Cache;
class Node;

enum TimeAttrib
{
	AccessTime,
	ModificationTime,
	ChangeTime
};

enum class FileType
{
	Directory,
	GoogleDoc,
	Binary,
	Shortcut
};

class Tree
{
public:
	class IDirectoryIterator
	{
	public:
		virtual bool hasNext() =0;
		virtual Node* next() =0;
		virtual ~IDirectoryIterator() {}
	};
	G2F_DECLARE_PTR(IDirectoryIterator);

	class IMetaSource
	{
	public:
		class IFileWrapper
		{
		public:
			virtual void setFileType(FileType value) =0;
			virtual void setId(const std::string &id) =0;
			virtual void setFileName(const std::string &path) =0;
			virtual void setSize(size_t size) =0;
			virtual void setMD5(const MD5Signature& md5) =0;
			virtual void setTime(TimeAttrib what,const timespec &value) =0;
			virtual ~IFileWrapper() {}
		};
		G2F_DECLARE_PTR(IFileWrapper);

		virtual std::set<std::string> fetchList(const std::string &id) =0;
		virtual void fetchFile(const std::string &id,IFileWrapper& dest) =0;
		virtual ~IMetaSource() {}
	};
	G2F_DECLARE_PTR(IMetaSource);

	Tree();
	void setMetaCallback(IMetaSource *source);
	Node *root();

	Node *get(const fs::path &path);
	Node *find(fs::path::iterator &it,const fs::path::iterator &end);
	//Node *remove(const fs::path &path);
	//void move(const fs::path &from,const fs::path &to);

	const std::string& id(Node *node) const;
	void setName(Node *node,const fs::path &name);
	const fs::path& name(Node *node) const;
	bool isFolder(Node *node) const;
	void setSize(Node *node,size_t size);
	size_t size(Node *node) const;
	void fillAttr(Node *node,struct stat& statbuf) const;
	const MD5Signature& MD5(Node *node);
	void setMD5(Node *node,const MD5Signature& md5);

	void setTime(Node *node,TimeAttrib what,const timespec &value);
	const timespec &time(Node *node,TimeAttrib what) const;
	void clearTime(Node *node,TimeAttrib what);

	IDirectoryIterator *getDirectoryIterator(Node *dir);

private:
	void _fillDir(Node *dir);
	sptr<Node> _root;
	uptr<Cache> _cache;
	IMetaSource *_metaSource=0;
};
G2F_DECLARE_PTR(Tree);

#endif // META_H
