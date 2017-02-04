#include <map>
#include <boost/filesystem/fstream.hpp>
#include "AbstractFileStaticInitPropertiesList.h"
#include "IPropertiesList.h"
#include "control/Application.h"
#include "control/types/PropertyTypesRegistry.h"
//#include <boost/utility/string_ref.hpp>
#include <regex>

const char *rePattern=R"--(^\s*(\w+)\s*=\s*(\w+))--";
bool updateConf(std::fstream &f,const std::string &name,const std::string &val)
{
	std::regex re(rePattern);

	char line[128];
	std::fstream::pos_type offset=-1;

	// Will read file line by line while we do not detect given property.
	while(!f.eof())
	{
		f.getline(line,sizeof(line));

		std::cmatch ma;
		if(std::regex_match(line,ma,re))
		{
			if(ma[1].str()==name)
			{
				const std::string &cv=ma[2].str();
				if(cv==val)
					return false;
				// Memorize position
				offset=f.tellp();
				offset-=cv.size();
			}
		}
		// Bingo!
		if(offset!=-1)
		{
			// Read the rest of file
			std::string rest;
			rest.assign(std::istream_iterator<std::string::value_type>(f),
										   std::istream_iterator<std::string::value_type>());
			// Move file input pointer
			f.seekg(offset);
			f << val << std::endl << rest;
			return true;
		}
	}
	f << name << "=" << val << std::endl;
	return true;
}

bool readConf(std::fstream &f,const std::string &name,std::string &val)
{
	std::regex re(rePattern);

	char line[128];
	while(!f.eof())
	{
		f.getline(line,sizeof(line));

		std::cmatch ma;
		if(std::regex_match(line,ma,re))
		{
			if(ma[1].str()==name)
			{
				val=ma[2].str();
				return true;
			}
		}
	}
	return false;
}

/**
 * @brief Provides reading/writig properties from/to conf-file.
 */
class FileFirstProperty : public IPropertyDefinition
{
public:
	FileFirstProperty(AbstractFileStaticInitPropertiesList* parent,const IPropertyDefinitionPtr &upstream)
		: _parent(parent),
		  _upstream(upstream)
	{
		CLEAR_TIMESPEC(_lastChange);
	}

	// IPropertyDefinition interface
public:
	virtual std::string getName() override
	{
		return _upstream->getName();
	}

	virtual std::string getValue() override
	{
		// TODO Use inotify API to monitor file changes
		fs::path p=_parent->getPropertiesFileName();
		if(fs::exists(p))
		{
			fs::fstream f(p);
			if(!f)
				G2F_EXCEPTION("Can't open file '%1' while reading propery '%2'").arg(p.string()).arg(getName()).throwIt(G2FErrorCodes::BadFileOperation);
			f.exceptions(std::fstream::failbit|std::fstream::badbit);
			bool readed=readConf(f,_upstream->getName(),_value);
			if(readed)
				clock_gettime(CLOCK_REALTIME,&_lastChange);
		}

		if(!ISSET_TIMESPEC(_lastChange))
			return _upstream->getValue();
		return _value;
	}

	virtual G2FError setValue(const std::string &val) override
	{
		fs::path p=_parent->getPropertiesFileName();
		try
		{
			if(!fs::exists(p))
			{
				if(!fs::exists(p.parent_path()))
					fs::create_directories(p.parent_path());
				fs::ofstream newFile(p);
			}
			fs::fstream f(p);
			if(!f)
				G2F_EXCEPTION("Can not open file '%1'").arg(p).throwItSystem(EACCES);
			f.exceptions(std::fstream::failbit|std::fstream::badbit);
			bool changed=updateConf(f,_upstream->getName(),val);
			if(changed)
				clock_gettime(CLOCK_REALTIME,&_lastChange);
		}
		catch(const G2FException &e)
		{
			return e.code();
		}
		catch(const std::exception &e)
		{
			return G2FError(G2FErrorCodes::ErrorModifyConfFile).setDetail(e.what());
		}
		return G2FError();
	}

	virtual std::string getDescription() override
	{
		return _upstream->getDescription();
	}

	virtual IPropertyTypePtr getType() override
	{
		return _upstream->getType();
	}

	virtual bool isRuntimeChange() override
	{
		return _upstream->isRuntimeChange();
	}

	virtual std::string getDefaultValue() override
	{
		return _upstream->getDefaultValue();
	}

	virtual timespec getLastChangeTime() override
	{
		if(!ISSET_TIMESPEC(_lastChange))
			return _upstream->getLastChangeTime();
		return _lastChange;
	}

	virtual bool resetToDefault() override
	{
		// TODO Implement if need
		return false;
	}

private:
	AbstractFileStaticInitPropertiesList *_parent=nullptr;
	IPropertyDefinitionPtr _upstream;
	std::string _value;
	timespec _lastChange;
};




IPropertyDefinitionPtr AbstractFileStaticInitPropertiesList::decorate(const IPropertyDefinitionPtr &p)
{
	return std::make_shared<FileFirstProperty>(this,p);
}
