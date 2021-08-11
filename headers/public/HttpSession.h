/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_SESSION_H_
#define _B_HTTP_SESSION_H_


#include <future>
#include <memory>

#include <Messenger.h>


namespace BPrivate {

namespace Network {

class BHttpRequest;
class BHttpResult;

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
	BHttpResult					AddRequest(BHttpRequest request,
									BMessenger observer = BMessenger());

private:
	struct Wrapper;
	struct Data;
	std::shared_ptr<Data>		fData;
	static	status_t			ControlThreadFunc(void* arg);
	static	status_t			DataThreadFunc(void* arg);

	// Helper Functions
	static	void				_ResolveHostName(Wrapper& request);
	static	void				_OpenConnection(Wrapper& request);
	static	std::string			_CreateRequestHeaders(Wrapper& request);
	static	bool				_RequestRead(Wrapper& request);
	static	void				_ParseStatus(Wrapper& request);
	static	void				_ParseHeaders(Wrapper& request);
};


namespace UrlEvent {
	enum {
		HttpStatus = '_HST',
		HttpHeaders = '_HHD',
		CertificateError = '_CER'
	};
}


namespace UrlEventData {
	extern const char* HttpStatus;
	extern const char* SSLCertificate;
	extern const char* SSLMessage;
}


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_SESSION
