#pragma once

#include <boost/filesystem.hpp>
#include "utils/decls.h"
#include <googleapis/client/transport/http_transport.h>
#include <googleapis/client/auth/oauth2_authorization.h>

namespace g_cli=googleapis::client;
namespace g_utl=googleapis::util;
namespace fs=boost::filesystem;

namespace google
{
	using OAuth2CredentialPtr=sptr<g_cli::OAuth2Credential>;

	class Auth
	{
	public:
		/*
		 * posession of 'transport' is passed to instance
		 * */
		Auth(g_cli::HttpTransport* transport,const fs::path& secretClientFile);

		std::string authURI() const;
		std::string clientId() const;
		std::string clientSecret() const;
		std::string redirectURI() const;

		void setShellCallback(const std::string &authCode);
		void setErrorCallback();
		OAuth2CredentialPtr auth(const std::string &email,const fs::path& credentialHomePath);

	private:
		class Impl;
		sptr<Impl> _impl;
	};

}
