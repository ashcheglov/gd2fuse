#pragma once

#include "utils/decls.h"
#include "utils/assets.h"
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <set>
#include "providers/IMetaWrapper.h"
#include "providers/IDataProvider.h"

class Cache;
class Node;


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

	Tree();
	void setDataProvider(const IDataProviderPtr &provider);
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

	void setTime(Node *node,IMetaWrapper::TimeAttrib what,const timespec &value);
	const timespec &time(Node *node,IMetaWrapper::TimeAttrib what) const;
	void clearTime(Node *node,IMetaWrapper::TimeAttrib what);

	IDirectoryIterator *getDirectoryIterator(Node *dir);

private:
	void _fillDir(Node *dir);
	sptr<Node> _root;
	uptr<Cache> _cache;
	IDataProviderPtr _provider;
};
G2F_DECLARE_PTR(Tree);
