#include "GoogleProvider.h"
#include "Auth.h"
#include "control/Application.h"
#include "providers/IProvider.h"
#include "providers/IDataReader.h"
#include "error/G2FException.h"
#include "control/props/AbstractFileStaticInitPropertiesList.h"
#include "control/ChainedCofiguration.h"
#include "control/paths/PathManager.h"

#include "providers/google/Auth.h"
#include <googleapis/client/util/status.h>
#include <googleapis/client/transport/curl_http_transport.h>
#include <googleapis/client/transport/http_request_batch.h>
//#include <googleapis/strings/strcat.h>
#include <googleapis/client/data/data_reader.h>
#include <googleapis/client/transport/http_response.h>
#include <googleapis/client/util/date_time.h>
#include <googleapis/strings/stringpiece.h>
#include <googleapis/client/transport/http_transport.h>
#include <google/drive_api/drive_service.h>
#include <googleapis/client/transport/http_authorization.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

namespace g_api=googleapis;
namespace g_cli=googleapis::client;
namespace g_utl=googleapis::util;
namespace g_drv=google_drive_api;
namespace fs=boost::filesystem;

namespace google
{


// TODO Watch service to detect modification

const std::string ID_ROOT("root");
const std::string FILE_RESOURCE_FIELD("etag,title,mimeType,createdDate,modifiedDate,lastViewedByMeDate,originalFilename,fileSize,md5Checksum");
MimePair FOLDER_MIME("application","vnd.google-apps.folder");
MimePair GOOGLE_DOC("application","vnd.google-apps.document");
MimePair SHORTCUT("application","vnd.google-apps.drive-sdk");

void gdt2timespec(const g_cli::DateTime &from, timespec &to)
{
	struct timeval tv;
	from.GetTimeval(&tv);
	to.tv_sec=tv.tv_sec;
	to.tv_nsec=tv.tv_usec*1000;
}

void sp2md5sign(const g_api::StringPiece &str, MD5Signature &ret)
{
	if(str.size()!=32)
		throw G2F_EXCEPTION("Can't recognize MD5 hash. Wrong length =%1, must be %2").arg(str.size()).arg(32);
	boost::algorithm::unhex(str.begin(),str.end(),ret.begin());
}

IMetaWrapper::FileType getFileType(const MimePair &mp)
{
	if(mp==FOLDER_MIME)
		return IMetaWrapper::FileType::Directory;
	if(mp==GOOGLE_DOC)
		return IMetaWrapper::FileType::Exported;
	if(mp==SHORTCUT)
		return IMetaWrapper::FileType::Shortcut;
	return IMetaWrapper::FileType::Binary;
}

G2FError checkHttpResponse(const g_cli::HttpRequest* request)
{
	G2FError ret;

	g_cli::HttpResponse* response=request->response();
	const g_utl::Status& transportStatus = response->transport_status();
	if(!transportStatus.ok())
	{
		ret=G2FErrorCodes::HttpTransportError;
		ret.setDetail(transportStatus.error_message());
		return ret;
	}
	int httpCode=response->http_code();

	if(!response->ok())
	{
		if(!response->body_reader()->Reset())
		{
			ret=G2FErrorCodes::InternalError;
			ret.setDetail(G2FMESSAGE("HTTP stream operation failed"));
			return ret;
		}
		//response->headers().find("Content-type");
		/*string body;
		googleapis::util::Status status = response->GetBodyString(&body);
		if(!status.ok())
		{
			ret=G2FErrorCodes::InternalError;
			ret.setDetail(status.error_message());
			return ret;
		}*/
		if(httpCode>=400 && httpCode<500)
			ret=G2FErrorCodes::HttpClientError;
		else
		if(httpCode>=401 && httpCode<=403)
			ret=G2FErrorCodes::HttpPermissionError;
		else
		if(httpCode==408)
			ret=G2FErrorCodes::HttpTimeoutError;
		if(httpCode>=500 && httpCode<600)
			ret=G2FErrorCodes::HttpServerError;
		ret.setDetail(request->url());

		// TODO Using exponential backoff
		// Detect necessity of using of exponential backoff (see https://developers.google.com/drive/v3/web/handle-errors#exponential-backoff)
		// try parse content as JSON or decode content-type
		/*g_cli::JsonCppCapsule<g_cli::JsonCppData> jsonData;
		googleapis::util::Status status=jsonData.LoadFromJsonReader(response->body_reader());
		if(!status.ok())
		{
			ret=G2FErrorCodes::InternalError;
			return ret;
		}*/
		// Restore offset in case someone downstream wants to read the body again.
		if (!response->body_reader()->Reset())
			ret=G2FErrorCodes::InternalError;
	}
	return ret;
}





class AbstractGoogleConfiguration : public IConfiguration
{
	// IConfiguration interface
public:
	virtual boost::optional<std::string> getProperty(const std::string &name) override
	{

		const IPropertyDefinitionPtr &p=getPL()->findProperty(name);
		if(p)
			return p->getValue();
		return boost::none;
	}
	virtual G2FError setProperty(const std::string &name, const std::string &value) override
	{
		IPropertyDefinitionPtr p=getPL()->findProperty(name);
		if(p)
			return p->setValue(value);
		return G2FError(G2FErrorCodes::PropertyNotFound).setDetail(name);
	}
	virtual IPropertiesListPtr getProperiesList() override
	{
		return getPL();
	}

protected:
	virtual const IPropertiesListPtr& getPL() =0;

};





//
//   Data Reader
//
/////////////////////////////////////////
class DataReader : public IDataReader
{
public:
	DataReader(uptr<g_drv::FilesResource_GetMethod> method)
		: _method(std::move(method))
	{}

// IDataReader interface
	virtual bool done() override
	{
		return reader()->done();
	}
	virtual int64_t read(char *buffer, int64_t bufSize) override
	{
		return reader()->ReadToBuffer(bufSize,buffer);
	}
	virtual G2FError error() override
	{
		if(reader()->ok())
			return G2FError();

		G2FError ret;
		ret=G2FErrorCodes::HttpReadError;
		ret.setDetail(reader()->status().error_message());
		return ret;

	}

private:
	uptr<g_drv::FilesResource_GetMethod> _method;
	g_cli::DataReader *_reader=0;

	g_cli::DataReader *reader()
	{
		if(!_reader)
		{
			_method->Execute();
			const G2FError &e=checkHttpResponse(_method->mutable_http_request());
			if(e)
				G2FExceptionBuilder().throwIt(e);
			g_cli::HttpResponse *resp=_method->mutable_http_request()->response();
			_reader=resp->body_reader();
		}
		return _reader;
	}
};





//
//   Data Provider
//
/////////////////////////////////////////
class GoogleDataProvider : public IDataProvider
{
public:

	GoogleDataProvider(IProviderSession *parent,sptr<g_drv::DriveService> service, const OAuth2CredentialPtr &authCred)
		: _parent(parent),
		  _service(service),
		  _authCred(authCred)
	{}

#define G2F_STRPAIR(str) str,str+sizeof(str)/sizeof(char)-1

	// IDataProvider interface
	virtual IDataReaderPtr createContentReader(const std::string &id, IConvertFormatPtr targetFormat) override
	{
		// TODO Make multithread download
		uptr<g_drv::FilesResource_GetMethod> lm(_service->get_files().NewGetMethod(_authCred.get(),id));
		lm->set_alt("media");
		return std::make_shared<DataReader>(std::move(lm));
	}

	class IdList : public IIdList
	{
	public:
		typedef std::vector<std::string> List;
		List vals;
		List::const_iterator it;

		void charge()
		{
			it=vals.begin();
		}

		// IIdList interface
		virtual bool hasNext() override
		{
			return it<vals.end();
		}
		virtual std::string next() override
		{
			return *(it++);
		}
	};
	G2F_DECLARE_PTR(IdList);

	virtual IIdListPtr fetchList(const std::string &id) override
	{
		IdListPtr ret=std::make_shared<IdList>();
		uptr<g_drv::ChildrenResource_ListMethod> lm(_service->get_children().NewListMethod(_authCred.get(),id));
		lm->set_fields("etag,items(id)");
		uptr<g_drv::ChildList> data(g_drv::ChildList::New());
		const g_utl::Status& s=lm->ExecuteAndParseResponse(data.get());
		const G2FError &e=checkHttpResponse(lm->http_request());
		if(e)
			G2FExceptionBuilder().throwIt(e);

		// There is really problem to work with original Google API: many things doesn't work as must.
		// Therefore I'll use low level JsonCpp API
		const char n[]="items";
		const Json::Value *items=data->Storage().find(G2F_STRPAIR(n));
		if(items && items->isArray())
		{
			// TODO Implement with Batch mode (see google-api-cpp-client/src/googleapis/client/transport/http_request_batch.h)
			for(int i=0;i<items->size();++i)
			{
				const Json::Value& e=(*items)[i];
				ret->vals.push_back(e.get("id","").asString());
			}
		}
		ret->charge();
		return ret;
	}

	virtual void fetchMeta(const std::string &id, IMetaWrapper &dest) override
	{
		uptr<g_drv::FilesResource_GetMethod> lm(_service->get_files().NewGetMethod(_authCred.get(),id));
		lm->set_fields(FILE_RESOURCE_FIELD);
		uptr<g_drv::File> file(g_drv::File::New());
		const g_utl::Status& s=lm->ExecuteAndParseResponse(file.get());
		const G2FError &e=checkHttpResponse(lm->http_request());
		if(e)
			G2FExceptionBuilder().throwIt(e);

		IMetaWrapper::FileType ft=getFileType(splitMime(file->get_mime_type().ToString()));
		dest.setId(id);
		dest.setFileName(file->get_title().ToString());
		dest.setFileType(ft);

		timespec ts;
		//gdt2timespec(file->get_created_date(),ts);
		//leaf->setTime(Node::CreatedTime,ts);
		gdt2timespec(file->get_modified_date(),ts);
		dest.setTime(ModificationTime,ts);
		dest.setTime(ChangeTime,ts);
		gdt2timespec(file->get_last_viewed_by_me_date(),ts);
		dest.setTime(AccessTime,ts);

		size_t size=file->get_file_size();
		dest.setSize(size);
		//if(ft==FileType::Binary)
		if(size)
		{
			const g_api::StringPiece& m=file->get_md5_checksum();
			if(!m.empty())
			{
				MD5Signature md5;
				sp2md5sign(file->get_md5_checksum(),md5);
				dest.setMD5(md5);
			}
		}
	}

	virtual IProviderSession *getParent() override
	{
		return _parent;
	}

private:
	IProviderSession *_parent=nullptr;
	sptr<g_drv::DriveService> _service;
	OAuth2CredentialPtr _authCred;
};



//
//   OAuth2 process
//
/////////////////////////////////////////
class GoogleOAuth2 : public IOAuth2Process
{
	// IOAuth2Process interface
public:
	GoogleOAuth2(g_cli::HttpTransport *trt,const std::string &accId,const fs::path &secretFile,const fs::path &credentialHomeDir)
		: _trt(trt),
		  _accId(accId),
		  _secretFile(secretFile),
		  _credentialHomeDir(credentialHomeDir)
	{}
	virtual bool isFinished() override
	{
		return !(!_oa2c);
	}
	virtual std::string getError() override
	{
		return _errorMessage;
	}
	virtual bool startNativeApp() override
	{
		try
		{
			Auth a(_trt,_secretFile);
			a.setShellCallback(std::string());
			_oa2c=a.auth(_accId,_credentialHomeDir);
		}
		catch(const std::exception &e)
		{
			_errorMessage=e.what();
		}
	}
	virtual int finishNativeApp(const std::string &authCode) override
	{
		try
		{
			Auth a(_trt,_secretFile);
			a.setShellCallback(authCode);
			_oa2c=a.auth(_accId,_credentialHomeDir);
		}
		catch(const std::exception &e)
		{
			_errorMessage=e.what();
		}
	}

	OAuth2CredentialPtr getAuthToken()
	{
		return _oa2c;
	}

private:
	g_cli::HttpTransport *_trt=nullptr;
	std::string _accId;
	fs::path _secretFile;
	fs::path _credentialHomeDir;
	OAuth2CredentialPtr _oa2c;
	std::string _errorMessage;
};






//
//   Provider Session Configuration
//
/////////////////////////////////////////
//
//   Provider Configuration
//
/////////////////////////////////////////
class GoogleProviderSessionProperties : public AbstractFileStaticInitPropertiesList
{

public:
	GoogleProviderSessionProperties(const fs::path &fileName,const std::string &accName)
		: _fileName(fileName),
		  _accName(accName)
	{}

	// IPropertiesList interface
public:
	virtual std::string getName() override
	{
		return _accName;
	}

	// AbstractStaticInitPropertiesList interface
protected:
	virtual std::pair<const PropDefi *, size_t> getPropertiesDefinition() override
	{
		return std::make_pair(nullptr,0);
	}

	virtual std::pair<const EnumDefi *, size_t> getEnumsDefinition() override
	{
		return std::make_pair(nullptr,0);
	}

	// AbstractFileStaticInitPropertiesList interface
public:
	virtual boost::filesystem::path getPropertiesFileName() override
	{
		return _fileName;
	}
private:
	fs::path _fileName;
	std::string _accName;
};
G2F_DECLARE_PTR(GoogleProviderSessionProperties);







class GoogleSessionConfiguration : public AbstractGoogleConfiguration
{
public:
	GoogleSessionConfiguration(const IPathManagerPtr &parentPM,const std::string &accName)
	{
		_pm=createNextLevelWrapperPathManager(parentPM,accName);
		_props=std::make_shared<GoogleProviderSessionProperties>(_pm->getDir(IPathManager::CONFIG),accName);
	}

	// IConfiguration interface
public:
	virtual IPathManagerPtr getPaths() override
	{
		return _pm;
	}
protected:
	virtual const IPropertiesListPtr& getPL() override
	{
		return _props;
	}

private:
	IPropertiesListPtr _props;
	IPathManagerPtr _pm;
};





//
//   Provider Session
//
/////////////////////////////////////////
class GoogleProviderSession : public IProviderSession
{
public:

	GoogleProviderSession(const std::string &accId,sptr<g_cli::HttpTransportLayerConfig> conf,IProvider* parent)
		: _accId(accId),
		  _httpConf(conf),
		  _parent(parent)
	{
		_service=std::make_shared<g_drv::DriveService>(createTransport().release());

		IConfigurationPtr current=std::make_shared<GoogleSessionConfiguration>(parent->getConfiguration()->getPaths(),_accId);
		_conf=std::make_shared<ChainedConfiguration>(parent->getConfiguration(),current);
	}

	// IProviderSession interface
public:
	virtual IDataProviderPtr createDataProvider() override
	{
		return std::make_shared<GoogleDataProvider>(this,_service,getAuthCred());
	}

	virtual IOAuth2ProcessPtr createOAuth2Process() override
	{
		return std::make_shared<GoogleOAuth2>(_service->transport(),_accId,getSecretFile(),getCredentialHomeDir());
	}

	virtual IProvider *getProvider() override
	{
		return _parent;
	}

	virtual bool isAuthorized() override
	{
		return !(!_authCred);
	}

	virtual IConfigurationPtr getConfiguration() override
	{
		return _conf;
	}

	OAuth2CredentialPtr getAuthCred()
	{
		if(!_authCred)
		{
			Auth a(createTransport().release(),getSecretFile());
			a.setErrorCallback();
			_authCred=a.auth(_accId,getCredentialHomeDir());
		}
		return _authCred;
	}

	fs::path getSecretFile()
	{
		const fs::path &confDir=_parent->getConfiguration()->getPaths()->getDir(IPathManager::CONFIG);
		return confDir/G2F_APP_NAME "_secret.json";
	}

	fs::path getCredentialHomeDir()
	{
		fs::path confDir=getConfiguration()->getPaths()->getDir(IPathManager::CONFIG);
		return confDir/"auth.info";
	}

	uptr<g_cli::HttpTransport> createTransport()
	{
		g_utl::Status status;
		uptr<g_cli::HttpTransport> ret(_httpConf->NewDefaultTransport(&status));
		if (!status.ok())
			throw G2F_EXCEPTION("Error creating HTTP transport").arg(status.error_code()).arg(status.ToString());
		return ret;
	}

private:
	std::string _accId;
	sptr<g_cli::HttpTransportLayerConfig> _httpConf;
	sptr<g_drv::DriveService> _service;
	OAuth2CredentialPtr _authCred;
	IProvider *_parent=nullptr;
	IConfigurationPtr _conf;

};





//
//   Provider Configuration
//
/////////////////////////////////////////
namespace
{

const AbstractStaticInitPropertiesList::PropDefi propsDefi[]=
{
//	name							type					enum		def					change	descr
	"permission_new_gdoc",			IPropertyType::OINT,	0,			"444",				true,	"Permissions to new GDoc files (non editable).",
	"export_gdoc_documents",		IPropertyType::ENUM,	"gdde",		"plain_text",		true,	"Format to export GDoc documents.",
	"export_gdoc_spreadsheets",		IPropertyType::ENUM,	"gdds",		"csv",				true,	"Format to export GDoc Spreadsheets.",
	"export_gdoc_drawings",			IPropertyType::ENUM,	"gddd",		"jpeg",				true,	"Format to export GDoc Drawings.",
	"export_gdoc_presentations",	IPropertyType::ENUM,	"gddp",		"plain_text",		true,	"Format to export GDoc Presentations.",
	"disable-ssl-verify",			IPropertyType::BOOL,	0,			"false",			true,	"Google servers certificate's checking will be disabled.",
	"ssl-ca-path",					IPropertyType::PATH,	0,			"",					false,	"path to the SSL certificate authority validation data."
};

const AbstractStaticInitPropertiesList::EnumDefi enumDefi[]=
{
	"gdde", {
		{"html",			"HTML"},
		{"plain_text",		"Plain text"},
		{"rich_text",		"RTF"},
		{"openoffice_doc",	"OpenOffice odt"},
		{"pdf",				"PDF"},
		{"ms_word",			"MS Word"}
	},
	"gdds", {
		{"csv",				"Comma separated list"},
		{"openoffice_sheet","OpenOffice ods"},
		{"pdf",				"PDF"},
		{"ms_excel",		"MS Excel"}
	},
	"gddd", {
		{"jpeg",			"JPEG"},
		{"png",				"PNG"},
		{"svg",				"SVG"},
		{"pdf",				"PDF"}
	},
	"gddp", {
		{"plain_text",		"Plain text"},
		{"pdf",				"PDF"},
		{"ms_powerpoint",	"MS PowerPoint"}
	}

};


}


class GoogleProviderProperties : public AbstractFileStaticInitPropertiesList
{

public:
	GoogleProviderProperties(const fs::path &fileName)
		: _fileName(fileName)
	{}

	// IPropertiesList interface
public:
	virtual std::string getName() override
	{
		return "gdrive";
	}

	// AbstractStaticInitPropertiesList interface
protected:
	virtual std::pair<const PropDefi *, size_t> getPropertiesDefinition() override
	{
		return std::make_pair(propsDefi,sizeof(propsDefi)/sizeof(AbstractStaticInitPropertiesList::PropDefi));
	}

	virtual std::pair<const EnumDefi *, size_t> getEnumsDefinition() override
	{
		return std::make_pair(enumDefi,sizeof(enumDefi)/sizeof(AbstractStaticInitPropertiesList::EnumDefi));
	}

	// AbstractFileStaticInitPropertiesList interface
public:
	virtual boost::filesystem::path getPropertiesFileName() override
	{
		return _fileName;
	}
private:
	fs::path _fileName;
};
G2F_DECLARE_PTR(GoogleProviderProperties);







class GoogleConfiguration : public AbstractGoogleConfiguration
{
public:
	GoogleConfiguration(const IConfigurationPtr &parent)
	{
		_pm=createNextLevelWrapperPathManager(parent->getPaths(),"gdrive");
		_props=std::make_shared<GoogleProviderProperties>(_pm->getDir(IPathManager::CONFIG));
	}

	// IConfiguration interface
public:
	virtual IPathManagerPtr getPaths() override
	{
		return _pm;
	}
protected:
	virtual const IPropertiesListPtr& getPL() override
	{
		return _props;
	}

private:
	IPropertiesListPtr _props;
	IPathManagerPtr _pm;
};





//
//   Provider
//
/////////////////////////////////////////
class GoogleProvider : public IProvider
{
public:
	GoogleProvider(const IConfigurationPtr &globalConf)
	{
		IConfigurationPtr current=std::make_shared<GoogleConfiguration>(globalConf);
		_conf=std::make_shared<ChainedConfiguration>(globalConf,current);
	}

public:
	virtual std::string getName()
	{
		return "gdrive";
	}

	virtual int getProperties()
	{
		return IProvider::NEED_AUTHORIZATION;
	}

	virtual IProviderSessionPtr createSession(const std::string &accountId,const PropList &props) override
	{
		return std::make_shared<GoogleProviderSession>(accountId,transportConfig(props),this);
	}

	virtual ISupportedConversionPtr getSupportedConversion() override
	{
		// TODO Implement
		G2F_EXCEPTION("GoogleProvider::getSupportedConversion()").throwIt(G2FErrorCodes::NotImplemented);
		return ISupportedConversionPtr();
	}

	virtual IConfigurationPtr getConfiguration() override
	{
		return _conf;
	}

	// Allowable args:
	// disable-ssl-verify - Google servers certificate's checking will be disabled
	// ssl-ca-path=<path> - path to the SSL certificate authority validation data.
	sptr<g_cli::HttpTransportLayerConfig> transportConfig(const PropList &props)
	{
		sptr<g_cli::HttpTransportLayerConfig> ret(new g_cli::HttpTransportLayerConfig);
		ret->ResetDefaultTransportFactory(new g_cli::CurlHttpTransportFactory(ret.get()));
		g_cli::HttpTransportOptions* opts=ret->mutable_default_transport_options();
		for(std::string p : props)
		{
			boost::to_lower(p);
			if(p=="disable-ssl-verify")
				opts->set_cacerts_path(g_cli::HttpTransportOptions::kDisableSslVerification);
			else
			{
				std::string::size_type off=p.find("ssl-ca-path=");
				if(off!=std::string::npos)
					opts->set_cacerts_path(p.substr(off));
			}
		}
		return ret;
	}

private:
	IConfigurationPtr _conf;
};







class GoogleProviderfactory : public IProviderFactory
{


	// IProviderFactory interface
public:
	virtual std::string getName() override
	{
		return "gdrive";
	}
	virtual IProviderPtr create(const IConfigurationPtr &globalConf) override
	{
		return std::make_shared<GoogleProvider>(globalConf);
	}
};

ProviderRegistrar<GoogleProviderfactory> _reg(ProvidersRegistry::getInstance());

}


/*
struct ContentFetchCallback
{
	ContentFetchCallback(const fs::path &name_)
		: name(name_)
	{}
	void ready(g_cli::HttpRequest *request)
	{
		// check response
		const ResponseCheckResult &res=checkResponse(request);
		if(!res.ok())
		{
			error=res.errorMessage;
			return;
		}
		// save to file
		boost::system::error_code ec;
		fs::create_directories(name.parent_path(),ec);
		if(!ec)
		{
			error=ec.message();
			return;
		}
		fs::ofstream out(name);
		if(!out)
		{
			//error=boost::format("Error file opening: %1%") % name;
			error="File open error: "+name.string();
			return;
		}
		g_cli::HttpResponse *resp=request->response();
		g_cli::DataReader *reader=resp->body_reader();
		std::array<char,1024> buf;
		while(!reader->done())
		{
			int64 rdd=reader->ReadToBuffer(buf.size(),buf.data());
			if(rdd)
				out.write(buf.data(),rdd);
		}
		if(reader->error())
			error=reader->status().error_message();
	}

	std::string error;
	fs::path name;
};*/