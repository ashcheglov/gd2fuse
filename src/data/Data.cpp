#include "Data.h"
#include "utils/log.h"
#include "control/Application.h"
#include "cache/Cache.h"
#include <boost/ptr_container/ptr_list.hpp>


namespace
{
	const fs::path ROOT_PATH("/");
}

class Node
{
public:
	friend class Tree;

	typedef boost::ptr_list<Node> NodeList;
	typedef NodeList::iterator iterator;
	typedef NodeList::const_iterator const_iterator;


	// Simple model: regular tree. Today we don't support links
	Node(Node *parent)
		: _parent(parent)
	{
	}
	void setId(const std::string& id)
	{
		_id=id;
	}

	const std::string& id() const
	{
		return _id;
	}
	void setName(const fs::path &name)
	{
		_name=name;
	}
	const fs::path& name() const
	{
		return _name;
	}
	void setFileType(IMetaWrapper::FileType type)
	{
		_fileType=type;
	}
	bool isFolder() const
	{
		return _fileType==IMetaWrapper::FileType::Directory;
	}
	size_t size() const
	{
		return _size;
	}
	void fillAttr(struct stat& statbuf) const
	{
		statbuf.st_size=_size;
		if(!isFolder())
			statbuf.st_mode|=S_IFREG;
		else
			statbuf.st_mode|=S_IFDIR;
		// set access mode
		fillAccessMode(statbuf.st_mode);
		statbuf.st_nlink=1;
		statbuf.st_uid=Application::getUID();
		statbuf.st_gid=Application::getGID();
		statbuf.st_ino=reinterpret_cast<ino_t>(this);
		statbuf.st_atim=_lastAccess;
		statbuf.st_mtim=_lastModification;
		statbuf.st_ctim=_lastChange;
	}
	Node *find(fs::path::iterator &it,const fs::path::iterator &end)
	{
		Node *ret=this;
		for(;it!=end;++it)
		{
			Node::iterator itNext=std::find_if(ret->begin(),ret->end(),
											   [&it](const Node &n){return n.name()==*it;});
			if(itNext==ret->end())
				break;
			ret=&*itNext;
		}
		return ret;
	}
	const MD5Signature& MD5()
	{
		return _md5;
	}

	void setMD5(const MD5Signature& md5)
	{
		_md5=md5;
	}

	void setTime(IMetaWrapper::TimeAttrib what,const timespec &value)
	{
		timespec *p=&_lastAccess;
		switch(what)
		{
		case IMetaWrapper::TimeAttrib::ModificationTime:
			p=&_lastModification;
			break;
		case IMetaWrapper::TimeAttrib::ChangeTime:
			p=&_lastChange;
			break;
		}
		*p=value;
	}
	const timespec &time(IMetaWrapper::TimeAttrib what) const
	{
		switch(what)
		{
		case IMetaWrapper::TimeAttrib::ModificationTime:
			return _lastModification;
			break;
		case IMetaWrapper::TimeAttrib::ChangeTime:
			return _lastChange;
			break;
		}
		return _lastAccess;
	}
	void clearTime(IMetaWrapper::TimeAttrib what)
	{
		timespec *p=&_lastAccess;
		switch(what)
		{
		case IMetaWrapper::TimeAttrib::ModificationTime:
			p=&_lastModification;
			break;
		case IMetaWrapper::TimeAttrib::ChangeTime:
			p=&_lastChange;
			break;
		}
		memset(p,0,sizeof(timespec));
	}

	void setSize(size_t size)
	{
		_size=size;
	}

	void addNext(Node *value)
	{
		_next.push_back(value);
	}
	void fillAccessMode(mode_t &mode) const
	{
		if(isFolder())
			mode|=S_IRWXU;
		else
			mode|=S_IRUSR|S_IWUSR;
		mode|=S_IRGRP;
	}

	inline iterator begin() { return _next.begin(); }
	inline const_iterator begin() const { return _next.begin(); }
	inline iterator end() { return _next.end(); }
	inline const_iterator end() const { return _next.end(); }

private:
	bool _dirFilled=false;
	NodeList _next;
	std::string _id;
	fs::path _name;
	size_t _size = 0;
	IMetaWrapper::FileType _fileType=IMetaWrapper::FileType::Binary;
	MD5Signature _md5;
	Node *_parent=0;

	timespec _lastAccess;        // time of last access
	timespec _lastModification;  // time of last modification
	timespec _lastChange;        // time of last status change
};









class FileSink : public IMetaWrapper
{
public:
	FileSink(Node *node=nullptr)
		: _node(node)
	{}
	void setNode(Node *node)
	{
		_node=node;
	}

	// IFileWrapper interface
public:

	virtual void setFileType(FileType value) override
	{
		_node->setFileType(value);
	}
	virtual void setId(const std::string &id) override
	{
		_node->setId(id);
	}
	virtual void setFileName(const std::string &path) override
	{
		_node->setName(path);
	}
	virtual void setSize(size_t size) override
	{
		_node->setSize(size);
	}
	virtual void setMD5(const MD5Signature &md5) override
	{
		_node->setMD5(md5);
	}
	virtual void setTime(TimeAttrib what, const timespec &value) override
	{
		_node->setTime(what,value);
	}
	virtual void setMime(const std::string &value) override
	{
		// TODO Implement
	}
	virtual void isEditable(bool value) override
	{
		// TODO Implement
	}
	virtual void setSupportedConversions(const IConvertFormatPtr &convs) override
	{
		// TODO Implement
	}

private:
	Node *_node=nullptr;
};





Tree::Tree()
{
	_cache.reset(new Cache);
}

void Tree::setDataProvider(const IDataProviderPtr &provider)
{
	_provider=provider;
}

Node *Tree::root()
{
	if(!_root)
	{
		_root=std::make_shared<Node>(nullptr);
		FileSink wrapper(_root.get());
		_provider->fetchMeta("root",wrapper);
		_root->setName("/");
	}
	return _root.get();
}

void Tree::_fillDir(Node *dir)
{
	if(!dir->isFolder())
		return;

	if(dir->_dirFilled)
	{
		dir->_next.clear();
		dir->_dirFilled=false;
	}
	FileSink wrapper;

	const IIdListPtr& ids=_provider->fetchList(dir->id());
	while(ids->hasNext())
	{
		uptr<Node> nn(new Node(dir));
		wrapper.setNode(nn.get());
		_provider->fetchMeta(ids->next(),wrapper);
		dir->addNext(nn.get());
		nn.release();
	}
	dir->_dirFilled=true;
}

Node *Tree::get(const boost::filesystem::path &path)
{
	Node *n=0;
	if(path==ROOT_PATH)
		return root();

	if(n=_cache->findByPath(path))
		return n;

	fs::path::iterator it=path.begin();
	n=root()->find(++it,path.end());
	if(it==path.end())
	{
		_cache->insert(path,n->id(),n);
		return n;
	}
	if(!n->_dirFilled)
	{
		while(it!=path.end() && n->isFolder())
		{
			_fillDir(n);
			Node::iterator itNext=std::find_if(n->begin(),n->end(),
											[&](const Node& node){ return *it==node.name(); });
			if(itNext==n->end())
				break;
			Node *next=&*itNext;
			_cache->insert(fromPathIt(path.begin(),++it),next->id(),next);
			n=next;
		}
		// We have gone this way
		if(it==path.end())
			return n;
	}
	return 0;
}

Node *Tree::find(boost::filesystem::path::iterator &it, const boost::filesystem::path::iterator &end)
{
	return root()->find(it,end);
}

const std::string &Tree::id(Node *node) const
{
	return node->id();
}

void Tree::setName(Node *node, const boost::filesystem::path &name)
{
	node->setName(name);
}

const boost::filesystem::path &Tree::name(Node *node) const
{
	return node->name();
}

bool Tree::isFolder(Node *node) const
{
	return node->isFolder();
}

void Tree::setSize(Node *node, size_t size)
{
	node->setSize(size);
}

size_t Tree::size(Node *node) const
{
	return node->size();
}

void Tree::fillAttr(Node *node,struct stat &statbuf) const
{
	node->fillAttr(statbuf);
}

const MD5Signature &Tree::MD5(Node *node)
{
	return node->MD5();
}

void Tree::setMD5(Node *node, const MD5Signature &md5)
{
	node->setMD5(md5);
}

void Tree::setTime(Node *node, IMetaWrapper::TimeAttrib what, const timespec &value)
{
	node->setTime(what,value);
}

const timespec &Tree::time(Node *node, IMetaWrapper::TimeAttrib what) const
{
	return node->time(what);
}

void Tree::clearTime(Node *node, IMetaWrapper::TimeAttrib what)
{
	node->clearTime(what);
}

class DirIt : public Tree::IDirectoryIterator
{
public:
	DirIt(const Node::iterator &begin,const Node::iterator &end)
		: _it(begin),
		  _itEnd(end)
	{}

	// IDirectoryIterator interface
public:
	virtual bool hasNext() override
	{
		return _it!=_itEnd;
	}
	virtual Node *next() override
	{
		Node *ret=&*_it;
		++_it;
		return ret;
	}

private:
	Node::iterator _it,_itEnd;
};

Tree::IDirectoryIterator *Tree::getDirectoryIterator(Node *dir)
{
	if(dir->isFolder())
	{
		if(!dir->_dirFilled)
			_fillDir(dir);

		return new DirIt(dir->begin(),dir->end());
	}
	return nullptr;
}
