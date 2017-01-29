#pragma once

#include "utils/decls.h"
#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "fs/INode.h"

// TODO Implement LRU cache
class Cache
{
public:
	Cache();
	INode *findByPath(const fs::path &path);
	INode *findById(const std::string &id);

	bool remove(const INode *node);
	Cache &insert(const fs::path& path, const std::string &id, INode *node);

	void slotNodeRemoved(INode &node);

private:
	boost::shared_mutex _m;
	typedef boost::unordered_map<fs::path,INode*> PathNodes;
	typedef boost::unordered_map<std::string,INode*> IdNodes;
	typedef boost::unordered_map<INode*,fs::path> Nodes;
	PathNodes _pathNodes;
	IdNodes _idNodes;
	Nodes _nodes;
};

