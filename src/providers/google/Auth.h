#pragma once

#include <boost/filesystem.hpp>
#include "utils/decls.h"
#include <googleapis/client/transport/http_transport.h>
#include <googleapis/client/auth/oauth2_authorization.h>

G2F_GOOGLE_NS_SHORTHANDS
namespace fs=boost::filesystem;

using OAuth2CredentialPtr=uptr<g_cli::OAuth2Credential>;

class Auth
{
public:
	Auth(g_cli::HttpTransport* transport, fs::path&& secretClientFile);

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

