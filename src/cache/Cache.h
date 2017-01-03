#pragma once

#include "utils/decls.h"
#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>

class Node;

// TODO Implement LRU cache
class Cache
{
public:
	Cache();
	Node *findByPath(const fs::path &path);
	Node *findById(const std::string &id);

	bool remove(const Node *node);
	Cache &insert(const fs::path& path, const std::string &id, Node *node);

private:
	boost::shared_mutex _m;
	typedef boost::unordered_map<fs::path,Node*> PathNodes;
	typedef boost::unordered_map<std::string,Node*> IdNodes;
	//typedef boost::unordered_map<Node*,std::string> Nodes;
	PathNodes _pathNodes;
	IdNodes _idNodes;
};

