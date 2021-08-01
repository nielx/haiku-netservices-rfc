/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_SESSION_H_
#define _B_HTTP_SESSION_H_


#include <memory>


namespace BPrivate {

namespace Network {

class BHttpRequest;


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

	// Requests
	void					AddRequest(BHttpRequest request);

private:
	struct Wrapper;
	struct Data;
	std::shared_ptr<Data>	fData;
	static	status_t		ControlThreadFunc(void* arg);
	static	status_t		DataThreadFunc(void* arg);
};

}

}

#endif // _B_HTTP_SESSION
