#include <iostream>
#include "utils/log.h"
#include "utils/decls.h"
#include "utils/Enum.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "control/Application.h"
#include "control/paths/PathManager.h"

namespace po=boost::program_options;
using namespace std;


// TODO Make cleaning mode (account info, cache, conf directories)
enum Action
{
	GetAuth,
	Service
};

BEGIN_ENUM_MAP(Action)
	ENUM_MAP_ITEM_STR(GetAuth,	"GETAUTH")
	ENUM_MAP_ITEM_STR(Service,	"SERVICE")
END_ENUM_MAP


po::options_description createFUSEopts()
{
	po::options_description ret("FUSE options");
	ret.add_options()
		("mount-point",			po::value<fs::path>(),
								G2FMESSAGE("arguments"))
		("-f",					G2FMESSAGE("foreground operation"))
		("-s",					G2FMESSAGE("disable multi-threaded operation"))
		("-o",					po::value<FUSEOpts::O0oList>(),
								G2FMESSAGE("options"))
		;

	return ret;
}

FUSEOpts& readFUSEopts(const po::variables_map& vm,FUSEOpts& opts)
{
	opts.debug=vm.count("debug")!=0;
	opts.disableMThread=vm.count("-s")!=0;
	opts.foreground=vm.count("-f")!=0;

	if(vm.count("mount-point"))
		opts.mountPoint=vm["mount-point"].as<fs::path>();
	if(vm.count("-o"))
		opts.ooo=vm["-o"].as<FUSEOpts::O0oList>();
	return opts;
}


int main(int argc, char *argv[])
{
	try
	{
		G2F_LOG_INIT();
		Enum<Action> action(Service);
		fs::path confPath;
		std::string authCode;
		std::string email;
		Application::ProviderArgs prArgs;

		// TODO demonize() app in SERVICE mode
		po::options_description opts("options");
		opts.add_options()
			("help,h",				G2FMESSAGE("help"))
			("mode,m",				po::value<Enum<Action> >(&action),
									G2FMESSAGE("mode:") "\n"
									"  GETAUTH - \t" G2FMESSAGE("Start authorization process") "\n"
									"  SERVICE - \t" G2FMESSAGE("Start as service"))
			("property,p",			po::value<Application::ProviderArgs>(&prArgs),G2FMESSAGE("Redefined property value"))
			("version,V",			G2FMESSAGE("print version"))
			("debug,d",				G2FMESSAGE("debug output"))
			("read-only",			G2FMESSAGE("read only mode"))
			("conf-dir",			po::value<fs::path>(&confPath),
									G2FMESSAGE("optional application conf directory"))
			("auth-code",			po::value<std::string>(&authCode),
									G2FMESSAGE("authorization code was obtained from cloud provider"))
			("email",				po::value<std::string>(&email),
									G2FMESSAGE("Provider email account"))
			;

/*
libfuse
general options:
	-o opt,[opt...]        mount options
	-h   --help            print help
	-V   --version         print version

FUSE options:
	-d   -o debug          enable debug output (implies -f)
	-f                     foreground operation
	-s                     disable multi-threaded operation
*/
		po::options_description all("all");
		all.add(opts).add(createFUSEopts());

		po::positional_options_description p;
		p.add("mount-point",1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc,argv).options(all).positional(p).run(), vm);
		po::notify(vm);

		if(argc==1 || vm.count("help"))
		{
			std::cerr << "Connects cloud drives to FUSE" << std::endl << std::endl << "usage: " G2F_APP_NAME " mode [opts] [mountpoint]" << endl
					  << opts << endl;
			return Application::fuseHelp();
		}

		Application *app=Application::create(email,argc,argv,confPath);
		app->setDebug(vm.count("debug")!=0);
		app->setReadOnly(vm.count("read-only")!=0);

		app->setProviderArgs(prArgs);

		if(action==GetAuth)
			return app->createAuthFile(authCode);

		FUSEOpts fuseOpts;
		return app->fuseStart(readFUSEopts(vm,fuseOpts));
	}
	catch(const G2FException &e)
	{
		std::cerr << e.what() << std::endl;
		return e.code().value();
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 128;
	}
}
