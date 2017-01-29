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

INode *FuseGate::getINode(const char *path)
{
	return _fs->get(path);
}

INode *FuseGate::createFile(const char *fileName, int flags)
{
	return _fs->createNode(fileName,false);
}

IFileSystem &FuseGate::getFS()
{
	return *_fs;
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

