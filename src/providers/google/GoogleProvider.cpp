#include "GoogleProvider.h"
#include "Auth.h"
#include "control/Application.h"
#include "providers/IProvider.h"
#include "providers/IDataReader.h"
#include "error/G2FException.h"
#include "control/props/AbstractFileStaticInitPropertiesList.h"
#include "control/ChainedCofiguration.h"
#include "control/paths/PathManager.h"
#include "fs/AbstractFileSystem.h"

#include "providers/google/Auth.h"
#include <googleapis/client/util/status.h>
#include <googleapis/client/transport/curl_http_transport.h>
#include <googleapis/client/transport/http_request_batch.h>
//#include <googleapis/strings/strcat.h>
#include <googleapis/client/data/data_reader.h>
#include <googleapis/client/transport/http_response.h>
#include <googleapis/client/util/date_time.h>
#include <googleapis/client/util/status.h>
#include <googleapis/strings/stringpiece.h>
#include <googleapis/client/transport/http_transport.h>
#include <google/drive_api/drive_service.h>
#include <googleapis/client/transport/http_authorization.h>
#include "googleapis/base/integral_types.h"

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

#define G2F_GDRIVE_CONF_FILENAME	"gdrive.conf"

const fs::path ROOT_PATH("/");
const std::string ID_ROOT("root");
const std::string FILE_RESOURCE_FIELD("id,etag,title,mimeType,createdDate,modifiedDate,lastViewedByMeDate,originalFilename,fileSize,md5Checksum");
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

g_cli::DateTime timespec2gdt(const timespec &from)
{
	return g_cli::DateTime(from.tv_sec);
}

void sp2md5sign(const g_api::StringPiece &str, MD5Signature &ret)
{
	if(str.size()!=32)
		G2F_EXCEPTION("Can't recognize MD5 hash. Wrong length =%1, must be %2").arg(str.size()).arg(32).throwIt(G2FErrorCodes::MD5Error);
	boost::algorithm::unhex(str.begin(),str.end(),ret.begin());
}

std::string getMimeType(const MimePair &mp)
{
	std::string ret(mp.first);
	ret+='/';
	ret+=mp.second;
	return ret;
}


INode::NodeType getFileType(const MimePair &mp)
{
	if(mp==FOLDER_MIME)
		return INode::NodeType::Directory;
	if(mp==GOOGLE_DOC)
		return INode::NodeType::Exported;
	if(mp==SHORTCUT)
		return INode::NodeType::Shortcut;
	return INode::NodeType::Binary;
}

std::string getFileMime(const INode::NodeType &type)
{
	switch(type)
	{
	case INode::NodeType::Directory:
		return getMimeType(FOLDER_MIME);
	case INode::NodeType::Exported:
		return getMimeType(GOOGLE_DOC);
	case INode::NodeType::Shortcut:
		return getMimeType(SHORTCUT);
	}
	return std::string();
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




/**
 * @brief The AbstractGoogleConfiguration class
 *
 ********************************************************/
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





/**
 * @brief Data Reader
 *
 *****************************************************/
class ContentReader : public ContentManager::IReader
{
public:
	ContentReader(uptr<g_drv::FilesResource_GetMethod> method)
		: _method(std::move(method))
	{}

// IReader interface
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
				G2FExceptionBuilder("Fail to read data from Google Drive").throwIt(e);
			g_cli::HttpResponse *resp=_method->mutable_http_request()->response();
			_reader=resp->body_reader();
		}
		return _reader;
	}
};










/**
 * @brief Implements Google File System
 *
 **************************************************/
class GoogleFileSystem : public AbstractFileSystem
{
public:
	GoogleFileSystem(const ContentManagerPtr &cm,
					 sptr<g_drv::DriveService> service,
					 OAuth2CredentialPtr authCred)
		: AbstractFileSystem(cm),
		  _service(service),
		  _authCred(authCred)
	{}

	class GoogleReader : public g_cli::DataReader
	{
	public:
		GoogleReader(ContentManager::IReader *content)
			: DataReader(nullptr),
			  _content(content)
		{}

		// DataReader interface
	protected:
		// FIX Conflict between int64 definition (int64 = long vs long long)
		virtual long long DoReadToBuffer(long long max_bytes, char* storage) override
		{
			long long ret=_content->read(storage,max_bytes);

			if(ret!=max_bytes)
			{
				if(_content->done())
					set_done(true);

				G2FError e=_content->error();
				if(e!=err::errc::success)
				{
					if(e.category()==err::generic_category())
						set_status(g_cli::StatusFromErrno(e.value(),e.message()));
					else
						set_status(g_cli::StatusUnknown(e.message()));
				}
			}
			return ret;
		}

		ContentManager::IReader *_content;
	};

	// AbstractFileSystem interface
protected:
	virtual void cloudFetchMeta(Node &dest) override
	{
		uptr<g_drv::FilesResource_GetMethod> lm(_service->get_files().NewGetMethod(_authCred.get(),dest.getId()));
		lm->set_fields(FILE_RESOURCE_FIELD);
		lm->mutable_http_request()->mutable_options()->set_timeout_ms(_timeout);
		uptr<g_drv::File> file(g_drv::File::New());
		const g_utl::Status& s=lm->ExecuteAndParseResponse(file.get());
		const G2FError &e=checkHttpResponse(lm->http_request());
		if(e.isError())
			G2FExceptionBuilder("GoogleFS: fail to get node description").throwIt(e);

		fillNode(*file,dest);
	}
#define G2F_STRPAIR(str) str,str+sizeof(str)/sizeof(char)-1

	void fillNode(g_drv::File &source,Node &dest)
	{
		dest.setId(source.get_id().ToString());
		dest.setName(source.get_title().ToString());
		INode::NodeType ft=getFileType(splitMime(source.get_mime_type().ToString()));
		dest.setFileType(ft);

		timespec ts;
		//gdt2timespec(source.get_created_date(),ts);
		//leaf->setTime(Node::CreatedTime,ts);
		gdt2timespec(source.get_modified_date(),ts);
		dest.setTime(ModificationTime,ts);
		dest.setTime(ChangeTime,ts);
		gdt2timespec(source.get_last_viewed_by_me_date(),ts);
		dest.setTime(AccessTime,ts);

		size_t size=source.get_file_size();
		dest.setSize(size);
		if(size)
		{
			const g_api::StringPiece& m=source.get_md5_checksum();
			if(!m.empty())
			{
				MD5Signature md5;
				sp2md5sign(source.get_md5_checksum(),md5);
				dest.setMD5(md5);
			}
		}
	}

	virtual std::vector<std::string> cloudFetchChildrenList(const std::string &parentId) override
	{
		std::vector<std::string> ret;

		uptr<g_drv::ChildrenResource_ListMethod> lm(_service->get_children().NewListMethod(_authCred.get(),parentId));
		lm->mutable_http_request()->mutable_options()->set_timeout_ms(_timeout);
		lm->set_fields("etag,items(id)");
		uptr<g_drv::ChildList> data(g_drv::ChildList::New());
		const g_utl::Status& s=lm->ExecuteAndParseResponse(data.get());
		const G2FError &e=checkHttpResponse(lm->http_request());
		if(e)
			G2FExceptionBuilder("GoogleFS: Error reading children list").throwIt(e);

		// There is really problem to work with original Google API: many things doesn't work as must.
		// Therefore I'll use low level JsonCpp API
		const char n[]="items";
		const Json::Value *items=data->Storage().find(G2F_STRPAIR(n));
		if(items && items->isArray())
		{
			// TODO Implement with Batch mode (see google-api-cpp-client/src/googleapis/client/transport/http_request_batch.h)
			for(int i=0;i<boost::numeric_cast<int>(items->size());++i)
			{
				const Json::Value& e=(*items)[i];
				ret.push_back(e.get("id","").asString());
			}
		}
		return ret;
	}

	virtual void cloudCreateMeta(Node &dest) override
	{
		Json::Value jStorage;
		g_drv::File f(&jStorage);
		f.set_title(dest.getName().string());

		Json::Value jRefStorage;
		g_drv::ParentReference pref(&jRefStorage);
		pref.set_id(dest.getParent()->getId());

		Json::Value jParentsStorage;
		g_cli::JsonCppArray<g_drv::ParentReference> parents(&jParentsStorage);
		parents.set(0,pref);
		(*f.MutableStorage())["parents"]=*parents.MutableStorage();

		if(dest.isFolder())
			f.set_mime_type(getMimeType(FOLDER_MIME));

		uptr<g_drv::FilesResource_InsertMethod> lm(_service->get_files().NewInsertMethod(_authCred.get(),
																						 &f,
																						 "",
																						 nullptr));
		lm->mutable_http_request()->mutable_options()->set_timeout_ms(_timeout);
		lm->set_fields(FILE_RESOURCE_FIELD);
		uptr<g_drv::File> file(g_drv::File::New());

		const g_utl::Status& s=lm->ExecuteAndParseResponse(file.get());
		const G2FError &e=checkHttpResponse(lm->mutable_http_request());
		if(e)
			G2FExceptionBuilder("GoogleFS: fail to create node").throwIt(e);

		fillNode(*file,dest);
	}

	virtual ContentManager::IReaderUPtr cloudReadMedia(Node &node) override
	{
		uptr<g_drv::FilesResource_GetMethod> lm(_service->get_files().NewGetMethod(_authCred.get(),node.getId()));
		lm->set_alt("media");
		lm->mutable_http_request()->mutable_options()->set_timeout_ms(_timeout);
		return std::make_unique<ContentReader>(std::move(lm));
	}

	virtual void cloudUpdate(Node &node,int patchFields,const std::string &mediaType,ContentManager::IReader *content) override
	{
		assert(patchFields!=0 || content);

		Json::Value jStorage;
		Json::Value jRefStorage;
		Json::Value jParentsStorage;
		g_drv::File f(&jStorage);
		const g_drv::File* metadata=nullptr;

		if(patchFields & Node::Field::Parent)
		{
			g_drv::ParentReference pref(&jRefStorage);
			pref.set_id(node.getParent()->getId());

			g_cli::JsonCppArray<g_drv::ParentReference> parents(&jParentsStorage);
			parents.set(0,pref);
			(*f.MutableStorage())["parents"]=*parents.MutableStorage();
		}

		if(patchFields & Node::Field::Name)
		{
			f.set_title(node.getName().string());
		}

		if(patchFields & Node::Field::Time)
		{
			f.set_modified_date(timespec2gdt(node.getTime(TimeAttrib::ModificationTime)));
			f.set_created_date(timespec2gdt(node.getTime(TimeAttrib::ChangeTime)));
			f.set_last_viewed_by_me_date(timespec2gdt(node.getTime(TimeAttrib::AccessTime)));
		}

		if(patchFields & Node::Field::Content)
		{
			f.set_mime_type(getFileMime(node.getNodeType()));
		}


		if(patchFields)
			metadata=&f;

		uptr<GoogleReader> dataReader;
		if(content)
			dataReader=std::make_unique<GoogleReader>(content);

		// Takes ownership about dataReader
		uptr<g_drv::FilesResource_UpdateMethod> lm(_service->get_files().NewUpdateMethod(_authCred.get(),
																						 node.getId(),
																						 metadata,
																						 mediaType,
																						 dataReader.release()));
		lm->mutable_http_request()->mutable_options()->set_timeout_ms(_timeout);
		lm->Execute();
		const G2FError &e=checkHttpResponse(lm->mutable_http_request());
		if(e.isError())
			G2FExceptionBuilder("GoogleFS: Fail to update data").throwIt(e);
	}

	virtual void cloudRemove(Node &node) override
	{
		// TODO Implement remove to trash
		uptr<g_drv::FilesResource_DeleteMethod> m(_service->get_files().NewDeleteMethod(
													  _authCred.get(),
													  node.getId()));
		m->mutable_http_request()->mutable_options()->set_timeout_ms(_timeout);
		m->Execute();
		const G2FError &e=checkHttpResponse(m->mutable_http_request());
		if(e.isError())
			G2FExceptionBuilder("GoogleFS: Fail to remove node id '%1'").arg(node.getId()).throwIt(e);
	}

private:
	sptr<g_drv::DriveService> _service;
	OAuth2CredentialPtr _authCred;
	int64_t _timeout=60000;
};
G2F_DECLARE_PTR(GoogleFileSystem);



























/**
 * @brief produce HttpTransport
 *
 *******************************/
class HttpTransportFactory
{
public:
	HttpTransportFactory(const sptr<g_cli::HttpTransportLayerConfig> &conf)
		:_httpConf(conf)
	{}

	g_cli::HttpTransport *createTransport() const
	{
		g_utl::Status status;
		uptr<g_cli::HttpTransport> ret(_httpConf->NewDefaultTransport(&status));
		if (!status.ok())
			G2F_EXCEPTION("Error creating HTTP transport").arg(status.error_code()).arg(status.ToString()).throwIt(G2FErrorCodes::HttpTransportError);
		return ret.release();
	}

private:
	sptr<g_cli::HttpTransportLayerConfig> _httpConf;
};





/**
 * @brief OAuth2 process
 *
 ******************************************/
class GoogleOAuth2 : public IOAuth2Process
{
	// IOAuth2Process interface
public:
	GoogleOAuth2(const HttpTransportFactory& trf,const std::string &accId,const fs::path &secretFile,const fs::path &credentialHomeDir)
		: _trf(trf),
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
			Auth a(_trf.createTransport(),_secretFile);
			a.setShellCallback(std::string());
			_oa2c=a.auth(_accId,_credentialHomeDir);
		}
		catch(const std::exception &e)
		{
			_errorMessage=e.what();
			return false;
		}
		return true;
	}
	virtual bool finishNativeApp(const std::string &authCode) override
	{
		try
		{
			Auth a(_trf.createTransport(),_secretFile);
			a.setShellCallback(authCode);
			_oa2c=a.auth(_accId,_credentialHomeDir);
		}
		catch(const std::exception &e)
		{
			_errorMessage=e.what();
			return false;
		}
		return true;
	}

	OAuth2CredentialPtr getAuthToken()
	{
		return _oa2c;
	}

private:
	const HttpTransportFactory& _trf;
	std::string _accId;
	fs::path _secretFile;
	fs::path _credentialHomeDir;
	OAuth2CredentialPtr _oa2c;
	std::string _errorMessage;
};






/**
 * @brief Provider Session Configuration
 *
 *********************************************************/
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





/**
 * @brief The GoogleProviderSession class
 *
 *****************************************************/
class GoogleProviderSession : public IProviderSession
{
public:

	GoogleProviderSession(const std::string &accId,sptr<g_cli::HttpTransportLayerConfig> conf,IProvider* parent)
		: _accId(accId),
		  _transportFactory(conf),
		  _parent(parent)
	{
		_service=std::make_shared<g_drv::DriveService>(_transportFactory.createTransport());

		IConfigurationPtr current=std::make_shared<GoogleSessionConfiguration>(parent->getConfiguration()->getPaths(),_accId);
		_conf=std::make_shared<ChainedConfiguration>(parent->getConfiguration(),current);
	}

	// IProviderSession interface
public:
	virtual IFileSystemPtr getFileSystem() override
	{
		if(!_fs)
		{
			_fs=std::make_shared<GoogleFileSystem>(
						std::make_shared<ContentManager>(_conf->getPaths()->getDir(IPathManager::DATA)),
						_service,
						getAuthCred());
		}
		return _fs;
	}

	virtual IOAuth2ProcessPtr createOAuth2Process() override
	{
		return std::make_shared<GoogleOAuth2>(_transportFactory,_accId,getSecretFile(),getCredentialHomeDir());
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
			Auth a(_transportFactory.createTransport(),getSecretFile());
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

private:
	std::string _accId;
	HttpTransportFactory _transportFactory;
	sptr<g_drv::DriveService> _service;
	OAuth2CredentialPtr _authCred;
	IProvider *_parent=nullptr;
	IConfigurationPtr _conf;
	GoogleFileSystemPtr _fs;

};





/**
 *			Provider Configuration
 *
 *********************************************/
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
	"use-ssl-verify",				IPropertyType::BOOL,	0,			"true",				false,	"Google servers certificate's checking will be disabled.",
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
		_props=std::make_shared<GoogleProviderProperties>(_pm->getDir(IPathManager::CONFIG)/G2F_GDRIVE_CONF_FILENAME);
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





/**
 * @brief The GoogleProvider class
 *
 ****************************************/
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
		// TODO
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
		// TODO Configurable
		opts->set_connect_timeout_ms(50000);
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
