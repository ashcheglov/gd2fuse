#pragma once

#include "utils/decls.h"
#include "INode.h"
#include <boost/signals2.hpp>

namespace bs2 = boost::signals2;

class IFileSystem
{

public:

	class INotify
	{
	public:
		typedef boost::signals2::signal<void (INode*)> OnContentChange;

		virtual bs2::connection subscribeToContentChange(const OnContentChange::slot_type &sub) =0;

		virtual ~INotify(){}
	};

	virtual INotify* getNotifier() =0;
	virtual INode* getRoot() =0;
	virtual INode* get(const fs::path &p) =0;
	virtual INode* createNode(const fs::path &p,bool isDirectory) =0;

	virtual ~IFileSystem() {}
};

G2F_DECLARE_PTR(IFileSystem);
