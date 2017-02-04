#include "AbstractMemoryStaticInitPropertiesList.h"
#include "IPropertiesList.h"
#include "control/Application.h"
#include "control/types/PropertyTypesRegistry.h"

/**
 * @brief InMemoryProperty set value only in memory - without any persistence
 */
class InMemoryProperty : public IPropertyDefinition
{
public:
	InMemoryProperty(AbstractMemoryStaticInitPropertiesList* parent,const IPropertyDefinitionPtr &upstream)
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
		if(!ISSET_TIMESPEC(_lastChange))
			return _upstream->getValue();
		return _value;
	}

	virtual G2FError setValue(const std::string &val) override
	{
		std::string lv(val);
		AbstractStaticInitPropertiesList::normalizeValue(lv);
		if(!_upstream->getType()->checkValidity(lv))
			return G2FError(EINVAL,err::system_category());

		bool c=false;
		if((ISSET_TIMESPEC(_lastChange) && lv!=_value) || lv!=_upstream->getValue())
			c=true;

		if(c)
		{
			_value=lv;
			clock_gettime(CLOCK_REALTIME,&_lastChange);
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
		bool ret=false;
		if(ISSET_TIMESPEC(_lastChange))
			ret=true;
		CLEAR_TIMESPEC(_lastChange);
		_value.clear();
		return ret;
	}

private:
	AbstractMemoryStaticInitPropertiesList *_parent=nullptr;
	IPropertyDefinitionPtr _upstream;
	std::string _value;
	timespec _lastChange;
};




IPropertyDefinitionPtr AbstractMemoryStaticInitPropertiesList::decorate(const IPropertyDefinitionPtr &p)
{
	return std::make_shared<InMemoryProperty>(this,p);
}
