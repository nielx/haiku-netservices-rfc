/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_SESSION_H_
#define _B_HTTP_SESSION_H_


#include <future>
#include <memory>


namespace BPrivate {

namespace Network {

class BHttpRequest;
struct BHttpResponse;

class BHttpSession {
public:
	// Constructor & Destructor
								BHttpSession();
								~BHttpSession() = default;

	// Session modifiers
	void						SetCookieJar() { }
	void						AddAuthentication() { }
	void						SetProxy() { }
	void						AddCertificateException() { }

	// Session Accessors
	void						GetCookieJar() { }
	void						GetAuthentication() { }
	bool						UseProxy() { return false; }
	void						GetProxyHost() { }
	void						GetProxyPort() { }
	bool						HasCertificateException() { return false; }

	// Requests
	std::future<BHttpResponse>	AddRequest(BHttpRequest request);

private:
	struct Wrapper;
	struct Data;
	std::shared_ptr<Data>		fData;
	static	status_t			ControlThreadFunc(void* arg);
	static	status_t			DataThreadFunc(void* arg);

	// Helper Functions
	static	bool				_ResolveHostName(Wrapper& request);
	static	bool				_OpenConnection(Wrapper& request);
	static	std::string			_CreateRequestHeaders(Wrapper& request);
	static	bool				_RequestRead(Wrapper& request);
	static	void				_ParseStatus(Wrapper& request);
	static	void				_ParseHeaders(Wrapper& request);
};

}

}

#endif // _B_HTTP_SESSION
