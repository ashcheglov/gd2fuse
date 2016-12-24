#ifndef APPERROR_H
#define APPERROR_H

#include "utils/decls.h"
#include <boost/system/error_code.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/optional.hpp>

enum G2FErrorCodes
{
	WrongAppArguments,
	HttpReadError,
	HttpTransportError,
	InternalError,
	HttpClientError,
	HttpServerError,
	HttpPermissionError,
	HttpTimeoutError
};

namespace boost
{
namespace system
{
template <>
struct is_error_code_enum<G2FErrorCodes>
		: public boost::true_type {};
}
}

namespace err=boost::system;

err::error_code make_error_code(G2FErrorCodes e);
err::error_condition make_error_condition(G2FErrorCodes e);
const err::error_category& appErrorCategory();


class G2FError : public err::error_code
{
public:
	G2FError();
	G2FError(int val,const err::error_category &cat);
	void setDetail(const std::string &detail);
	const std::string& detail();

	template <class ErrorCodeEnum>
	G2FError(ErrorCodeEnum e) BOOST_SYSTEM_NOEXCEPT
	{
		*this = make_error_code(e);
	}

	template<typename ErrorCodeEnum>
	G2FError &operator=( ErrorCodeEnum val ) BOOST_SYSTEM_NOEXCEPT
	{
		*this = make_error_code(val);
		return *this;
	}

	G2FError &operator=(const err::error_code &ec) BOOST_SYSTEM_NOEXCEPT;

	inline friend bool operator==( const G2FError & lhs,
								   const G2FError & rhs ) BOOST_SYSTEM_NOEXCEPT;


	void clear();
private:
	std::string _detail;
};

#endif // APPERROR_H
