#include "G2FException.h"
#include <utility>
#include <set>
#include <boost/algorithm/string.hpp>

namespace algo=boost::algorithm;

namespace
{
	enum TokenType : int
	{
		Open,
		Close,
		NextBlock,
		Symbol,
		End
	};

	typedef std::pair<TokenType,char> TokenValue;

	TokenValue _readNext(std::string::const_iterator& it,	// <- is being changed
										const std::string::const_iterator& itEnd)
	{
		if(it==itEnd)
			return TokenValue(TokenType::End,0);

		TokenValue ret(TokenType::Symbol,0);
		bool lookaheadStep=false;
		TokenType firstStep=TokenType::Symbol;
		while(it!=itEnd)
		{
			char c=*it;
			switch(c)
			{
			case '{':
				firstStep=TokenType::Open;
				break;
			case '}':
				firstStep=TokenType::Close;
				break;
			case '|':
				firstStep=TokenType::NextBlock;
				break;
			}
			if(lookaheadStep)
			{
				if(firstStep==ret.first)
					ret.first=TokenType::Symbol;
				break;
			}
			++it;
			if(firstStep==Symbol)
			{
				ret.second=c;
				break;
			}
			lookaheadStep=true;
			ret.first=firstStep;
			ret.second=c;
		}
		return ret;
	}

	typedef std::set<size_t> UsedArgsList;
	typedef std::string::const_iterator cs_iter;
	typedef std::string::iterator s_iter;
	typedef boost::iterator_range<cs_iter> cs_iter_range;
	typedef boost::iterator_range<s_iter> s_iter_range;
	typedef algo::find_iterator<cs_iter> string_find_iterator;

	struct PlaceholderFinder
	{
		PlaceholderFinder()
			: value(0)
		{}

		// may be use std::regex?
		template<typename It>
		boost::iterator_range<It> operator()(
			It it,
			It itEnd )
		{
			while(it!=itEnd)
			{
				if(*it=='%')
				{
					It itNext=std::next(it);
					It itLast=std::find_if_not(itNext,itEnd,
									 [](char sym){return std::isdigit(sym);});
					if(itLast!=itNext)
					{
						boost::iterator_range<It> r(itNext, itLast);
						if(boost::conversion::try_lexical_convert(r,value) && value)
							return boost::make_iterator_range(it, itLast);
						it=itLast;
					}
				}
				++it;
			}
			return boost::iterator_range<It>();
		}

		size_t value;
	};

	// Search all %N (N>=1) entries, check condition accordingly 'args'
	bool _isAcceptBlock(const std::string& block,const std::vector<std::string> &args,UsedArgsList& usedArgs)
	{
		PlaceholderFinder pf;
		for(string_find_iterator It=
			algo::make_find_iterator(block, boost::ref(pf));
			It!=string_find_iterator();
			++It)
		{
			if(pf.value>=args.size())
				return false;
			usedArgs.insert(pf.value);
		}
		return true;
	}
}

// %N - argument placeholder
// {} - conditional block, separated by '|'
// Duplicating control symbols ({}|) acts like a shield ("{{" -> '{')
// Using standard shield symbol '\' inside string literals make code unreadable
std::string _compileMessage(const std::string &message,const std::vector<std::string> &args)
{
	std::string ret;
	if(message.empty())
	{
		// массив с указанием переданных аргументов
		UsedArgsList usedArgs;

#define G2F_OUTOFCONDITION	0
#define G2F_INSIDECONDITION	1
#define G2F_SKIPTOENDCOND	2
		int state(G2F_OUTOFCONDITION);
		std::string block;	// {block|block|...|block}
		std::string::const_iterator it=message.begin(),itEnd=message.end();
		for(TokenValue v=_readNext(it,itEnd);v.first!=TokenType::End;v=_readNext(it,itEnd))
		{
			switch(state)
			{
			case G2F_OUTOFCONDITION:	// Out-of-condition state
				{
					if(v.first!=TokenType::Open)
						ret+=v.second;
					else
						state=G2F_INSIDECONDITION;
				}
				break;
			case G2F_INSIDECONDITION:	// Inside-condition state
				{
					if(v.first==TokenType::NextBlock || v.first==TokenType::Close)
					{
						if(_isAcceptBlock(block,args,usedArgs))
						{
							ret+=block;
							state=G2F_SKIPTOENDCOND;
						}
						block.clear();
						if(v.first==TokenType::Close)
							state=G2F_OUTOFCONDITION;
					}
					else
						block+=v.second;
				}
			case G2F_SKIPTOENDCOND:	// Skip-to-end-condition state
				{
					if(v.first==TokenType::Close)
						state=G2F_OUTOFCONDITION;
				}
				break;
			}
		}

		PlaceholderFinder pf;
		algo::find_format_all(ret,
						  std::ref(pf),
						  [&](cs_iter_range match){assert(pf.value>0);return args[pf.value-1];});
		//algo::replace_all(ret,"%file",_srcFile);
		//algo::replace_all(ret,"%line",boost::lexical_cast<std::string>(_srcLine));
	}
	return ret;
}



G2FExceptionBuilder &G2FExceptionBuilder::message(const std::string &message)
{
	_message=message;
	return *this;
}

G2FExceptionBuilder &G2FExceptionBuilder::reason(const std::string &reason)
{
	_reason=reason;
	return *this;
}

void G2FExceptionBuilder::throwIt(G2FError code)
{
	G2FException e(code);
	const std::string &m=_compileMessage(_message,_args);
	if(!m.empty())
		e._error.setDetail(m);
	if(!_reason.empty())
		e << g2f_error_detail_string(_reason);
	BOOST_THROW_EXCEPTION(e);
}

G2FException::G2FException(const G2FError &errorCode)
{
	_error=errorCode;
}

const G2FError &G2FException::code()
{
	return _error;
}

const char *G2FException::what() const noexcept
{
	return _what.c_str();
}

void G2FException::_buildWhat()
{
	_what=_error.message();
	if(!_error.detail().empty())
		_what+=": "+_error.detail();
}
