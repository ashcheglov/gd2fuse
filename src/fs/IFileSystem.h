#pragma once

#include "utils/decls.h"
#include "INode.h"
#include <boost/signals2.hpp>

namespace bs2 = boost::signals2;

class IFileSystem
{

public:
	enum RemoveStatus
	{
		RemoveSuccess,
		RemoveNotFound,
		RemoveForbidden
	};

	enum CreateStatus
	{
		CreateSuccess,
		CreateAlreadyExists,
		CreateForbidden,
		CreateBadPath
	};

	class INotifier
	{
	public:
		enum DirectoryChangeType
		{
			Remove,
			Added
		};

		typedef boost::signals2::signal<void (INode&)> OnFileContentChange;
		typedef boost::signals2::signal<void (INode&,int)> OnNodeChange;
		typedef boost::signals2::signal<void (INode&)> OnNodeRemove;
		typedef boost::signals2::signal<void (INode&)> OnNodeCreate;
		typedef boost::signals2::signal<void (INode& dir,DirectoryChangeType type,INode& what)> OnDirectoryChange;

		virtual bs2::connection subscribeToFileContentChange(const OnFileContentChange::slot_type &sub) =0;
		virtual bs2::connection subscribeToNodeChange(const OnNodeChange::slot_type &sub) =0;
		virtual bs2::connection subscribeToNodeRemove(const OnNodeRemove::slot_type &sub) =0;
		virtual bs2::connection subscribeToNodeCreate(const OnNodeCreate::slot_type &sub) =0;
		virtual bs2::connection subscribeToDirectoryChange(const OnDirectoryChange::slot_type &sub) =0;

		virtual ~INotifier(){}
	};

	typedef std::tuple<CreateStatus,INode*> CreateResult;

	virtual INotifier* getNotifier() =0;
	virtual INode* getRoot() =0;
	virtual INode* get(const fs::path &p) =0;
	virtual CreateResult createNode(const fs::path &p,bool isDirectory) =0;
	virtual RemoveStatus removeNode(const fs::path &p) =0;
	virtual void renameNode(const fs::path &oldPath,const fs::path &newPath) =0;
	virtual void replaceNode(const fs::path &pathToReplace,INode &onThis) =0;
	virtual void insertNode(const fs::path &parentPath,INode &that) =0;

	virtual ~IFileSystem() {}
};

G2F_DECLARE_PTR(IFileSystem);
