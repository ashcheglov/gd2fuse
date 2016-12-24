#include <iostream>
#include "utils/log.h"
//#include <googleapis/client/service/client_service.h>
#include "utils/decls.h"
#include "utils/Enum.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "Application.h"

namespace po=boost::program_options;
using namespace std;


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
								"arguments")
		("f",					"foreground operation")
		("s",					"disable multi-threaded operation")
		("o",					po::value<FUSEOpts::O0oList>(),
								"options")
		;
	return ret;
}

FUSEOpts& readFUSEopts(const po::variables_map& vm,FUSEOpts& opts)
{
	opts.debug=vm.count("debug")!=0;
	opts.disableMThread=vm.count("s")!=0;
	opts.foreground=vm.count("f")!=0;

	if(vm.count("mount-point"))
		opts.mountPoint=vm["mount-point"].as<fs::path>();
	if(vm.count("o"))
		opts.ooo=vm["o"].as<FUSEOpts::O0oList>();
	return opts;
}


int main(int argc, char *argv[])
{
	G2F_LOG_INIT();
	typedef vector<string> Strings;
	Enum<Action> action(Service);
	fs::path confPath;
	std::string authCode;
	std::string email;

	// TODO Implement Init/DeInit process
	// TODO Needs possibility setting up umask
	// TODO demonize() app in SERVICE mode
	po::options_description opts("options");
	opts.add_options()
		("help,h",				"help")
		("mode,m",				po::value<Enum<Action> >(&action),
								"mode:\n"
								"  GETAUTH - \tCreate authorization file\n"
								"  SERVICE - \tStart in service mode")
		("version,V",			"print version")
		("debug,d",				"debug output")
		("read-only",			"read only mode")
		("conf-dir",			po::value<fs::path>(&confPath),
								"application conf directory")
		("auth-code",			po::value<std::string>(&authCode),
									"authorization code was obtained from Google")
		("no-ssl-vfy",			"disable verify server sertificates")
		("email",				po::value<std::string>(&email),
								"Google email account")
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
	po::store(po::command_line_parser(argc,argv).
		options(all).positional(p).run(), vm);
	po::notify(vm);

	if(argc==1 || vm.count("help"))
	{
		std::cerr << endl << "usage: " G2F_APP_NAME " mode [opts] [mountpoint]" << endl
				  << opts << endl;
		return Application::fuseHelp();
	}

	Application *app=Application::create(email,argc,argv);
	app->setSSLVerificationDisabled(vm.count("no-ssl-vfy")!=0);
	app->setDebug(vm.count("debug")!=0);
	app->setReadOnly(vm.count("read-only")!=0);
	app->setPathManager(createDefaultPathMapping(&confPath));

	if(action==GetAuth)
		return app->createAuthFile(authCode);
    
	FUSEOpts fuseOpts;
	return app->fuseStart(readFUSEopts(vm,fuseOpts));
}
