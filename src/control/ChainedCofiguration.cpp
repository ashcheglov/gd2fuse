#include "ChainedCofiguration.h"



class ChainedPropertyIterator : public IPropertyIterator
{
public:
	ChainedPropertyIterator(const IPropertyIteratorPtr &upstream,const IPropertyIteratorPtr &current)
		: _upstream(upstream),
		  _current(current)
	{
		_it=&_upstream;
	}

	// IPropertyIterator interface
public:
	virtual bool hasNext() override
	{
		bool ret=(*_it)->hasNext();
		if(ret)
			return ret;

		// Is something more?
		if(_it==&_upstream)
			return _current->hasNext();
		return ret;
	}
	virtual IPropertyDefinitionPtr next() override
	{
		const auto &ret=(*_it)->next();
		if(!(*_it)->hasNext() && _it==&_upstream)
			_it=&_current;
		return ret;
	}

private:
	IPropertyIteratorPtr _upstream;
	IPropertyIteratorPtr _current;
	IPropertyIteratorPtr *_it=nullptr;
};






class ChainedPropertiesList : public IPropertiesList
{

public:
	ChainedPropertiesList(const IPropertiesListPtr &upstream,const IPropertiesListPtr &current)
		: _upstream(upstream),
		  _current(current)
	{}

	// IPropertiesList interface
public:
	virtual std::string getName() override
	{
		return _upstream->getName()+'/'+_current->getName();
	}
	virtual IPropertyDefinitionPtr findProperty(const std::string &name) override
	{
		const auto &ret=_current->findProperty(name);
		if(ret)
			return ret;
		return _upstream->findProperty(name);
	}
	virtual bool isEmpty() override
	{
		return _current->isEmpty() && _upstream->isEmpty();
	}
	virtual size_t getPropertiesSize() override
	{
		return _current->getPropertiesSize()+_upstream->getPropertiesSize();
	}
	virtual IPropertyIteratorPtr getProperties() override
	{
		return std::make_shared<ChainedPropertyIterator>(_upstream->getProperties(),_current->getProperties());
	}

private:
	IPropertiesListPtr _upstream;
	IPropertiesListPtr _current;
};





ChainedConfiguration::ChainedConfiguration(const IConfigurationPtr &upstream, const IConfigurationPtr &current)
	: _upstream(upstream),
	  _current(current)
{}

IPathManagerPtr ChainedConfiguration::getPaths()
{
	return _current->getPaths();
}

boost::optional<std::string> ChainedConfiguration::getProperty(const std::string &name)
{
	boost::optional<std::string> ret=_current->getProperty(name);
	if(ret)
		return ret;
	return _upstream->getProperty(name);
}

G2FError ChainedConfiguration::setProperty(const std::string &name, const std::string &value)
{
	return _current->setProperty(name,value);
}

IPropertiesListPtr ChainedConfiguration::getProperiesList()
{
	return std::make_shared<ChainedPropertiesList>(_upstream->getProperiesList(),_current->getProperiesList());
}
