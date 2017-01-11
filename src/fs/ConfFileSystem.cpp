#include "IFileSystem.h"
#include "AbstractMountFileSystem.h"
#include <sstream>
#include "control/IConfiguration.h"
#include "control/Application.h"

/*
 * Default Stub Implementation
 *
 * *************************************/
class AbstractNodeStub : public INode
{
	// INode interface
public:
	virtual void setId(const std::string &id) override
	{}
	virtual std::string getId() override
	{
		return std::string();
	}
	virtual void setName(const boost::filesystem::path &name) override
	{}
	virtual void setSize(size_t size) override
	{}
	virtual size_t getSize() override
	{
		return 0;
	}
	virtual void setTime(TimeAttrib what, timespec value) override
	{}
	virtual timespec getTime(TimeAttrib what) override
	{
		return Application::instance()->startTime();
	}
	virtual void setMD5(const MD5Signature &md5) override
	{}
	virtual MD5Signature getMD5() override
	{
		return MD5Signature();
	}
};


std::string exportPropery(IPropertyDefinition& prop)
{
	std::ostringstream o;

	IPropertyType &pt=*prop.getType();

	o << "name: " << prop.getName() << std::endl
	  << "type: " << pt.getType().toString() << std::endl
	  << "runtime: " << std::boolalpha << prop.isRuntimeChange() << std::endl
	  << "value: " << prop.getValue() << std::endl
	  << "default: " << prop.getDefaultValue() << std::endl
	  << "title: " << prop.getDescription();
	if(pt.getType()==IPropertyType::ENUM)
	{
		o  << std::endl << "enum_values: " ;

		const auto & ed=pt.getEnumDef();
		for(size_t i=0; i<ed->getSize(); ++i)
		{
			if(i)
				o << ",";
			o << ed->getEntry(i);
		}
	}
	return o.str();
}



/*
 * Property Node
 *
 * *************************************/
class PropNode : public AbstractNodeStub
{

public:
	class ContentHandle : public IContentHandle
	{
	public:
		ContentHandle(INode *parent,const IPropertyDefinitionPtr &p,std::string &&s)
			: _parent(parent),
			  _p(p),
			  _s(std::move(s))
		{}
		// IContentHandle interface
	public:
		virtual INode *getMeta() override
		{
			return _parent;
		}
		virtual int getError() override
		{
			return 0;
		}
		virtual int read(char *buf, size_t len, off_t offset) override
		{
			return boost::numeric_cast<int>(_s.copy(buf,len,offset));
		}
		virtual int close() override
		{
			return 0;
		}
	private:
		INode *_parent=nullptr;
		IPropertyDefinitionPtr _p;
		std::string _s;
	};



	PropNode(const IPropertyDefinitionPtr &p)
		:_p(p)
	{
		_lastAccess=_p->getLastChangeTime();
	}

	// INode interface
public:
	virtual bool isFolder() override
	{
		return false;
	}
	virtual void fillAttr(struct stat &statbuf) override
	{
		statbuf.st_size=0;
		statbuf.st_mode|=S_IFREG;
		// set access mode

		statbuf.st_mode|=S_IRUSR|S_IRGRP;
		if(_p->isRuntimeChange())
			statbuf.st_mode|=S_IWUSR;
		statbuf.st_nlink=1;
		statbuf.st_uid=Application::getUID();
		statbuf.st_gid=Application::getGID();
		statbuf.st_ino=reinterpret_cast<ino_t>(this);
		statbuf.st_atim=_lastAccess;
		statbuf.st_mtim=_p->getLastChangeTime();
		statbuf.st_ctim=Application::instance()->startTime();

	}
	virtual fs::path getName() override
	{
		return _p->getName();
	}
	virtual timespec getTime(TimeAttrib what) override
	{
		if(what==TimeAttrib::ModificationTime)
			return _p->getLastChangeTime();
		if(what==TimeAttrib::AccessTime)
			return _lastAccess;
		return AbstractNodeStub::getTime(what);
	}
	virtual IDirectoryIteratorPtr getDirectoryIterator() override
	{
		return IDirectoryIteratorPtr();
	}
	virtual IContentHandle *openContent(int flags) override
	{
		return new ContentHandle(this,_p,exportPropery(*_p));
	}

private:
	IPropertyDefinitionPtr _p;
	timespec _lastAccess;
};
G2F_DECLARE_PTR(PropNode);





/*
 * Root Node Implementation
 *
 * *************************************/
class PropRootNode : public AbstractNodeStub
{
public:
	typedef std::vector<PropNodePtr> PropList;

	class It : public IDirectoryIterator
	{
	public:
		It(const PropList &propList)
		{
			_it=propList.begin();
			_itEnd=propList.end();
		}

		// IDirectoryIterator interface
	public:
		virtual bool hasNext() override
		{
			return _it!=_itEnd;
		}
		virtual INode *next() override
		{
			return (*_it++).get();
		}

	private:
		PropList::const_iterator _it,_itEnd;
	};




	PropRootNode(const IPropertiesListPtr &list)
	{
		const auto &it=list->getProperties();
		while(it->hasNext())
			_props.push_back(std::make_shared<PropNode>(it->next()));
		_lastAccess=Application::instance()->startTime();
	}

	// INode interface
public:
	virtual bool isFolder() override
	{
		return true;
	}
	virtual void fillAttr(struct stat &statbuf) override
	{
		statbuf.st_size=0;
		statbuf.st_mode|=S_IFDIR;

		// TODO Make configurable?
		statbuf.st_mode|=S_IRUSR|S_IXUSR|S_IRGRP|S_IWGRP;
		statbuf.st_nlink=1;
		statbuf.st_uid=Application::getUID();
		statbuf.st_gid=Application::getGID();
		statbuf.st_ino=reinterpret_cast<ino_t>(this);
		statbuf.st_atim=_lastAccess;
		statbuf.st_mtim=Application::instance()->startTime();
		statbuf.st_ctim=statbuf.st_mtim;
	}
	virtual fs::path getName() override
	{
		return "/";
	}
	virtual IDirectoryIteratorPtr getDirectoryIterator() override
	{
		clock_gettime(CLOCK_REALTIME,&_lastAccess);
		return std::make_shared<It>(_props);
	}
	virtual IContentHandle *openContent(int flags) override
	{
		return nullptr;
	}

	INode* get(const fs::path &fileName)
	{
		for(const auto& p : _props)
		{
			if(p->getName()==fileName)
				return p.get();
		}
		return nullptr;
	}

private:
	PropList _props;
	timespec _lastAccess;
};
G2F_DECLARE_PTR(PropRootNode);





/*
 * Whole FS around IConfiguration
 *
 * *************************************/
class ConfigurationFSWrapper : public AbstractMountFileSystem
{
public:
	ConfigurationFSWrapper(const IConfigurationPtr &conf)
		:_conf(conf)
	{
		_root=std::make_shared<PropRootNode>(_conf->getProperiesList());
	}

	// IFileSystem interface
public:
	virtual INode *getRoot() override
	{
		return _root.get();
	}

	virtual INode *get(const fs::path &p) override
	{
		if(p=="/")
			return getRoot();
		INode * m=findInMounts(p);
		if(m)
			return m;
		return _root->get(p.filename());
	}

private:
	IConfigurationPtr _conf;
	PropRootNodePtr _root;
};






IFileSystemPtr createConfigurationFS(const IConfigurationPtr &conf)
{
	return std::make_shared<ConfigurationFSWrapper>(conf);
}
