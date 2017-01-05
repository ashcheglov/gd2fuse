#include "GoogleProvider.h"
#include "Auth.h"
#include "control/Application.h"
#include "providers/IProvider.h"
#include "providers/IDataReader.h"
#include "error/G2FException.h"

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

		// TODO Exponential backoff
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

	GoogleDataProvider(sptr<g_drv::DriveService> service, const OAuth2CredentialPtr &authCred)
		:_service(service),
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
		dest.setTime(IMetaWrapper::ModificationTime,ts);
		dest.setTime(IMetaWrapper::ChangeTime,ts);
		gdt2timespec(file->get_last_viewed_by_me_date(),ts);
		dest.setTime(IMetaWrapper::AccessTime,ts);

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

private:
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
//   Provider Session
//
/////////////////////////////////////////
class GoogleProviderSession : public IProviderSession
{
public:

	GoogleProviderSession(const std::string &accId,sptr<g_cli::HttpTransportLayerConfig> conf,const IProvider* parent)
		: _accId(accId),
		  _conf(conf),
		  _parent(parent)
	{
		_service=std::make_shared<g_drv::DriveService>(createTransport().release());
	}

	// IProviderSession interface
public:
	virtual IDataProviderPtr createDataProvider() override
	{
		return std::make_shared<GoogleDataProvider>(_service,getAuthCred());
	}

	virtual IOAuth2ProcessPtr createOAuth2Process() override
	{
		return std::make_shared<GoogleOAuth2>(_service->transport(),_accId,Application::instance()->pathManager().secretFile(),Application::instance()->pathManager().credentialHomeDir());
	}

	virtual const IProvider *getProvider() override
	{
		return _parent;
	}

	virtual bool isAuthorized() override
	{
		return !(!_authCred);
	}

	OAuth2CredentialPtr getAuthCred()
	{
		if(!_authCred)
		{
			Auth a(createTransport().release(),Application::instance()->pathManager().secretFile());
			a.setErrorCallback();
			_authCred=a.auth(_accId,Application::instance()->pathManager().credentialHomeDir());
		}
		return _authCred;
	}

	uptr<g_cli::HttpTransport> createTransport()
	{
		g_utl::Status status;
		uptr<g_cli::HttpTransport> ret(_conf->NewDefaultTransport(&status));
		if (!status.ok())
			throw G2F_EXCEPTION("Error creating HTTP transport").arg(status.error_code()).arg(status.ToString());
		return ret;
	}

private:
	std::string _accId;
	sptr<g_cli::HttpTransportLayerConfig> _conf;
	sptr<g_drv::DriveService> _service;
	OAuth2CredentialPtr _authCred;
	const IProvider *_parent=nullptr;
};




//
//   Provider
//
/////////////////////////////////////////
class GoogleProvider : public IProvider
{

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
};

ProviderRegistrar<GoogleProvider> _reg(ProvidersRegistry::getInstance());

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
