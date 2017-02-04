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
	_nodes[node]=path;
	return *this;
}

void Cache::slotNodeRemoved(INode &node)
{
	// clear the cache
	auto itNode=_nodes.find(&node);
	if(itNode!=_nodes.end())
	{
		auto pathIt=_pathNodes.find(itNode->second);
		if(pathIt!=_pathNodes.end())
			_pathNodes.erase(pathIt);

		auto idIt=_idNodes.find(node.getId());
		if(idIt!=_idNodes.end())
			_idNodes.erase(idIt);
		_nodes.erase(itNode);
	}
}

void Cache::slotNodeChanged(INode &node, int)
{
	// I don't know enum Node::Fields here...
	slotNodeRemoved(node);
}
