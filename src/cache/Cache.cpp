#include "Cache.h"

Cache::Cache()
{}

INode *Cache::findByPath(const fs::path &path)
{
	boost::shared_lock<boost::shared_mutex> lock{_m};
	PathNodes::iterator it=_pathNodes.find(path);
	if(it!=_pathNodes.end())
		return it->second;
	return nullptr;
}

INode *Cache::findById(const std::string &id)
{
	boost::shared_lock<boost::shared_mutex> lock{_m};
	IdNodes::iterator it=_idNodes.find(id);
	if(it!=_idNodes.end())
		return it->second;
	return nullptr;
}

Cache &Cache::insert(const fs::path &path, const std::string &id, INode *node)
{
	boost::lock_guard<boost::shared_mutex> lock{_m};
	_pathNodes[path]=node;
	_idNodes[id]=node;
	return *this;
}
