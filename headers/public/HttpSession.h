/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_SESSION_H_
#define _B_HTTP_SESSION_H_


#include <memory>


namespace BPrivate {

namespace Network {


class BHttpSession {
public:
	// Constructor & Destructor
							BHttpSession();
							~BHttpSession() = default;

	// Session modifiers
	void					SetCookieJar() { }
	void					AddAuthentication() { }
	void					SetProxy() { }
	void					AddCertificateException() { }

	// Session Accessors
	void					GetCookieJar() { }
	void					GetAuthentication() { }
	bool					UseProxy() { return false; }
	void					GetProxyHost() { }
	void					GetProxyPort() { }
	bool					HasCertificateException() { return false; }

private:
	struct Data;
	std::shared_ptr<Data>	fData;
};

}

}

#endif // _B_HTTP_SESSION
