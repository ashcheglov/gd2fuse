#include "handler.h"
#include "FuseGate.h"
#include "utils/log.h"
#include "utils/ExponentialBackoff.h"
#include "error/G2FException.h"
#include "error/appError.h"
#include "utils/assets.h"


FuseGate::FuseGate(IProviderSession &ps, const boost::filesystem::path &workDir)
	: _ps(ps),
	  _workDir(workDir)
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
	const IDataProviderPtr &dp=_ps.createDataProvider();
	_tree.setDataProvider(dp);
	_cManager.init(_workDir,dp);
	return fuse_main(args.argc, args.argv, &g2f_oper, this);
}

Node *FuseGate::getMeta(const char *path)
{
	return _tree.get(path);
}

Tree &FuseGate::metaTree()
{
	return _tree;
}

Tree::IDirectoryIteratorUPtr FuseGate::getDirectoryIterator(Node *meta)
{
	return Tree::IDirectoryIteratorUPtr(_tree.getDirectoryIterator(meta));
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

int FuseGate::openContent(Node *meta, int flags, uint64_t &fd)
{
	try
	{
		fd=_cManager.openFile(_tree.id(meta),flags);
	}
	catch(G2FException &e)
	{
		return e.code().default_error_condition().value();
	}
	catch(err::system_error &e)
	{
		return e.code().default_error_condition().value();
	}
	return 0;
}

int FuseGate::readContent(uint64_t fd, char *buf, size_t len, off_t offset)
{
	try
	{
		return _cManager.readContent(fd,buf,len,offset);
	}
	catch(G2FException &e)
	{
		return e.code().default_error_condition().value();
	}
	catch(err::system_error &e)
	{
		return e.code().default_error_condition().value();
	}
	return 0;
}

int FuseGate::closeContent(uint64_t fd)
{
	try
	{
		return _cManager.closeFile(fd);
	}
	catch(G2FException &e)
	{
		return e.code().default_error_condition().value();
	}
	catch(err::system_error &e)
	{
		return e.code().default_error_condition().value();
	}
	return 0;
}

