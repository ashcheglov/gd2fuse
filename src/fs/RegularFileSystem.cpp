#include "IFileSystem.h"
#include "INode.h"
#include "utils/log.h"
#include "control/Application.h"
#include "cache/Cache.h"
#include <boost/ptr_container/ptr_list.hpp>
#include "ContentManager.h"
#include "IContentHandle.h"
#include "JoinedFileSystem.h"

namespace
{
	const fs::path ROOT_PATH("/");
}

//
//  Tree Forward Decl
//
///////////////////////////////////////
class Tree : public IFileSystem
{
public:
	//
	//  INode implemetation
	//
	///////////////////////////////////////
	class Node : public INode
	{
	public:
		friend class Tree;

		typedef boost::ptr_list<Node> NodeList;
		typedef NodeList::iterator iterator;
		typedef NodeList::const_iterator const_iterator;

		class DirIt : public IDirectoryIterator
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
			virtual INode *next() override
			{
				Node *ret=&*_it;
				++_it;
				return ret;
			}

		private:
			Node::iterator _it,_itEnd;
		};

		class FileHandle : public IContentHandle
		{
		public:
			FileHandle(Node *node,uint64_t fd)
				: _n(node),
				  _fd(fd)
			{}
			FileHandle(int error,Node *node)
				: _n(node),
				  _error(error)
			{}
			// IContentHandle interface
			virtual Node *getMeta() override
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
				_error=0;
				try
				{
					return _n->_tree->_cManager.readContent(_fd,buf,len,offset);
				}
				catch(G2FException &e)
				{
					_error=e.code().default_error_condition().value();
				}
				catch(err::system_error &e)
				{
					_error=e.code().default_error_condition().value();
				}
				return 0;
			}

			virtual posix_error_code close() override
			{
				_error=0;
				try
				{
					return _n->_tree->_cManager.closeFile(_fd);
				}
				catch(G2FException &e)
				{
					_error=e.code().default_error_condition().value();
				}
				catch(err::system_error &e)
				{
					_error=e.code().default_error_condition().value();
				}
				return _error;
			}
		private:
			Node *_n=nullptr;
			uint64_t _fd=-1;
			posix_error_code _error=0;

			// IContentHandle interface
		public:
		};

	public:
		// Simple model: regular tree. Today we don't support links
		Node(Tree *tree,Node *parent)
			: _tree(tree),
			  _parent(parent)
		{}

		// INode interface
		virtual bool isFolder() override
		{
			return _fileType==IMetaWrapper::FileType::Directory;
		}

		virtual void fillAttr(struct stat &statbuf) override
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

		virtual std::string getId() override
		{
			return _id;
		}

		virtual void setId(const std::string &id) override
		{
			_id=id;
		}

		virtual boost::filesystem::path getName() override
		{
			return _name;
		}

		virtual void setName(const fs::path &name) override
		{
			_name=name;
		}

		virtual void setSize(size_t size) override
		{
			_size=size;
		}

		virtual size_t getSize() override
		{
			return _size;
		}

		virtual void setTime(TimeAttrib what, timespec value) override
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

		virtual timespec getTime(TimeAttrib what) override
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

		virtual MD5Signature getMD5() override
		{
			return _md5;
		}

		virtual void setMD5(const MD5Signature &md5) override
		{
			_md5=md5;
		}

		virtual IDirectoryIteratorPtr getDirectoryIterator() override
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

		virtual IContentHandle *openContent(int flags) override
		{
			uint64_t fd=0;
			try
			{
				fd=_tree->_cManager.openFile(_id,flags);
				clock_gettime(CLOCK_REALTIME,&_lastAccess);
			}
			catch(G2FException &e)
			{
				return new FileHandle(e.code().default_error_condition().value(),this);
			}
			catch(err::system_error &e)
			{
				return new FileHandle(e.code().default_error_condition().value(),this);
			}
			return new FileHandle(this,fd);
		}

		virtual posix_error_code truncate(off_t newSize) override
		{
			if(this->isFolder())
				return EISDIR;
			//  TODO
			return ENOSYS;
		}

		void setFileType(IMetaWrapper::FileType type)
		{
			_fileType=type;
		}

		size_t size() const
		{
			return _size;
		}

		Node *find(fs::path::iterator &it,const fs::path::iterator &end)
		{
			Node *ret=this;
			for(;it!=end;++it)
			{
				Node::iterator itNext=std::find_if(ret->_next.begin(),ret->_next.end(),
												   [&it](Node &n){return n.getName()==*it;});
				if(itNext==_next.end())
					break;
				ret=&*itNext;
			}
			return ret;
		}

		void clearTime(TimeAttrib what)
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

		void addNext(Node *value)
		{
			_next.push_back(value);
		}

		void fillAccessMode(mode_t &mode)
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
		Tree *_tree=nullptr;
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

	//
	//  IMetaWrapper implemetation
	//
	///////////////////////////////////////
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
public:
	Tree(const IDataProviderPtr &dp)
		: _provider(dp)
	{
		_cache.reset(new Cache);
		_cManager.init(_provider->getParent()->getConfiguration()->getPaths()->getDir(IPathManager::DATA),_provider);
	}

	void fillDir(Node *dir)
	{
		if(!dir->isFolder())
			return;

		if(dir->_dirFilled)
		{
			dir->_next.clear();
			dir->_dirFilled=false;
		}
		FileSink wrapper;

		const IIdListPtr& ids=_provider->fetchList(dir->getId());
		while(ids->hasNext())
		{
			uptr<Node> nn(new Node(this,dir));
			wrapper.setNode(nn.get());
			_provider->fetchMeta(ids->next(),wrapper);
			dir->addNext(nn.get());
			nn.release();
		}
		dir->_dirFilled=true;
	}

	// IData interface
	virtual Node *getRoot() override
	{
		if(!_root)
		{
			_root=std::make_shared<Node>(this,nullptr);
			FileSink wrapper(_root.get());
			_provider->fetchMeta("root",wrapper);
			_root->setName("/");
		}
		return _root.get();
	}

	virtual INode *get(const fs::path &path) override
	{
		if(path==ROOT_PATH)
			return getRoot();

		if(INode *in=_cache->findByPath(path))
			return in;

		fs::path::iterator it=path.begin();
		Node *n=getRoot()->find(++it,path.end());
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
				Node::iterator itNext=std::find_if(n->begin(),n->end(),
												[&](Node& node){ return *it==node.getName(); });
				if(itNext==n->end())
					break;
				Node *next=&*itNext;
				_cache->insert(fromPathIt(path.begin(),++it),next->getId(),next);
				n=next;
			}
			// We have gone this way
			if(it==path.end())
				return n;
		}
		return 0;
	}

	//Node *remove(const fs::path &path);
	//void move(const fs::path &from,const fs::path &to);

private:
	friend class Node;

	sptr<Node> _root;
	uptr<Cache> _cache;
	IDataProviderPtr _provider;
	ContentManager _cManager;
};

















IFileSystemPtr createRegularFS(const IDataProviderPtr &dp)
{
	return std::make_shared<Tree>(dp);
}

G2F_DECLARE_PTR(Tree);

