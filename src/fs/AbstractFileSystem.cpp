#include "IFileSystem.h"
#include "INode.h"
#include "utils/log.h"
#include "control/Application.h"
#include "fs/AbstractFileSystem.h"
#include "IContentHandle.h"

namespace
{
	const fs::path ROOT_PATH("/");
}




/**
 * @brief The AbstractFileSystem::Notifier class
 *
 ****************************************************************/
class AbstractFileSystem::Notifier : public IFileSystem::INotify
{
public:
	Notifier(AbstractFileSystem *parent)
		: _parent(parent)
	{}

	// INotify interface
public:
	virtual bs2::connection subscribeToContentChange(const OnContentChange::slot_type &sub) override
	{
		return onContentChange.connect(sub);
	}

	virtual bs2::connection subscribeToNodeRemove(const OnNodeRemove::slot_type &sub) override
	{
		return onNodeRemove.connect(sub);
	}

	IFileSystem::INotify::OnContentChange	onContentChange;
	IFileSystem::INotify::OnNodeRemove		onNodeRemove;

private:
	AbstractFileSystem *_parent=nullptr;
};









/**
 * @brief Directory's observer
 *
 **************************************/
class DirIt : public IDirectoryIterator
{
public:
	DirIt(const AbstractFileSystem::Node::iterator &begin,const AbstractFileSystem::Node::iterator &end)
		: _it(begin),
		  _itEnd(end)
	{}

	// IDirectoryIterator interface
public:
	virtual bool hasNext() override
	{
		return _it!=_itEnd;
	}
	virtual INode *next() override
	{
		AbstractFileSystem::Node *ret=&*_it;
		++_it;
		return ret;
	}

private:
	AbstractFileSystem::Node::iterator _it,_itEnd;
};









/**
 * @brief Handle for opened file
 *
 *****************************************/
class AbstractFileSystem::Node::FileHandle : public IContentHandle
{
public:
	FileHandle(AbstractFileSystem::Node *node,uint64_t fd)
		: _n(node),
		  _fd(fd)
	{}
	FileHandle(int error,AbstractFileSystem::Node *node)
		: _n(node),
		  _error(error)
	{}
	// IContentHandle interface
	virtual AbstractFileSystem::Node *getMeta() override
	{
		return _n;
	}

	virtual posix_error_code flush() override
	{
		_error=0;
		return 0;
	}

	virtual posix_error_code getError() override
	{
		return _error;
	}

	virtual int read(char *buf, size_t len, off_t offset) override
	{
		return _n->_tree->_cm->readContent(_fd,buf,len,offset);
	}

	virtual int write(const char *buf, size_t len, off_t offset) override
	{
		int ret=_n->_tree->_cm->writeContent(_fd,buf,len,offset);
		_changed=true;
		return ret;
	}

	virtual void close() override
	{
		_error=0;
		_n->_tree->_cm->closeFile(_fd);
		if(_changed)
			_n->_tree->_notifier->onContentChange(*_n);
	}
private:
	AbstractFileSystem::Node *_n=nullptr;
	uint64_t _fd=-1;
	posix_error_code _error=0;
	bool _changed=false;

	// IContentHandle interface
public:
};










/**
 * @brief The AbstractFileSystem::Node class
 *
 ************************************************/
AbstractFileSystem::Node::Node(AbstractFileSystem *tree,Node *parent)
	: _tree(tree),
	  _parent(parent)
{}

// INode interface
bool AbstractFileSystem::Node::isFolder()
{
	return _fileType==FileType::Directory;
}

void AbstractFileSystem::Node::fillAttr(struct stat &statbuf)
{
	memset(&statbuf,0,sizeof(struct stat));
	statbuf.st_size=_size;
	//statbuf.st_mode=0;
	if(!isFolder())
		statbuf.st_mode|=S_IFREG;
	else
		statbuf.st_mode|=S_IFDIR;

	// set access mode
	if(isFolder())
		statbuf.st_mode|=S_IRWXU;
	else
		statbuf.st_mode|=S_IRUSR|S_IWUSR;
	statbuf.st_mode|=S_IRGRP;

	statbuf.st_nlink=1;
	statbuf.st_uid=Application::getUID();
	statbuf.st_gid=Application::getGID();
	statbuf.st_ino=reinterpret_cast<ino_t>(this);
	statbuf.st_atim=_lastAccess;
	statbuf.st_mtim=_lastModification;
	statbuf.st_ctim=_lastChange;
}

std::string AbstractFileSystem::Node::getId()
{
	return _id;
}

void AbstractFileSystem::Node::setId(const std::string &id)
{
	_id=id;
}

boost::filesystem::path AbstractFileSystem::Node::getName()
{
	return _name;
}

void AbstractFileSystem::Node::setName(const fs::path &name)
{
	_name=name;
}

void AbstractFileSystem::Node::setSize(size_t size)
{
	_size=size;
}

size_t AbstractFileSystem::Node::getSize()
{
	return _size;
}

void AbstractFileSystem::Node::setTime(TimeAttrib what, timespec value)
{
	timespec *p=&_lastAccess;
	switch(what)
	{
	case TimeAttrib::ModificationTime:
		p=&_lastModification;
		break;
	case TimeAttrib::ChangeTime:
		p=&_lastChange;
		break;
	default:
		break;
	}
	*p=value;
}

timespec AbstractFileSystem::Node::getTime(TimeAttrib what)
{
	switch(what)
	{
	case TimeAttrib::ModificationTime:
		return _lastModification;
		break;
	case TimeAttrib::ChangeTime:
		return _lastChange;
		break;
	}
	return _lastAccess;
}

MD5Signature AbstractFileSystem::Node::getMD5()
{
	return _md5;
}

void AbstractFileSystem::Node::setMD5(const MD5Signature &md5)
{
	_md5=md5;
}

IDirectoryIteratorPtr AbstractFileSystem::Node::getDirectoryIterator()
{
	if(this->isFolder())
	{
		if(!_dirFilled)
			_tree->fillDir(this);
		IDirectoryIteratorPtr ret=std::make_shared<DirIt>(_next.begin(),_next.end());
		clock_gettime(CLOCK_REALTIME,&_lastAccess);
		return ret;
	}
	return IDirectoryIteratorPtr();
}

IContentHandle *AbstractFileSystem::Node::openContent(int flags)
{
	uint64_t fd=0;

	ContentManager &cm=*_tree->_cm;
	if(!cm.is(_id))
		cm.createFile(_id,_tree->cloudReadMedia(*this).get());

	fd=cm.openFile(_id,flags);
	clock_gettime(CLOCK_REALTIME,&_lastAccess);

	return new FileHandle(this,fd);
}

posix_error_code AbstractFileSystem::Node::truncate(off_t newSize)
{
	if(this->isFolder())
		return EISDIR;
	ContentManager &cm=*_tree->_cm;
	if(!cm.is(_id))
		cm.createFile(_id,_tree->cloudReadMedia(*this).get());
	cm.truncateFile(_id,_size,newSize);
	return 0;
}

void AbstractFileSystem::Node::setFileType(FileType type)
{
	_fileType=type;
}

size_t AbstractFileSystem::Node::size() const
{
	return _size;
}

AbstractFileSystem::Node *AbstractFileSystem::Node::getParent()
{
	return _parent;
}

void AbstractFileSystem::Node::removeNodes(Node *what)
{
	NodeList::iterator it=begin(),itEnd=end();
	for(;it!=itEnd;++it)
	{
		if(what && &(*it)!=what)
			continue;

		it->removeNodes(nullptr);
		_tree->cloudRemove(*it);
		if(!it->isFolder())
			_tree->_cm->deleteFile(it->getId());
		_tree->_notifier->onNodeRemove(*it);

		if(what)
			break;
	}
	if(what)
		_next.erase(it);
	else
		_next.clear();
}

AbstractFileSystem::Node *AbstractFileSystem::Node::find(fs::path::iterator &it,const fs::path::iterator &end)
{
	AbstractFileSystem::Node *ret=this;
	for(;it!=end;++it)
	{
		AbstractFileSystem::Node::iterator itNext=std::find_if(ret->_next.begin(),ret->_next.end(),
										   [&it](AbstractFileSystem::Node &n){return n.getName()==*it;});
		if(itNext==ret->_next.end())
			break;
		ret=&*itNext;
	}
	return ret;
}

void AbstractFileSystem::Node::clearTime(TimeAttrib what)
{
	timespec *p=&_lastAccess;
	switch(what)
	{
	case TimeAttrib::ModificationTime:
		p=&_lastModification;
		break;
	case TimeAttrib::ChangeTime:
		p=&_lastChange;
		break;
	}
	memset(p,0,sizeof(timespec));
}

void AbstractFileSystem::Node::addNext(Node *value)
{
	_next.push_back(value);
}









/**
 * @brief AbstractFileSystem
 *
 **************************/
AbstractFileSystem::AbstractFileSystem(const ContentManagerPtr &cm)
	: _cm(cm)
{
	_cache.reset(new Cache);
	_notifier.reset(new Notifier(this));
	//_notifier->subscribeToContentChange(IFileSystem::INotify::OnContentChange::slot_type(&AbstractFileSystem::updateNodeContent,this));
	_notifier->subscribeToContentChange(boost::bind(&AbstractFileSystem::updateNodeContent,this,_1));
	_notifier->subscribeToNodeRemove(boost::bind(&Cache::slotNodeRemoved,_cache.get(),_1));
	//_cManager.init(_provider->getParent()->getConfiguration()->getPaths()->getDir(IPathManager::DATA));
}

// uptr and pimpl
AbstractFileSystem::~AbstractFileSystem()
{}

void AbstractFileSystem::fillDir(Node *dir)
{
	if(!dir->isFolder())
		return;

	if(dir->_dirFilled)
	{
		dir->_next.clear();
		dir->_dirFilled=false;
	}

	const auto& ids=cloudFetchChildrenList(dir->getId());
	for(const auto &id : ids)
	{
		uptr<Node> nn(new Node(this,dir));
		nn->setId(id);
		cloudFetchMeta(*nn);
		dir->addNext(nn.release());
	}
	dir->_dirFilled=true;
}

// IData interface
IFileSystem::INotify *AbstractFileSystem::getNotifier()
{
	return _notifier.get();
}

AbstractFileSystem::Node *AbstractFileSystem::getRoot()
{
	if(!_root)
	{
		_root=std::make_shared<AbstractFileSystem::Node>(this,nullptr);
		_root->setId("root");
		cloudFetchMeta(*_root);
		_root->setId("root");
		_root->setName("/");
	}
	return _root.get();
}

INode *AbstractFileSystem::get(const fs::path &path)
{
	if(path==ROOT_PATH)
		return getRoot();

	if(INode *in=_cache->findByPath(path))
		return in;

	fs::path::iterator it=path.begin();
	AbstractFileSystem::Node *n=getRoot()->find(++it,path.end());
	if(it==path.end())
	{
		_cache->insert(path,n->getId(),n);
		return n;
	}
	if(!n->_dirFilled)
	{
		while(it!=path.end() && n->isFolder())
		{
			fillDir(n);
			AbstractFileSystem::Node::iterator itNext=std::find_if(n->begin(),n->end(),
											[&](AbstractFileSystem::Node& node){ return *it==node.getName(); });
			if(itNext==n->end())
				break;
			AbstractFileSystem::Node *next=&*itNext;
			_cache->insert(fromPathIt(path.begin(),++it),next->getId(),next);
			n=next;
		}
		// We have gone this way
		if(it==path.end())
			return n;
	}
	return 0;
}

IFileSystem::CreateResult AbstractFileSystem::createNode(const fs::path &path,bool isDirectory)
{
	fs::path::iterator it=path.begin(),itEnd=path.end();
	Node *n=getRoot()->find(++it,itEnd);
	size_t entries=std::distance(it,itEnd);
	if(!entries)
		return std::make_tuple(CreateAlreadyExists,nullptr);

	uptr<Node> nn;
	while(entries>0)
	{
		nn=std::make_unique<Node>(this,n);
		nn->setName(*it);
		nn->setFileType(INode::FileType::Directory);

		// last entry - file or dir (depends from flag)
		if(entries==1 && !isDirectory)
			nn->setFileType(INode::FileType::Binary);

		cloudCreateMeta(*nn);
		n->addNext(nn.get());
		n=nn.release();

		--entries;
		++it;
	}
	return std::make_tuple(CreateSuccess,n);
}

IFileSystem::RemoveStatus AbstractFileSystem::removeNode(const fs::path &path)
{
	INode *n=get(path);
	if(!n)
		return IFileSystem::RemoveNotFound;
	Node *node=dynamic_cast<Node*>(n);
	assert(node);
	Node *parent=node->getParent();
	if(!parent)
		return IFileSystem::RemoveForbidden;
	parent->removeNodes(node);
	return IFileSystem::RemoveSuccess;
}

void AbstractFileSystem::updateNodeContent(INode &n)
{
	Node *node=dynamic_cast<Node*>(&n);
	assert(node);
	const ContentManager::IReaderPtr &reader=_cm->readContent(n.getId());
	cloudUpdate(*node,nullptr,"",reader.get());
	cloudFetchMeta(*node);
}

