#include "JoinedFileSystem.h"
#include <boost/ptr_container/ptr_list.hpp>
#include <map>
#include "control/Application.h"

bool superpath(const fs::path &basic,fs::path::iterator &it, const fs::path::iterator &end)
{
	for(const auto& b : basic)
	{
		if(it==end || *it!=b)
			return false;
	}
	return true;
}






class JoinedFileSystem : public IFileSystem
{
	class JoinedNode : public INode
	{
		friend class JoinedFileSystem;
	public:
		class MergeIt : public IDirectoryIterator
		{
		public:
			MergeIt(const IDirectoryIteratorPtr &next,JoinedNode *mount)
				: _next(next)
			{
				if(mount)
				{
					for(auto &n : mount->_next)
						_mounts.insert(std::make_pair(n.getName(),&n));
				}
				_it=_mounts.begin();
				if(!_next)
					_mode=1;
				_res=_readNext();
			}

			// IDirectoryIterator interface
		public:
			virtual bool hasNext() override
			{
				return _res!=nullptr;
			}
			virtual INode *next() override
			{
				INode * n=_res;
				_res=_readNext();
				return n;
			}

			INode *_readNext()
			{
				INode * n=nullptr;
				if(_mode==2)
				{
					while(_next->hasNext())
					{
						n=_next->next();
						if(!_mounts.count(n->getName()))
							break;
						n=nullptr;
					}
					if(!n)
						--_mode;
				}
				if(!n && _mode==1)
				{
					if(_it!=_mounts.end())
						n=(_it++)->second;
					else
						--_mode;
				}
				return n;
			}

		private:
			IDirectoryIteratorPtr _next;
			typedef std::map<fs::path,JoinedNode *> NodeMap;
			NodeMap _mounts;
			NodeMap::const_iterator _it;
			unsigned char _mode=2;
			INode *_res=nullptr;
		};

		friend class MergeIt;


		JoinedNode(JoinedNode *parent,const fs::path &name,mode_t mode)
			: _parent(parent),
			  _name(name),
			  _mode(mode)
		{
			_lastAccess=Application::instance()->startTime();
		}

		// INode interface
	public:
		virtual void setId(const std::string &id) override
		{}
		virtual bool isFolder() override
		{
			return true;
		}
		virtual void fillAttr(struct stat &statbuf) override
		{
			G2F_CLEAN_STAT(statbuf);
			statbuf.st_mode=S_IFDIR|_mode;
			statbuf.st_nlink=1;
			statbuf.st_uid=Application::getUID();
			statbuf.st_gid=Application::getGID();
			statbuf.st_ino=reinterpret_cast<ino_t>(this);
			statbuf.st_atim=_lastAccess;
			statbuf.st_mtim=Application::instance()->startTime();
			statbuf.st_ctim=statbuf.st_mtim;
		}
		virtual std::string getId() override
		{
			return std::string();
		}
		virtual void setName(const fs::path &name) override
		{
		}
		virtual fs::path getName() override
		{
			return _name;
		}
		virtual void setSize(size_t size) override
		{
		}
		virtual size_t getSize() override
		{
			return 0;
		}
		virtual void setTime(TimeAttrib what, timespec value) override
		{
		}
		virtual timespec getTime(TimeAttrib what) override
		{
			if(what==TimeAttrib::AccessTime)
				return _lastAccess;
			return Application::instance()->startTime();
		}
		virtual void setMD5(const MD5Signature &md5) override
		{
		}
		virtual MD5Signature getMD5() override
		{
			return MD5Signature();
		}
		virtual IDirectoryIteratorPtr getDirectoryIterator() override
		{
			IDirectoryIteratorPtr ret;
			if(_fs)
				ret=_fs->getRoot()->getDirectoryIterator();
			clock_gettime(CLOCK_REALTIME,&_lastAccess);
			if(_next.empty())
				return ret;
			return std::make_shared<MergeIt>(ret,this);

			return IDirectoryIteratorPtr();
		}
		virtual IContentHandle *openContent(int flags) override
		{
			return nullptr;
		}
		virtual posix_error_code truncate(off_t newSize) override
		{
			return EISDIR;
		}

		JoinedNode *buildBranch(fs::path::const_iterator &it,const fs::path::const_iterator &end,const IFileSystemPtr &fs,mode_t mode)
		{
			JoinedNode *curr=this;
			while(it!=end)
			{
				NodeList::iterator itNext=std::find_if(curr->_next.begin(),curr->_next.end(),
													   [&it](JoinedNode &n){return n.getName()==*it;});
				if(itNext==curr->_next.end())
				{
					// Build branch
					_next.push_back(new JoinedNode(curr,*it,mode));
					curr=&_next.back();
				}
				else
					curr=&*itNext;

				++it;
			}
			curr->_fs=fs;
			return curr;
		}

		JoinedNode* findMountPoint(fs::path::const_iterator &it,const fs::path::const_iterator &end)
		{
			// Greedy in depth
			JoinedNode *mp=nullptr;
			fs::path::const_iterator bckIt=end;

			JoinedNode *curr=this;
			if(curr->_fs)
			{
				mp=curr;
				bckIt=it;
			}
			while(it!=end)
			{
				NodeList::iterator itNext=std::find_if(curr->_next.begin(),curr->_next.end(),
													   [&it](JoinedNode &n){return n.getName()==*it;});
				if(itNext==curr->_next.end())
					break;
				curr=&*itNext;
				++it;
				if(curr->_fs)
				{
					mp=curr;
					bckIt=it;
				}
			}
			it=bckIt;
			return mp;
		}

		JoinedNode* findPrevMountPoint()
		{
			if(_parent)
			{
				JoinedNode* upper=_parent;
				while(upper->_fs || upper->_parent)
				{
					if(upper->_fs)
						return upper;
					upper=upper->_parent;
				}

			}
			return nullptr;
		}

	private:
		JoinedNode *_parent=nullptr;
		fs::path _name;
		typedef boost::ptr_list<JoinedNode> NodeList;
		NodeList _next;
		timespec _lastAccess;
		mode_t _mode=0;

		IFileSystemPtr _fs;
	};
	G2F_DECLARE_PTR(JoinedNode);


public:
	JoinedFileSystem(int rootMode,const std::vector<std::tuple<fs::path,int,IFileSystemPtr>> &mounts)
	{
		if(!mounts.empty())
		{
			// Immediately construct alternate tree from mounts list
			_mountedRoot=std::make_unique<JoinedNode>(nullptr,"/",rootMode);
			for(const auto &m : mounts)
			{
				fs::path::const_iterator it=std::get<0>(m).begin();
				// Omit root dir
				_mountedRoot->buildBranch(++it,std::get<0>(m).end(),std::get<2>(m),std::get<1>(m));
			}
		}
	}

	// IFileSystem interface
public:
	virtual INotify *getNotifier() override
	{
		// TODO
		return nullptr;
	}

	virtual INode *getRoot() override
	{
		return _mountedRoot.get();
	}

	virtual INode *get(const fs::path &path) override
	{
		if(_mountedRoot)
		{
			if(path=="/")
				return _mountedRoot.get();
			fs::path::iterator it=path.begin();
			JoinedNode *p=_mountedRoot->findMountPoint(++it,path.end());
			if(p)
			{
				if(it==path.end())
					return p;
				fs::path rest("/");
				rest/=fromPathIt(it,path.end());
				return p->_fs->get(rest);
			}
		}
		return nullptr;
	}

	virtual CreateResult createNode(const fs::path &path,bool isDirectory) override
	{
		fs::path rest;
		const IFileSystemPtr &fs=getFS(path,rest);
		if(fs)
			return fs->createNode(rest,isDirectory);
		return std::make_tuple(CreateBadPath,nullptr);
	}

	virtual RemoveStatus removeNode(const fs::path &p) override
	{
		fs::path rest;
		const IFileSystemPtr &fs=getFS(p,rest);
		if(fs)
			return fs->removeNode(rest);
		return RemoveStatus::RemoveNotFound;
	}

	IFileSystemPtr getFS(const fs::path &path,fs::path &fsPath)
	{
		fs::path::iterator it=path.begin();
		JoinedNode *p=_mountedRoot->findMountPoint(++it,path.end());
		if(p && it!=path.end())
		{
			fs::path rest("/");
			fsPath="/";
			fsPath/=fromPathIt(it,path.end());
			return p->_fs;
		}
		return IFileSystemPtr();
	}

private:
	JoinedNodeUPtr _mountedRoot;
	//std::vector<std::pair<fs::path,IFileSystemPtr>> _mounts;
};
G2F_DECLARE_PTR(JoinedFileSystem);








JoinedFileSystemFactory::JoinedFileSystemFactory(int mode)
	: _mode(mode)
{}

bool JoinedFileSystemFactory::mount(const boost::filesystem::path &mountPoint, const IFileSystemPtr &fs, int mode)
{
	_mounts.push_back(std::make_tuple(mountPoint,mode,fs));
	return true;
}

IFileSystemPtr JoinedFileSystemFactory::build() const
{
	return std::make_shared<JoinedFileSystem>(_mode,_mounts);
}
