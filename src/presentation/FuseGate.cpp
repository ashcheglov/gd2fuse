#include "handler.h"
#include "FuseGate.h"
#include "utils/log.h"
#include "utils/ExponentialBackoff.h"
#include "error/G2FException.h"
#include "error/appError.h"
#include "utils/assets.h"
#include "fs/ConfFileSystem.h"
#include "fs/JoinedFileSystem.h"


FuseGate::FuseGate(IProviderSession &ps)
	: _ps(ps)
{}

int FuseGate::run(const FUSEOpts &fuseOpts)
{
	fuse_args args=FUSE_ARGS_INIT(0,NULL);
	fuse_opt_add_arg(&args,G2F_APP_NAME);
	if(fuseOpts.debug)
		fuse_opt_add_arg(&args,"-d");
	if(fuseOpts.foreground)
		fuse_opt_add_arg(&args,"-f");
	if(fuseOpts.disableMThread)
		fuse_opt_add_arg(&args,"-s");
	for(const std::string& o : fuseOpts.ooo)
	{
		fuse_opt_add_arg(&args,"-o");
		fuse_opt_add_arg(&args,o.c_str());
	}
	if(!fuseOpts.mountPoint.empty())
		fuse_opt_add_arg(&args,fuseOpts.mountPoint.string().c_str());

	fuse_operations g2f_oper;
	g2f_init_ops(&g2f_oper);

	const IFileSystemPtr &confFS=createConfigurationFS(_ps.getConfiguration());
	const auto &cd=_ps.getConfiguration()->getProperty("control_dir");

	JoinedFileSystemFactory jfsf(S_IRWXU|S_IRGRP|S_IXGRP);
	// TODO Make permissions configurable
	jfsf.mount("/",_ps.getFileSystem(),S_IRWXU|S_IRGRP|S_IXGRP);
	jfsf.mount(*cd,confFS,S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP);
	_fs=jfsf.build();

	return fuse_main(args.argc, args.argv, &g2f_oper, this);
}

IFileSystem &FuseGate::getFS()
{
	return *_fs;
}

INode *FuseGate::getINode(const char *path)
{
	try
	{
		return _fs->get(path);
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
	}
	return nullptr;
}

posix_error_code FuseGate::openContent(const char *path, int flags, IContentHandle *&outChn)
{
	posix_error_code err=0;
	try
	{
		INode *n=_fs->get(path);
		if(!n)
			return ENOENT;
		if(n->isFolder())
			return EISDIR;
		IContentHandle *chn=n->openContent(flags);
		assert(chn!=0);
		err=chn->getError();
		if(err)
			delete chn;
		outChn=chn;
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
		return e.code().default_error_condition().value();
	}
	return err;
}

posix_error_code FuseGate::createFile(const char *fileName, INode *&f)
{
	try
	{
		fs::path p(fileName);

		f=_fs->get(p);
		if(f)
		{
			if(f->isFolder())
				return EISDIR;
		}
		else
		{
			INode *n=_fs->get(p.parent_path().c_str());
			if(!n->isFolder())
				return ENOTDIR;
		}
		if(!f)
		{
			IFileSystem::CreateStatus s;
			std::tie(s,f)=_fs->createNode(fileName,false);

			switch(s)
			{
			case IFileSystem::CreateSuccess:
				return 0;
			case IFileSystem::CreateAlreadyExists:
				return EEXIST;
			case IFileSystem::CreateForbidden:
				return EPERM;
			case IFileSystem::CreateBadPath:
				return EISDIR;
			default:
				assert(false);
				return EACCES;
			}
		}
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
		return e.code().default_error_condition().value();
	}
	return 0;
}

posix_error_code FuseGate::removeINode(const char *path)
{
	try
	{
		IFileSystem::RemoveStatus ret=_fs->removeNode(path);
		if(ret==IFileSystem::RemoveStatus::RemoveSuccess)
			return 0;
		if(ret==IFileSystem::RemoveStatus::RemoveNotFound)
			return ENOENT;
		//if(ret==IFileSystem::RemoveStatus::Forbidden)
		return EPERM;
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
		return e.code().default_error_condition().value();
	}
	return 0;
}

posix_error_code FuseGate::createDir(const char *dirName, mode_t mode, INode *&f)
{
	try
	{
		fs::path path(dirName);

		IFileSystem::CreateStatus s;
		std::tie(s,f)=_fs->createNode(dirName,true);

		switch(s)
		{
		case IFileSystem::CreateSuccess:
			return 0;
		case IFileSystem::CreateAlreadyExists:
			return EEXIST;
		case IFileSystem::CreateForbidden:
			return EPERM;
		case IFileSystem::CreateBadPath:
			return ENOTDIR;
		default:
			assert(false);
			return EACCES;
		}
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
		return e.code().default_error_condition().value();
	}
	return 0;
}

posix_error_code FuseGate::rename(const char *oldName, const char *newName)
{
	try
	{
		_fs->renameNode(oldName,newName);
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
		return e.code().default_error_condition().value();
	}
	return 0;
}

int FuseGate::fuseHelp()
{
	fuse_args args=FUSE_ARGS_INIT(0,NULL);
	fuse_opt_add_arg(&args,G2F_APP_NAME);
	fuse_opt_add_arg(&args,"-ho");	// Omit fuse help header
	fuse_operations ops;
	memset(&ops,0,sizeof(fuse_operations));
	return fuse_main(args.argc, args.argv, &ops, 0);
}

