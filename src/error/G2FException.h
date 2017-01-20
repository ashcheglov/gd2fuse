#pragma once

#include <string>
#include <list>
#include "appError.h"
#include <boost/lexical_cast.hpp>
#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_errno,std::string> g2f_error_detail_string;



class G2FException : public virtual boost::exception, public virtual std::exception
{
public:
	friend class G2FExceptionBuilder;

	G2FException(const G2FError &errorCode);

	const G2FError& code() const;
	virtual const char* what() const noexcept;

private:
	void _buildWhat();
	G2FError _error;
	std::string _what;
};



class G2FExceptionBuilder
{
public:
	G2FExceptionBuilder() = default;
	G2FExceptionBuilder(const std::string &message)
		: _message(message)
	{}
	G2FExceptionBuilder& message(const std::string &message);
	G2FExceptionBuilder& reason(const std::string &reason);

	template<typename T>
	G2FExceptionBuilder& arg(T val)
	{
		_args.push_back(boost::lexical_cast<std::string>(val));
		return *this;
	}

	void throwIt(G2FError code);
	G2FException getIt(G2FError code);


private:
	std::string _message;
	std::string _reason;
	std::vector<std::string> _args;

};



#define G2F_EXCEPTION(message)	G2FExceptionBuilder(message)

#define G2F_EXCEPTION_GSTATUS(message,status) G2F_EXCEPTION(message).reason(status.error_message())

