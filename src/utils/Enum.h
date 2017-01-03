#pragma once

#include <climits>
#include <cstring>
#include <string>
#include <vector>
#include <utility>


struct EqualIgnoreCase
{
	bool operator()(const char* left,const char* right) const
	{
		size_t lenLeft=std::strlen(left);
		size_t lenRight=std::strlen(right);
		if(lenLeft==lenRight)
		{
			size_t i=0;
			while(i<lenLeft)
			{
				char l=std::tolower(left[i]);
				char r=std::tolower(right[i++]);
				if(l!=r)
					return false;
			}
			return true;
		}
		return false;
	}

	bool operator()(char left,char right) const
	{
		left=std::tolower(left);
		right=std::tolower(right);
		if(left==right)
			return true;
		return false;
	}
};

template<typename EnumType>
struct EnumMapItem
{
	const EnumType _enum;
	const char* _str;
};

template<typename EnumType>
class Enum
{
	EnumType _enum;
	Enum(bool b);
	Enum& operator=(bool b);
public:
	typedef EnumType enum_t;
	typedef EnumMapItem<EnumType> enum_map_item_t;
	typedef Enum<EnumType> self_t;

	static const EnumType npos;

	Enum() : _enum(npos) {}
	Enum(int val) : _enum(toEnum(val)) {}
	Enum(const EnumType& e) : _enum(toEnum(e)) {}
	Enum(const char* str) : _enum(toEnum(str)) {}
	Enum(const std::string& str) : _enum(toEnum(str)) {}

	bool isDefine(void) const
	{
		return this->_enum!=npos;
	}

	template<typename OtherType>
	Enum& numericCastFrom(const Enum<OtherType>& from)
	{
		EnumType castFrom(static_cast<EnumType>(from.toEnum()));
		if(indexEnum(castFrom)==-1)
			this->_enum=npos;
		else
			this->_enum=castFrom;
		return (*this);
	}

	template<typename OtherType>
	Enum& stringCastFrom(const Enum<OtherType>& from)
	{
		if(from!=from.npos)
			this->_enum=toEnum(from.toString());
		else
			this->_enum=npos;
		return (*this);
	}

	Enum& operator=(int val)
	{
		_enum=toEnum(val);
		return *this;
	}

	Enum& operator=(const EnumType& e)
	{
		_enum=toEnum(e);
		return *this;
	}

	Enum& operator=(const char* str)
	{
		this->_enum=toEnum(str);
		return *this;
	}
	
	Enum& operator=(const std::string& str)
	{
		this->_enum=toEnum(str);
		return *this;
	}

	operator const EnumType&() const
	{
		return _enum;
	}

	const EnumType& toEnum() const
	{
		return _enum;
	}

	std::string toStdString() const
	{
		return toStdString(_enum);
	}

	const char* toString() const
	{
		return toString(_enum);
	}

	Enum& operator++()
	{
		incr();
		return *this;
	}
	Enum operator++(int)
	{
		EnumType e=_enum;
		incr();
		return Enum(e);
	}
	Enum& operator--()
	{
		decr();
		return *this;
	}
	Enum operator--(int)
	{
		EnumType e=_enum;
		decr();
		return Enum(e);
	}

	void clear(void)
	{
		_enum=npos;
	}

//	Static
	static Enum begin()
	{
		return Enum(getMapItemIndex(0)._enum);
	}

	static Enum end()
	{
		return Enum(npos);
	}

	static size_t getMapSize()
	{
		return getMap().first;
	}

	static int indexEnum(const EnumType& e)
	{
		const map_result_t& res=getMap();
		for(size_t i=0;i<res.first;i++)
			if(res.second[i]._enum==e)
				return static_cast<int>(i);
		return -1;
	}

	static int indexString(const char* str)
	{
		if(!str)
			return -1;
		EqualIgnoreCase comp;
		const map_result_t& res=getMap();
		for(size_t i=0;i<res.first;i++)
		{
			if(comp(res.second[i]._str,str))
				return static_cast<int>(i);
		}
		return -1;
	}

	static EnumType toEnum(int val)
	{
		int ifind=indexEnum(static_cast<EnumType>(val));
		return ifind==-1?npos:getMapItemIndex(ifind)._enum;
	}

	static EnumType toEnum(const EnumType& en)
	{
		int ifind=indexEnum(en);
		return ifind==-1?npos:getMapItemIndex(ifind)._enum;
	}

	static EnumType toEnum(const char* str)
	{
		int ifind=indexString(str);
		return ifind==-1?npos:getMapItemIndex(ifind)._enum;
	}
	
	static EnumType toEnum(const std::string& str)
	{
		int ifind=indexString(str.c_str());
		return ifind==-1?npos:getMapItemIndex(ifind)._enum;
	}

	static const char* toString(const EnumType& e)
	{
		int ifind=indexEnum(e);
		return ifind==-1?(const char*)0:getMapItemIndex(ifind)._str;
	}
	
	static std::string toStdString(const EnumType& e)
	{
		int ifind=indexEnum(e);
		if(ifind==-1)
			return std::string();
		return getMapItemIndex(ifind)._str;
	}

	static std::string printableStrList(const char* delimiter)
	{
		std::string ret;
		const map_result_t& res=getMap();
		if(res.first)
		{
			ret+=res.second[0]._str;
			for(size_t i=1;i<res.first;i++)
			{
				ret+=delimiter;
				ret+=res.second[i]._str;
			}
		}
		return ret;
	}

	bool operator==(const EnumType& e) const { return (int)_enum==(int)e; }
	bool operator!=(const EnumType& e) const { return (int)_enum!=(int)e; }
	bool operator>=(const EnumType& e) const { return (int)_enum>=(int)e; }
	bool operator<=(const EnumType& e) const { return (int)_enum<=(int)e; }
	bool operator>(const EnumType& e) const { return (int)_enum>(int)e; }
	bool operator<(const EnumType& e) const { return (int)_enum<(int)e; }

	typedef std::pair<size_t,const enum_map_item_t*> map_result_t;
	static map_result_t getMap();

protected:
	static const enum_map_item_t& getMapItemIndex(int index)
	{
		return getMap().second[index];
	}

	void incr()
	{
		if(_enum!=npos)
		{
			int ienum=indexEnum(_enum);
			if(ienum>=0)
			{
				if(ienum!=getMapSize()-1)
					_enum=getMapItemIndex(ienum+1)._enum;
				else
					_enum=npos;
			}
			else
				_enum=npos;
		}
		else
		{
			if(getMapSize())
				_enum=getMapItemIndex(0)._enum;
		}
	}
	void decr()
	{
		if(_enum!=npos)
		{
			int ienum=indexEnum(_enum);
			if(ienum>=0)
			{
				if(ienum)
					_enum=getMapItemIndex(ienum-1)._enum;
				else
					_enum=npos;
			}
			else
				_enum=npos;
		}
		else
		{
			size_t ms=getMapSize();
			if(ms)
				_enum=getMapItemIndex(static_cast<int>(ms-1))._enum;
		}
	}
};

template<typename EnumType> const EnumType Enum<EnumType>::npos=(EnumType)(INT_MAX);

template<typename EnumType>
std::ostream& operator<<(std::ostream& stream,const Enum<EnumType>& s)
{
	stream << s.toStdString();
	return stream;
}

template<typename EnumType>
std::istream& operator>>(std::istream& stream,Enum<EnumType>& s)
{
	const typename Enum<EnumType>::map_result_t& res=Enum<EnumType>::getMap();
	size_t mapSize=res.first;
	if(!mapSize)
		return stream;

	s.clear();
	int c(0);
	size_t readed(0);
	std::vector<const char*> variants(mapSize);
	for(size_t i=0;i<mapSize;i++)
		variants[i]=res.second[i]._str;

	bool isNext=true;
	const char* pBingo=0;
	while(isNext && stream && !stream.eof() && !variants.empty())
	{
		bool isEof=false;
		bool isSpace=false;
		c=stream.rdbuf()->sbumpc();
		if(c==std::ios::traits_type::eof())
		{
			isEof=true;
			stream.setstate(std::ios::eofbit);
		}
		else
			isSpace=isspace(c)!=0;

		EqualIgnoreCase comp;
		std::vector<const char*> tmp;
		std::vector<const char*>::const_iterator it=variants.begin(),itEnd=variants.end();
		for(;it!=itEnd;it++)
		{
			char cs=(*it)[readed];
			if(!cs)
			{
				if(isEof || isSpace)
				{
					pBingo=*it;
					isNext=false;
					if(isSpace)
						stream.unget();
					break;
				}
				continue;
			}
			else
			{
				if(isEof)
				{
					isNext=false;
					break;
				}
				else
				if(!comp(cs,c))
					continue;
			}
			tmp.push_back(*it);
		}
		if(isNext)
			variants=tmp;
		readed++;
	}
	if(pBingo)
	{
		s=pBingo;
	}
	else
	{
		//	fits nothing
		stream.setstate(std::ios::failbit);
	}
	return stream;
}

//	map generator
#define BEGIN_ENUM_MAP(EnumType) \
template<> std::pair<size_t,const EnumMapItem<EnumType>* > Enum<EnumType>::getMap() \
{ \
	static const EnumMapItem<EnumType> map[]= \
{

#define ENUM_MAP_ITEM_STR(EnumType,str) \
	{EnumType,str},

#define ENUM_MAP_ITEM(sEnum) \
	ENUM_MAP_ITEM_STR(sEnum,#sEnum)

#define END_ENUM_MAP \
}; \
	return std::make_pair(sizeof(map)/sizeof(enum_map_item_t),map); \
}

#define BEGIN_ENUM_MAP_C(classname,EnumType) \
	BEGIN_ENUM_MAP(classname::EnumType)

#define ENUM_MAP_ITEM_C_STR(classname,EnumType,str) \
	ENUM_MAP_ITEM_STR(classname::EnumType,str)

#define ENUM_MAP_ITEM_C(classname,sEnum) \
	ENUM_MAP_ITEM_STR(classname::sEnum,#sEnum)

#define END_ENUM_MAP_C \
END_ENUM_MAP

