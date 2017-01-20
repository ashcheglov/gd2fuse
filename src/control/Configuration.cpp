#include "Configuration.h"
#include "props/AbstractFileStaticInitPropertiesList.h"
#include "paths/PathManager.h"


#define G2F_GLOBAL_CONF_FILENAME	"global.conf"

namespace
{

const AbstractStaticInitPropertiesList::PropDefi propsDefi[]=
{
//	name							type					enum		def					change	descr
	"change_collision_strategy",	IPropertyType::ENUM,	"coll",		"prefer_remote",	true,	"Strategy of resolving collision of changed files.",
	"cache_max_size",				IPropertyType::UINT,	0,			"100",				true,	"Max size of file cache (Megabytes).",
	"read_only",					IPropertyType::BOOL,	0,			"false",			true,	"Read-only mode.",
	"cache_prefetch_strategy",		IPropertyType::ENUM,	"cpst",		"lazy",				true,	"Strategy of anticipatory caching.",
	"permission_new_file",			IPropertyType::OINT,	0,			"644",				true,	"New files permissions.",
	"permission_new_folder",		IPropertyType::OINT,	0,			"755",				true,	"New directory permissions.",
	"permission_new_spec",			IPropertyType::OINT,	0,			"444",				true,	"Permissions to new special files (non editable).",
	"erase_policy",					IPropertyType::ENUM,	"erpo",		"trash",			true,	"Element's deleting policy.",
	"sync",							IPropertyType::BOOL,	0,			"true",				true,	"Syncronization with remote side.",
	"limit_upload_max",				IPropertyType::UINT,	0,			"0",				true,	"Max upload speed (kilobits/sec).",
	"limit_download_max",			IPropertyType::UINT,	0,			"0",				true,	"Max download speed (kilobits/sec).",
	"limit_upload_min",				IPropertyType::UINT,	0,			"0",				true,	"Min upload speed limit after that sync will be disabled (kilobits/sec).",
	"control_dir",					IPropertyType::PATH,	0,			"/.control",		false,	"Control directory's mountpoint."
	// TODO Add property describes temporary files to exclude from exchange process
};

const AbstractStaticInitPropertiesList::EnumDefi enumDefi[]=
{
	"coll", {
		{"prefer_local",	"Prefer mine file."},
		{"prefer_remote",	"Prefer remote file."}
	},
	"cpst", {
		{ "none",  "Do not use anticipatory caching." },
		{ "eager", "Aggresive anticipatory caching." },
		{ "lazy",  "Balanced and soft :)" }	// TODO Rewrite
	},
	"erpo", {
		{ "forever", "Final deletion." },
		{ "trash",	 "Delete to trash." }
	}
};


}


class GlobalProperties : public AbstractFileStaticInitPropertiesList
{

public:
	GlobalProperties(const fs::path &fileName)
		: _fileName(fileName)
	{}

	// IPropertiesList interface
public:
	virtual std::string getName() override
	{
		return "global";
	}

	// AbstractStaticInitPropertiesList interface
protected:
	virtual std::pair<const PropDefi *, size_t> getPropertiesDefinition() override
	{
		return std::make_pair(propsDefi,sizeof(propsDefi)/sizeof(AbstractStaticInitPropertiesList::PropDefi));
	}

	virtual std::pair<const EnumDefi *, size_t> getEnumsDefinition() override
	{
		return std::make_pair(enumDefi,sizeof(enumDefi)/sizeof(AbstractStaticInitPropertiesList::EnumDefi));
	}

	// AbstractFileStaticInitPropertiesList interface
public:
	virtual boost::filesystem::path getPropertiesFileName() override
	{
		return _fileName;
	}
private:
	fs::path _fileName;
};

G2F_DECLARE_PTR(GlobalProperties);





class GlobalConfiguration : public IConfiguration
{

public:
	GlobalConfiguration(const fs::path &manualConf)
	{
		if(manualConf.empty())
			_pathManager=createXDGPathManager();
		else
			_pathManager=createSinglePointPathManager(manualConf);
		auto confFile=_pathManager->getDir(IPathManager::CONFIG)/G2F_GLOBAL_CONF_FILENAME;
		_globalProps=std::make_shared<GlobalProperties>(confFile);
	}

	// IConfiguration interface
public:
	virtual IPathManagerPtr getPaths() override
	{
		return _pathManager;
	}
	virtual boost::optional<std::string> getProperty(const std::string &name) override
	{
		IPropertyDefinitionPtr p=_globalProps->findProperty(name);
		if(p)
			return p->getValue();
		return boost::none;
	}

	virtual G2FError setProperty(const std::string &name, const std::string &value) override
	{
		IPropertyDefinitionPtr p=_globalProps->findProperty(name);
		if(p)
			return p->setValue(value);
		return G2FError(G2FErrorCodes::PropertyNotFound).setDetail(name);
	}

	virtual IPropertiesListPtr getProperiesList() override
	{
		return _globalProps;
	}

private:
	IPathManagerPtr _pathManager;
	GlobalPropertiesPtr _globalProps;

};

IConfigurationPtr createGlobalConfiguration(const boost::filesystem::path &manualConf)
{
	return std::make_shared<GlobalConfiguration>(manualConf);
}
