#include "log.h"
#include <iostream>

#ifdef G2F_USE_LOG

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/regex.hpp>	// Необходимо для включения библиотеки regex

#include <boost/log/common.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/filesystem.hpp>


namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;
namespace fs=boost::filesystem;

BOOST_LOG_GLOBAL_LOGGER_DEFAULT(g2flog,src::logger_mt)

//BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());

//BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr,"Tag",std::string)
//BOOST_LOG_ATTRIBUTE_KEYWORD(scope,"Scope",attrs::named_scope::value_type)
//BOOST_LOG_ATTRIBUTE_KEYWORD(timeline,"TimeStamp",attrs::local_clock::value_type)

bool g2fLogInitFlag=false;
void g2fLogInit()
{
	const boost::shared_ptr<logging::core>& core=logging::core::get();
	core->set_logging_enabled(false);
	const char* logSettings=getenv("G2F_LOG_SETTINGS");
	if(logSettings)
	{
		std::ifstream file(logSettings);
		if(file)
		{
			//core->set_logging_enabled(true);
			logging::init_from_stream(file);
#ifndef G2F_LOG_BOOSTLOG_DONT_LOG_SCOPE
			core->add_global_attribute("Scope",attrs::named_scope());
#endif
			// "LineID", "TimeStamp", "ProcessID" and "ThreadID"
			logging::add_common_attributes();
			//core->add_global_attribute("TimeStamp",attrs::local_clock());
			//core->add_global_attribute("ThreadID",attrs::current_thread_id());
			//core->add_global_attribute("ProcessID",attrs::current_process_id());
			g2fLogInitFlag=true;
		}
		else
			std::cerr << std::endl << "Error init log settings from file '" << logSettings << "'. Log was disabled";
	}
	else
	{
		const char* logFile=getenv("G2F_LOG_FILE");
		if(logFile)
		{
			fs::path path(logFile);
			try
			{
				core->set_logging_enabled(true);
				core->add_global_attribute("TimeStamp",attrs::local_clock());
				core->add_global_attribute("ThreadID",attrs::current_thread_id());
				// Create a backend and attach a couple of streams to it
				boost::shared_ptr<sinks::text_ostream_backend> backend=boost::make_shared<sinks::text_ostream_backend>();
				//backend->add_stream(boost::shared_ptr< std::ostream >(&std::clog, logging::empty_deleter()));

				if(!path.parent_path().empty() && !fs::exists(path.parent_path()))
				{
					fs::create_directories(path.parent_path());
				}
				backend->add_stream(boost::shared_ptr<fs::ofstream >(new fs::ofstream(path)));

				// Enable auto-flushing after each log record written
				backend->auto_flush(true);

				// Wrap it into the frontend and register in the core.
				// The backend requires synchronization in the frontend.
				typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
				boost::shared_ptr< text_sink > sink(new text_sink(backend));
				sink->set_formatter
					(
						expr::stream
						<< '[' << expr::format_date_time< boost::posix_time::ptime >("TimeStamp","%d.%m.%Y %H:%M:%S.%f")
						<< "] [" << expr::attr< attrs::current_thread_id::value_type >("ThreadID")
						<< ']'
						<< expr::if_(expr::has_attr("Tag"))
						[
							expr::stream
							<< " '" << expr::attr< std::string >("Tag")
							<< '\''
						]
#ifndef G2F_LOG_BOOSTLOG_DONT_LOG_SCOPE
				<< expr::if_(expr::has_attr("Scope"))
					[
						//	%n	Scope name	"void foo()"
						//	%f	Source file name of the scope	"foo.cpp"
						//	%l	Line number in the source file	"45"
						expr::stream
						<< " { " << expr::format_named_scope("Scope",keywords::format = "%n")
						<< " }"
					]
#endif
				<< ": " << expr::smessage
					);
				core->add_sink(sink);
#ifndef G2F_LOG_BOOSTLOG_DONT_LOG_SCOPE
				core->add_global_attribute("Scope",attrs::named_scope());
#endif
				g2fLogInitFlag=true;
			}
			catch(const boost::system::system_error& e)
			{
				std::cerr << e.what() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Cannot open log-file '" << path << '\''<< std::endl;
			}
		}
	}
}

#endif	// #G2F_USE_LOG
