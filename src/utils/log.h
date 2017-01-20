#pragma once

#ifdef G2F_USE_LOG

#ifndef G2F_STATIC_LINK
	#define BOOST_LOG_DYN_LINK
#endif

#include <boost/log/attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/logger.hpp>

namespace logging = boost::log;

void g2fLogInit();
extern bool g2fLogInitFlag;

BOOST_LOG_GLOBAL_LOGGER(g2flog,logging::sources::logger_mt)

#ifndef G2F_LOG_BOOSTLOG_DONT_LOG_SCOPE
	#define G2F_LOG_SCOPE() BOOST_LOG_FUNCTION()
	#define G2F_LOG_SCOPE_NAMED(name) BOOST_LOG_NAMED_SCOPE(name)
#else
	#define G2F_LOG_SCOPE()
	#define G2F_LOG_SCOPE_NAMED(name)
#endif

#define G2F_LOG_INIT()	g2fLogInit()
#define G2F_LOG(message) if(g2fLogInitFlag) BOOST_LOG(g2flog::get()) << message
#define G2F_LOG_TAG(tag,message)  if(g2fLogInitFlag) BOOST_LOG(g2flog::get()) << logging::add_value("Tag", tag) << message


#else	// G2F_USE_LOG

#define G2F_LOG_SCOPE()
#define G2F_LOG_SCOPE_NAMED(name)
#define G2F_LOG_INIT()
#define G2F_LOG(message)
#define G2F_LOG_TAG(tag,message)

#endif	//	G2F_USE_LOG

