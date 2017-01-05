#include <iostream>
#include "Auth.h"
#include "error/G2FException.h"
#include <googleapis/client/auth/file_credential_store.h>
#include <googleapis/client/data/data_reader.h>
#if HAVE_OPENSSL
#include <googleapis/client/data/openssl_codec.h>
#endif
#include <googleapis/client/util/status.h>
#include <googleapis/base/callback.h>
#include <google/drive_api/drive_service.h>
#include <boost/filesystem/fstream.hpp>

namespace g_drv=google_drive_api;
namespace g_api=googleapis;

namespace google
{

/*
 *  Callback
*/
struct Callback
{
	typedef g_utl::Status (Callback::*Type)(const g_cli::OAuth2RequestOptions& options,string* authorization_code);

	g_utl::Status exceptionCallback(const g_cli::OAuth2RequestOptions& options,
										   string* authorization_code)
	{
		throw G2F_EXCEPTION("Failed to authenticate. Please, refresh auth information");
	}

	g_utl::Status promptShellCallback(const g_cli::OAuth2RequestOptions& options,\
									  string* authorization_code)
	{
		// TODO Consider using of xdg-open, firefox or google-chrome
		if(!authCode.empty())
		{
			*authorization_code=authCode;
			return g_cli::StatusOk();
		}

		const std::string& url(flow->GenerateAuthorizationCodeRequestUrlWithOptions(options));

		std::cout << "Enter the following url into a browser:\n" << url << std::endl;
		std::cout << "Enter the browser's response: ";

		authorization_code->clear();
		std::cin >> *authorization_code;
		if (authorization_code->empty())
			return g_cli::StatusCanceled("Canceled");
		return g_cli::StatusOk();
	}

	g_cli::OAuth2AuthorizationFlow* flow;
	std::string authCode;
};


class Auth::Impl
{
public:

	Impl(g_cli::HttpTransport* transport,const fs::path &secretClientFile)
	{
		fs::ifstream sf(secretClientFile);
		if(!sf)
			throw G2F_EXCEPTION("Can't open secret client file '%1'").arg(secretClientFile.string());

		g_utl::Status status;
		std::string secret;
		secret.assign(std::istream_iterator<std::string::value_type>(sf),
						   std::istream_iterator<std::string::value_type>());

		_flow.reset(g_cli::OAuth2AuthorizationFlow::MakeFlowFromClientSecretsJson(
						secret, transport, &status));
		if (!status.ok())
			throw G2F_EXCEPTION_GSTATUS("Fail to load client secret file '%1'",status).arg(secretClientFile);

		_flow->set_default_scopes(g_drv::DriveService::SCOPES::DRIVE);
		//_flow->mutable_client_spec()->set_redirect_uri(g_cli::OAuth2AuthorizationFlow::kOutOfBandUrl);
		setCallback(&Callback::exceptionCallback,std::string());
	}

	void setCallback(Callback::Type method,const std::string& authCode)
	{
		//typedef ResultCallback2< util::Status, const OAuth2RequestOptions&, string*> AuthorizationCodeCallback;
		Callback *c=new Callback;
		c->flow=_flow.get();
		c->authCode=authCode;
		_flow->set_authorization_code_callback(g_api::NewPermanentCallback(c, method));
	}

	OAuth2CredentialPtr auth(const std::string &email,const fs::path& credentialHomePath)
	{
		g_utl::Status status;
		g_cli::FileCredentialStoreFactory store_factory(credentialHomePath.string());
#if HAVE_OPENSSL
		g_cli::OpenSslCodecFactory* openssl_factory = new g_cli::OpenSslCodecFactory;
		status = openssl_factory->SetPassphrase(_flow->client_spec().client_secret());
		if (!status.ok())
			throw G2F_EXCEPTION_GSTATUS("Fail to init OpenSSL codec",status);
		store_factory.set_codec_factory(openssl_factory);
#endif
		_flow->ResetCredentialStore(store_factory.NewCredentialStore(G2F_APP_NAME, &status));
		if (!status.ok())
			throw G2F_EXCEPTION_GSTATUS("Fail to create credential store '%1'",status).arg(G2F_APP_NAME);

		OAuth2CredentialPtr ret(new g_cli::OAuth2Credential);
		g_cli::OAuth2RequestOptions options;
		options.email=email;
		status=_flow->RefreshCredentialWithOptions(options,ret.get());
		if(!status.ok())
			throw G2F_EXCEPTION_GSTATUS("Fail to obtain authorization token",status);

		return ret;
	}

	uptr<g_cli::OAuth2AuthorizationFlow> _flow;
};








Auth::Auth(g_cli::HttpTransport *transport, const boost::filesystem::path &secretClientFile)
{
	_impl.reset(new Impl(transport,std::move(secretClientFile)));
}

void Auth::setShellCallback(const std::string &authCode)
{
	_impl->setCallback(&Callback::promptShellCallback,authCode);
}

void Auth::setErrorCallback()
{
	_impl->setCallback(&Callback::exceptionCallback,std::string());
}

OAuth2CredentialPtr Auth::auth(const std::string &email,const fs::path& credentialHomePath)
{
	return _impl->auth(email,credentialHomePath);
}

}

