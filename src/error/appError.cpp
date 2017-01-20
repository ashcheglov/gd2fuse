#include "appError.h"
#include "utils/decls.h"

/*
static struct CodeDef
{
	G2FErrorCodes code;
	const char *desc;
}
codes[]=
{
	WrongAppArguments,		G2FMESSAGE("Wrong application arguments")
}*/

// TODO Check default_category()
class G2FErrorCategory : public err::error_category
{
	// error_category interface
public:
	virtual const char *name() const noexcept override
	{
		return G2F_APP_NAME;
	}

	virtual std::string message(int code) const override
	{
		switch (code)
		{
		case WrongAppArguments:
			return G2FMESSAGE("Wrong application arguments");
		case HttpReadError:
			return G2FMESSAGE("Error of reading HTTP data stream");
		case HttpTransportError:
			return G2FMESSAGE("HTTP transport error");
		case InternalError:
			return G2FMESSAGE("Internal error");
		case HttpClientError:
			return G2FMESSAGE("HTTP client error");
		case HttpServerError:
			return G2FMESSAGE("HTTP transport error");
		case HttpPermissionError:
			return G2FMESSAGE("HTTP permission error");
		case HttpTimeoutError:
			return G2FMESSAGE("HTTP timeout error");
		case NotImplemented:
			return G2FMESSAGE("Not implemented yet");
		case NotSupported:
			return G2FMESSAGE("Feature is not supported");
		case CouldntDetermineProvider:
			return G2FMESSAGE("Could not detemine cloud provider by account name");
		case PropertyNotFound:
			return G2FMESSAGE("Unknown property");
		case BadFileOperation:
			return G2FMESSAGE("Fail to work with file");
		case UnknownEnvVariable:
			return G2FMESSAGE("Can not find environment variable");
		case PathError:
			return G2FMESSAGE("Error in path value");
		case AuthenticationFailed:
			return G2FMESSAGE("Authentication failed");
		case MD5Error:
			return G2FMESSAGE("MD5 error");
		}
		return G2FMESSAGE("Unknown error");
	}

	virtual bool equivalent(int code,const err::error_condition &condition) const  BOOST_SYSTEM_NOEXCEPT override
	{
		if(condition.category()==err::system_category())
		{
			int cond=condition.value();
			switch (code)
			{
			case PathError:
				{
					if(cond==err::errc::bad_address)
						return true;
				}
				break;
			case PropertyNotFound:
			case CouldntDetermineProvider:
			case WrongAppArguments:
				{
					if(cond==err::errc::invalid_argument)
						return true;
				}
				break;
			case ErrorModifyConfFile:
			case BadFileOperation:
			case HttpReadError:
			case HttpTransportError:
			case HttpClientError:
			case HttpServerError:
				{
					if(cond==err::errc::io_error)
						return true;
				}
				break;
			case AuthenticationFailed:
			case HttpPermissionError:
				{
					if(cond==err::errc::permission_denied)
						return true;
				}
				break;
			case HttpTimeoutError:
				{
					if(cond==err::errc::timed_out)
						return true;
				}
				break;
			case NotSupported:
			case NotImplemented:
				{
					if(cond==err::errc::not_supported)
						return true;
				}
				break;
			}
		}
		return false;
	}
};


err::error_code make_error_code(G2FErrorCodes e)
{
  return err::error_code(static_cast<int>(e),appErrorCategory());
}

err::error_condition make_error_condition(G2FErrorCodes e)
{
  return err::error_condition(static_cast<int>(e),appErrorCategory());
}

const err::error_category &appErrorCategory()
{
	static G2FErrorCategory g2fCategory;
	return g2fCategory;
}

G2FError::G2FError()
	: err::error_code(0,appErrorCategory())
{}

G2FError::G2FError(int val, const boost::system::error_category &cat)
	: err::error_code(val,cat)
{}

G2FError &G2FError::setDetail(const std::string &detail)
{
	_detail=detail;
	return *this;
}

const std::string &G2FError::getDetail()
{
	return _detail;
}

G2FError &G2FError::operator=(const boost::system::error_code &ec) BOOST_SYSTEM_NOEXCEPT
{
	this->assign(ec.value(),ec.category());
	return *this;
}

bool operator==(const G2FError &lhs, const G2FError &rhs) BOOST_SYSTEM_NOEXCEPT
{
	return static_cast<err::error_code>(lhs) == static_cast<err::error_code>(rhs);
}

void G2FError::clear()
{
	err::error_code::clear();
	_detail.clear();
}
