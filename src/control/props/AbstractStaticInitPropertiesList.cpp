#include "AbstractStaticInitPropertiesList.h"
#include "IPropertiesList.h"
#include <map>
#include "control/Application.h"
#include "control/types/PropertyTypesRegistry.h"


//
//			Property
//
////////////////////////////////////
class StaticProperty : public IPropertyDefinition
{
public:
	StaticProperty(const AbstractStaticInitPropertiesList::PropDefi& p,const IPropertyTypePtr &type)
		: _p(p),
		  _type(type)
	{}

	// IPropertyDefinition interface
public:
	virtual std::string getName() override
	{
		return _p.name;
	}

	virtual std::string getValue() override
	{
		return _p.defaultValue;
	}

	virtual G2FError setValue(const std::string &val) override
	{
		return G2FError(G2FErrorCodes::NotSupported);
	}

	virtual std::string getDescription() override
	{
		return _p.description;
	}

	virtual IPropertyTypePtr getType() override
	{
		return _type;
	}

	virtual bool isRuntimeChange() override
	{
		return _p.changeRt;
	}

	virtual std::string getDefaultValue() override
	{
		return _p.defaultValue;
	}

	virtual timespec getLastChangeTime() override
	{
		return Application::instance()->startTime();
	}

	virtual bool resetToDefault() override
	{
		return false;
	}

private:
	const AbstractStaticInitPropertiesList::PropDefi& _p;
	IPropertyTypePtr _type;
};




//
//	Implementation
//
////////////////////////////////////
class AbstractStaticInitPropertiesList::Impl
{
public:
	typedef AbstractStaticInitPropertiesList::PropDefi PropDefi;
	typedef AbstractStaticInitPropertiesList::EnumDefi EnumDefi;
	typedef std::map<std::string,IPropertyDefinitionPtr> PropsMap;

	Impl(AbstractStaticInitPropertiesList *parent)
	{
		const auto &sProps=parent->getPropertiesDefinition();
		const auto &sEnums=parent->getEnumsDefinition();
		for(size_t i=0;i<sProps.second;++i)
		{
			const AbstractStaticInitPropertiesList::PropDefi& p=sProps.first[i];

			IPropertyTypePtr type;
			if(p.type!=IPropertyType::ENUM)
				type=PropertyTypesRegistry::getInstance().getType(p.type);
			else
			{
				const EnumDefi *ed=findEnumDefi(sEnums,p.enumCode);
				assert(ed);

				PropertyTypesRegistry::EnumEntries es;
				for(auto e : ed->entries)
				{
					PropertyTypesRegistry::EnumEntry ee;
					ee.entry=e.entry;
					ee.description=e.description;
					es.push_back(ee);
				}
				type=PropertyTypesRegistry::getInstance().registerEnum(p.enumCode,es);
			}

			assert(type);
			IPropertyDefinitionPtr pd=parent->decorate(std::make_shared<StaticProperty>(p,type));
			props.insert(std::make_pair(pd->getName(),pd));
		}
	}

	const EnumDefi *findEnumDefi(const std::pair<const EnumDefi*,size_t> &sEnums, const char *code)
	{
		assert(code);

		for(size_t i=0;i<sEnums.second;++i)
		{
			if(!strcmp(sEnums.first[i].code,code))
			{
				return sEnums.first+i;
			}
		}
		return nullptr;
	}

	class It : public IPropertyIterator
	{
	public:
		It(const PropsMap &map)
		{
			_it=map.begin();
			_itEnd=map.end();
		}

		// IPropertyIterator interface
	public:
		virtual bool hasNext() override
		{
			return _it!=_itEnd;
		}
		virtual IPropertyDefinitionPtr next() override
		{
			return (_it++)->second;
		}

		PropsMap::const_iterator _it,_itEnd;
	};

	IPropertyIteratorPtr getIterator()
	{
		return std::make_shared<It>(props);
	}

	PropsMap props;
	bool isInit=false;
};



#define G2F_PROP_IMPL_LAZY(impl) if(!impl) impl=std::make_shared<AbstractStaticInitPropertiesList::Impl>(this);


//
//	AbstractStaticInitPropertiesList
//
///////////////////////////////////////////////
AbstractStaticInitPropertiesList::AbstractStaticInitPropertiesList()
{}

IPropertyDefinitionPtr AbstractStaticInitPropertiesList::findProperty(const std::string &name)
{
	G2F_PROP_IMPL_LAZY(_impl);
	auto it=_impl->props.find(name);
	if(it!=_impl->props.end())
		return it->second;
	return IPropertyDefinitionPtr();
}

bool AbstractStaticInitPropertiesList::isEmpty()
{
	G2F_PROP_IMPL_LAZY(_impl);
	return _impl->props.empty();
}

size_t AbstractStaticInitPropertiesList::getPropertiesSize()
{
	G2F_PROP_IMPL_LAZY(_impl);
	return _impl->props.size();
}

IPropertyIteratorPtr AbstractStaticInitPropertiesList::getProperties()
{
	G2F_PROP_IMPL_LAZY(_impl);
	return _impl->getIterator();
}

IPropertyDefinitionPtr AbstractStaticInitPropertiesList::decorate(const IPropertyDefinitionPtr &p)
{
	return p;
}

