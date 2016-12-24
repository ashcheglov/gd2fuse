#include "handler.h"
#include "FuseDrive.h"
#include "utils/log.h"
#include "utils/ExponentialBackoff.h"
#include <googleapis/client/data/data_reader.h>
#include <googleapis/client/transport/http_response.h>
#include "error/G2FException.h"
#include "error/appError.h"
#include "utils/assets.h"

// TODO Watch service to detect modification
namespace
{
	const std::string ID_ROOT("root");
	const std::string FILE_RESOURCE_FIELD("etag,title,mimeType,createdDate,modifiedDate,lastViewedByMeDate,originalFilename,fileSize,md5Checksum");
	MimePair FOLDER_MIME("application","vnd.google-apps.folder");
	MimePair GOOGLE_DOC("application","vnd.google-apps.document");
	MimePair SHORTCUT("application","vnd.google-apps.drive-sdk");
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

FileType getFileType(const MimePair &mp)
{
	if(mp==FOLDER_MIME)
		return FileType::Directory;
	if(mp==GOOGLE_DOC)
		return FileType::GoogleDoc;
	if(mp==SHORTCUT)
		return FileType::Shortcut;
	return FileType::Binary;
}

class GoogleSource : public Tree::IMetaSource, public ContentManager::IContentSource
{
public:
	class DataReader : public ContentManager::IContentSource::IDataReader
	{
	public:
		DataReader(uptr<g_drv::FilesResource_GetMethod> method)
			: _method(std::move(method))
		{}
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

	GoogleSource(g_drv::DriveService *service, g_cli::AuthorizationCredential *authCred)
		:_service(service),
		  _authCred(authCred)
	{}
	// IMetaSource interface
public:
#define G2F_STRPAIR(str) str,str+sizeof(str)/sizeof(char)-1

	virtual std::set<std::string> fetchList(const std::string &id) override
	{
		std::set<std::string> ret;
		uptr<g_drv::ChildrenResource_ListMethod> lm(_service->get_children().NewListMethod(_authCred,id));
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
				ret.insert(e.get("id","").asString());
			}
		}
		return ret;
	}
	virtual void fetchFile(const std::string &id, IFileWrapper &dest) override
	{
		uptr<g_drv::FilesResource_GetMethod> lm(_service->get_files().NewGetMethod(_authCred,id));
		lm->set_fields(FILE_RESOURCE_FIELD);
		uptr<g_drv::File> file(g_drv::File::New());
		const g_utl::Status& s=lm->ExecuteAndParseResponse(file.get());
		const G2FError &e=checkHttpResponse(lm->http_request());
		if(e)
			G2FExceptionBuilder().throwIt(e);

		FileType ft=getFileType(splitMime(file->get_mime_type().ToString()));
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
	virtual ContentManager::IContentSource::IDataReader *contentReader(const std::string &id) override
	{
		// TODO Make multithread download
		uptr<g_drv::FilesResource_GetMethod> lm(_service->get_files().NewGetMethod(_authCred,id));
		lm->set_alt("media");
		return new DataReader(std::move(lm));
	}

private:
	g_drv::DriveService *_service=nullptr;
	g_cli::AuthorizationCredential *_authCred=nullptr;
};

FuseDrive::FuseDrive(g_drv::DriveService &service, googleapis::client::AuthorizationCredential *authCred, const boost::filesystem::path &workDir)
	: _service(service),
	  _workDir(workDir),
	  _authCred(authCred)
{
}

int FuseDrive::run(const FUSEOpts &fuseOpts)
{
	fuse_args args=FUSE_ARGS_INIT(0,NULL);
	fuse_opt_add_arg(&args,G2F_APP_NAME);
	if(fuseOpts.debug)
		fuse_opt_add_arg(&args,"-d");
	if(fuseOpts.foreground)
		fuse_opt_add_arg(&args,"-f");
	if(fuseOpts.disableMThread)
		fuse_opt_add_arg(&args,"-s");
	for(const std::string& o : fuseOpts.ooo)
	{
		fuse_opt_add_arg(&args,"-o");
		fuse_opt_add_arg(&args,o.c_str());
	}
	if(!fuseOpts.mountPoint.empty())
		fuse_opt_add_arg(&args,fuseOpts.mountPoint.string().c_str());

	fuse_operations g2f_oper;
	g2f_init_ops(&g2f_oper);
	_googleSource.reset(new GoogleSource(&_service,_authCred));
	_tree.setMetaCallback(_googleSource.get());
	_cManager.init(_workDir,_googleSource.get());
	return fuse_main(args.argc, args.argv, &g2f_oper, this);
}

Node *FuseDrive::getMeta(const char *path)
{
	return _tree.get(path);
}

Tree &FuseDrive::metaTree()
{
	return _tree;
}

Tree::IDirectoryIteratorUPtr FuseDrive::getDirectoryIterator(Node *meta)
{
	return Tree::IDirectoryIteratorUPtr(_tree.getDirectoryIterator(meta));
}

int FuseDrive::fuseHelp()
{
	fuse_args args=FUSE_ARGS_INIT(0,NULL);
	fuse_opt_add_arg(&args,G2F_APP_NAME);
	fuse_opt_add_arg(&args,"-ho");	// Omit fuse help header
	fuse_operations ops;
	memset(&ops,0,sizeof(fuse_operations));
	return fuse_main(args.argc, args.argv, &ops, 0);
}

int FuseDrive::openContent(Node *meta, int flags, uint64_t &fd)
{
	try
	{
		fd=_cManager.openFile(_tree.id(meta),flags);
	}
	catch(G2FException &e)
	{
		return e.code().default_error_condition().value();
	}
	catch(err::system_error &e)
	{
		return e.code().default_error_condition().value();
	}
	return 0;
}

int FuseDrive::readContent(uint64_t fd, char *buf, size_t len, off_t offset)
{
	try
	{
		return _cManager.readContent(fd,buf,len,offset);
	}
	catch(G2FException &e)
	{
		return e.code().default_error_condition().value();
	}
	catch(err::system_error &e)
	{
		return e.code().default_error_condition().value();
	}
	return 0;
}

int FuseDrive::closeContent(uint64_t fd)
{
	try
	{
		return _cManager.closeFile(fd);
	}
	catch(G2FException &e)
	{
		return e.code().default_error_condition().value();
	}
	catch(err::system_error &e)
	{
		return e.code().default_error_condition().value();
	}
	return 0;
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
