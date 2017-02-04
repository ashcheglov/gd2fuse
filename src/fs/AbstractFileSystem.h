#pragma once

#include <string>
#include <boost/ptr_container/ptr_list.hpp>
#include "utils/decls.h"
#include "IFileSystem.h"
#include "cache/Cache.h"
#include "control/IConfiguration.h"
#include "ContentManager.h"


/**
 * @brief Abstract class provides generalization cloud file system
 *
 * To get fully functional cloud file system you should
 * implement pure abstract protected methods
 *
 *********************************************************************/
class AbstractFileSystem : public IFileSystem
{
public:

	/**
	 * @brief The Node class
	 *
	 *************************/
	class Node : public INode
	{
	public:
		enum Field
		{
			Name	= 1 << 0,
			Parent	= 1 << 1,
			Id		= 1 << 2,
			Time	= 1 << 3,
			Content = 1 << 4
		};

		friend class AbstractFileSystem;

		typedef boost::ptr_list<Node> NodeList;
		typedef NodeList::iterator iterator;
		typedef NodeList::const_iterator const_iterator;

	public:
		// Simple model: regular tree. Today we don't support links
		Node(AbstractFileSystem *tree,Node *parent);
		// INode interface
		virtual NodeType getNodeType() override;
		virtual void fillAttr(struct stat &statbuf) override;
		virtual std::string getId() override;
		virtual void setId(const std::string &id) override;
		virtual boost::filesystem::path getName() override;
		virtual void setName(const fs::path &name) override;
		virtual void setSize(size_t size) override;
		virtual size_t getSize() override;
		virtual void setTime(TimeAttrib what, timespec value) override;
		virtual timespec getTime(TimeAttrib what) override;
		virtual IDirectoryIteratorPtr getDirectoryIterator() override;
		virtual IContentHandle *openContent(int flags) override;
		virtual posix_error_code truncate(off_t newSize) override;

		MD5Signature getMD5();
		void setMD5(const MD5Signature &md5);
		void setFileType(NodeType type);
		Node *getParent();

		// Remove all (what==null) or particular (what!=null) inferrior nodes
		void removeNodes(Node *what=nullptr);

		size_t size() const;
		Node *find(fs::path::iterator &it,const fs::path::iterator &end);
		void clearTime(TimeAttrib what);
		void addNext(Node *value);
		bool importNreplace(INode &value, const fs::path &newName=fs::path(), Node *toReplace=nullptr);
		int patch(Node &dataSource, int patchField);
		bool detachNode(Node *node);
		void attachNode(Node *node);

		inline iterator begin() { return _next.begin(); }
		inline const_iterator begin() const { return _next.begin(); }
		inline iterator end() { return _next.end(); }
		inline const_iterator end() const { return _next.end(); }

	private:
		class FileHandle;

		AbstractFileSystem *_tree=nullptr;
		bool _dirFilled=false;
		NodeList _next;
		std::string _id;
		fs::path _name;
		size_t _size = 0;
		NodeType _fileType=NodeType::Binary;
		MD5Signature _md5;
		Node *_parent=0;

		timespec _lastAccess;        // time of last access
		timespec _lastModification;  // time of last modification
		timespec _lastChange;        // time of last status change
	};

public:
	AbstractFileSystem(const ContentManagerPtr &cm);
	~AbstractFileSystem();

	void fillDir(Node *dir);

	virtual INotifier *getNotifier() override;
	virtual Node *getRoot() override;
	virtual INode *get(const fs::path &path) override;
	virtual CreateResult createNode(const fs::path &path,bool isDirectory) override;
	virtual RemoveStatus removeNode(const fs::path &path) override;
	virtual void renameNode(const boost::filesystem::path &oldPath, const boost::filesystem::path &newPath) override;
	virtual void replaceNode(const fs::path &pathToReplace,INode &onThis);
	virtual void insertNode(const fs::path &parentPath,INode &that);

	Node *getNode(const fs::path &path,bool throwIfMissed);

protected:
	virtual void cloudFetchMeta(Node &dest) =0;
	virtual std::vector<std::string> cloudFetchChildrenList(const std::string &parentId) =0;
	virtual void cloudCreateMeta(Node &dest) =0;
	virtual ContentManager::IReaderUPtr cloudReadMedia(Node &node) =0;
	virtual void cloudUpdate(Node &node,int patchFields,const std::string &mediaType="",ContentManager::IReader *content=nullptr) =0;
	virtual void cloudRemove(Node &node) =0;

	void updateNodeContent(INode &n);
	//Node *remove(const fs::path &path);
	//void move(const fs::path &from,const fs::path &to);

private:
	friend class Node;
	class Notifier;
	friend class Notifier;

	sptr<Node> _root;
	uptr<Cache> _cache;
	uptr<Notifier> _notifier;
	ContentManagerPtr _cm;
};
