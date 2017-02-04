#include "IFileSystem.h"
#include "INode.h"
#include "utils/log.h"
#include "control/Application.h"
#include "fs/AbstractFileSystem.h"
#include "IContentHandle.h"
#include <fcntl.h>
#include "utils/assets.h"

namespace
{
	const fs::path ROOT_PATH("/");
}




/**
 * @brief The AbstractFileSystem::Notifier class
 *
 ****************************************************************/
class AbstractFileSystem::Notifier : public IFileSystem::INotifier
{
public:
	Notifier(AbstractFileSystem *parent)
		: _parent(parent)
	{}

	// INotify interface
public:
	virtual bs2::connection subscribeToFileContentChange(const OnFileContentChange::slot_type &sub) override
	{
		return onContentChange.connect(sub);
	}

	virtual bs2::connection subscribeToNodeRemove(const OnNodeRemove::slot_type &sub) override
	{
		return onNodeRemove.connect(sub);
	}

	virtual bs2::connection subscribeToNodeChange(const OnNodeChange::slot_type &sub) override
	{
		return onNodeChange.connect(sub);
	}

	virtual bs2::connection subscribeToDirectoryChange(const OnDirectoryChange::slot_type &sub) override
	{
		return onDirChange.connect(sub);
	}

	virtual bs2::connection subscribeToNodeCreate(const OnNodeCreate::slot_type &sub) override
	{
		return onNodeCreate.connect(sub);
	}

	IFileSystem::INotifier::OnFileContentChange		onContentChange;
	IFileSystem::INotifier::OnNodeChange			onNodeChange;
	IFileSystem::INotifier::OnNodeRemove			onNodeRemove;
	IFileSystem::INotifier::OnNodeCreate			onNodeCreate;
	IFileSystem::INotifier::OnDirectoryChange		onDirChange;

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
	FileHandle(AbstractFileSystem::Node *node,int64_t fd,bool changed)
		: _n(node),
		  _fd(fd),
		  _changed(changed)
	{}
	~FileHandle()
	{
		try
		{
			if(_fd!=-1)
				_n->_tree->_cm->closeFile(_fd);
		}
		catch(...)
		{}
	}

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
		_fd=-1;
		if(_changed)
			_n->_tree->_notifier->onContentChange(*_n);
	}
private:
	AbstractFileSystem::Node *_n=nullptr;
	int64_t _fd=-1;
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
INode::NodeType AbstractFileSystem::Node::getNodeType()
{
	return _fileType;
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
	bool willBeCreated=false;
	ContentManager &cm=*_tree->_cm;
	bool exists=cm.is(_id);
	if(flags&O_CREAT)
	{
		if(!exists)
			willBeCreated=true;
		else
		if(flags&O_EXCL)
			willBeCreated=cm.deleteFile(_id);
	}

	if(!willBeCreated)
	{
		if(!exists)
			cm.createFile(_id,_tree->cloudReadMedia(*this).get());
	}

	int64_t fd=cm.openFile(_id,flags);
	clock_gettime(CLOCK_REALTIME,&_lastAccess);

	return new FileHandle(this,fd,willBeCreated);
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


MD5Signature AbstractFileSystem::Node::getMD5()
{
	return _md5;
}

void AbstractFileSystem::Node::setMD5(const MD5Signature &md5)
{
	_md5=md5;
}

void AbstractFileSystem::Node::setFileType(NodeType type)
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

bool AbstractFileSystem::Node::importNreplace(INode &value,const fs::path &newName,Node *toReplace)
{
	Node *v=dynamic_cast<Node*>(&value);
	if(v && v==toReplace)
		return false;

	if(toReplace)
	{
		assert(toReplace->_parent==this);
		// Value is from our filesystem
		if(v && v->_tree==_tree)
		{
			_tree->cloudRemove(*toReplace);
			_tree->_notifier->onNodeRemove(*toReplace);
			auto p=toReplace->patch(*v,Field::Parent|Field::Name|Field::Id|Field::Time|Field::Content);
			_tree->cloudUpdate(*toReplace,p);
			_tree->_notifier->onNodeChange(*toReplace,p);
		}
		else
		{
			// TODO Make via update instead delete/create
			auto it=std::find_if(_next.begin(),_next.end(),[&](const auto &e){ return &e==toReplace; });
			if(it!=_next.end())
			{
				_tree->cloudRemove(*toReplace);
				_tree->_notifier->onNodeRemove(*toReplace);
				_next.erase(it);
			}
		}
	}
	uptr<Node> n(new Node(_tree,this));
	if(newName.empty())
		n->setName(value.getName());
	else
		n->setName(newName);
	n->setFileType(value.getNodeType());
	n->setTime(TimeAttrib::AccessTime,value.getTime(TimeAttrib::AccessTime));
	n->setTime(TimeAttrib::ModificationTime,value.getTime(TimeAttrib::ModificationTime));
	n->setTime(TimeAttrib::ChangeTime,value.getTime(TimeAttrib::ChangeTime));

	_tree->cloudCreateMeta(*n);
	_tree->_notifier->onNodeCreate(*n);
	Node *nn=n.release();
	_next.push_back(nn);
	_tree->_notifier->onDirChange(*this,INotifier::Added,*nn);

	if(value.isFolder())
	{
		IDirectoryIteratorPtr dirIt=value.getDirectoryIterator();
		while(dirIt->hasNext())
		{
			nn->importNreplace(*dirIt->next());
		}
	}
	else
	{
		// TODO Change open flags on something independent from POSIX
		size_t size=value.getSize();
		if(size)
		{
			uptr<IContentHandle> hSource(value.openContent(O_RDONLY));
			uptr<IContentHandle> hDest(nn->openContent(O_CREAT));

			char buf[4096];
			off_t offset=0;
			int readed=0;
			while(size!=0 && (readed=hSource->read(buf,sizeof(buf),offset))!=0)
			{
				if(readed==-1)
					G2F_EXCEPTION("Can't read imported file '%1'").arg(value.getName()).throwItSystem(hSource->getError());
				offset+=readed;
				if(hDest->write(buf,readed,offset)==-1)
					G2F_EXCEPTION("Can't write imported file '%1'").arg(value.getName()).throwItSystem(hDest->getError());

			}
			hDest->close();
			hSource->close();
		}
	}
	return true;
}

int AbstractFileSystem::Node::patch(Node &dataSource,int patchField)
{
	int ret=0;
	if(patchField & Name)
	{
		if(this->_name!=dataSource._name)
		{
			this->_name=dataSource._name;
			ret|=Name;
		}
	}

	if(patchField & Parent)
	{
		if(this->_parent!=dataSource._parent)
		{
			this->_parent=dataSource._parent;
			ret|=Parent;
		}
	}

	if(patchField & Id)
	{
		if(this->_id!=dataSource._id)
		{
			this->_id=dataSource._id;
			ret|=Id;
		}
	}

	if(patchField & Time)
	{
		if(this->_lastAccess!=dataSource._lastAccess)
		{
			this->_lastAccess=dataSource._lastAccess;
			ret|=Time;
		}
		if(this->_lastChange!=dataSource._lastChange)
		{
			this->_lastChange=dataSource._lastChange;
			ret|=Time;
		}
		if(this->_lastModification!=dataSource._lastModification)
		{
			this->_lastModification=dataSource._lastModification;
			ret|=Time;
		}
	}

	if(patchField & Content)
	{
		this->_fileType=dataSource._fileType;
		this->_next=dataSource._next;
		ret|=Content;
	}
	return ret;
}

bool AbstractFileSystem::Node::detachNode(Node *node)
{
	auto it=std::find_if(_next.begin(),_next.end(),[&](const auto &e){ return &e==node; });
	if(it!=_next.end())
	{
		node->_parent=nullptr;
		_next.release(it).release();
		return true;
	}
	return false;
}

void AbstractFileSystem::Node::attachNode(Node *node)
{
	auto it=std::find_if(_next.begin(),_next.end(),[&](const auto &e){ return &e==node; });
	if(it==_next.end())
	{
		node->_parent=this;
		_next.push_back(node);
	}
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
	_notifier->subscribeToFileContentChange(boost::bind(&AbstractFileSystem::updateNodeContent,this,_1));
	_notifier->subscribeToNodeRemove(boost::bind(&Cache::slotNodeRemoved,_cache.get(),_1));
	_notifier->subscribeToNodeChange(boost::bind(&Cache::slotNodeChanged,_cache.get(),_1,_2));
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
IFileSystem::INotifier *AbstractFileSystem::getNotifier()
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
		nn->setFileType(INode::NodeType::Directory);

		// last entry - file or dir (depends from flag)
		if(entries==1 && !isDirectory)
			nn->setFileType(INode::NodeType::Binary);

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
	Node *node=getNode(path,false);
	if(!node)
		return IFileSystem::RemoveNotFound;
	Node *parent=node->getParent();
	if(!parent)
		return IFileSystem::RemoveNotFound;
	parent->removeNodes(node);
	return IFileSystem::RemoveSuccess;
}

void AbstractFileSystem::renameNode(const boost::filesystem::path &oldPath, const boost::filesystem::path &newPath)
{
	Node *oldNode=getNode(oldPath,true);
	Node *newNode=getNode(newPath,false);

	Node *newParentNode=nullptr;
	if(!newNode)
		newParentNode=getNode(newPath.parent_path(),false);
	else
		newParentNode=newNode->getParent();
	if(!newParentNode)
		G2F_EXCEPTION("Parent path '%1' not found").arg(newPath.parent_path()).throwItSystem(ENOENT);

	Node *oldParentNode=oldNode->getParent();
	if(!oldParentNode)
		G2F_EXCEPTION("Parent path '%1' not found").arg(oldPath.parent_path()).throwItSystem(ENOENT);

	// Detach oldNode from tree
	assert(oldParentNode->detachNode(oldNode));
	// rename
	oldNode->setName(newPath.filename());
	// and attach oldNode to parent newNode and change parent information in cloud
	newParentNode->attachNode(oldNode);

	int patchFields=Node::Field::Name;
	if(newParentNode!=oldParentNode)
		patchFields|=Node::Field::Parent;
	cloudUpdate(*oldNode,patchFields);
	_notifier->onNodeChange(*oldNode,patchFields);

	// then remove newNode (completely - from cloud and tree)
	if(newNode)
		newParentNode->removeNodes(newNode);

}

void AbstractFileSystem::replaceNode(const boost::filesystem::path &pathToReplace, INode &onThis)
{
	Node *node=getNode(pathToReplace,true);
	Node *base=node->getParent();
	bool changed=base->importNreplace(onThis,fs::path(),node);
	if(changed)
		this->_notifier->onNodeChange(*node,-1);
}

void AbstractFileSystem::insertNode(const fs::path &parentPath, INode &that)
{
	Node *node=getNode(parentPath,true);
	node->importNreplace(that);
}

AbstractFileSystem::Node *AbstractFileSystem::getNode(const fs::path &path, bool throwIfMissed)
{
	INode *n=get(path);
	if(!n && throwIfMissed)
		G2F_EXCEPTION("Path '%1' not found").arg(path).throwItSystem(ENOENT);
	if(!n)
		return nullptr;
	Node *ret=dynamic_cast<Node*>(n);
	assert(ret);
	return ret;
}

void AbstractFileSystem::updateNodeContent(INode &n)
{
	Node *node=dynamic_cast<Node*>(&n);
	assert(node);
	const ContentManager::IReaderPtr &reader=_cm->readContent(node->getId());
	cloudUpdate(*node,{},"",reader.get());
	cloudFetchMeta(*node);
}

