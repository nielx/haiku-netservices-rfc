/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Stephan Aßmus, superstippi@gmx.de
 */


#include <HttpAuthentication.h>
#include <HttpForm.h>
#include <HttpHeaders.h>
#include <HttpRequest.h>
#include <ProxySecureSocket.h>
#include <Socket.h>
#include <SecureSocket.h>


using BPrivate::BError;
using namespace BPrivate::Network;


namespace BPrivate {

	class CheckedSecureSocket: public BSecureSocket
	{
		public:
			CheckedSecureSocket(BHttpRequest* request);

			bool			CertificateVerificationFailed(BCertificate& certificate,
					const char* message);

		private:
			BHttpRequest*	fRequest;
	};


	CheckedSecureSocket::CheckedSecureSocket(BHttpRequest* request)
		:
		BSecureSocket(),
		fRequest(request)
	{
	}


	bool
	CheckedSecureSocket::CertificateVerificationFailed(BCertificate& certificate,
		const char* message)
	{
		return fRequest->_CertificateVerificationFailed(certificate, message);
	}


	class CheckedProxySecureSocket: public BProxySecureSocket
	{
		public:
			CheckedProxySecureSocket(const BNetworkAddress& proxy, BHttpRequest* request);

			bool			CertificateVerificationFailed(BCertificate& certificate,
					const char* message);

		private:
			BHttpRequest*	fRequest;
	};


	CheckedProxySecureSocket::CheckedProxySecureSocket(const BNetworkAddress& proxy,
		BHttpRequest* request)
		:
		BProxySecureSocket(proxy),
		fRequest(request)
	{
	}


	bool
	CheckedProxySecureSocket::CertificateVerificationFailed(BCertificate& certificate,
		const char* message)
	{
		return fRequest->_CertificateVerificationFailed(certificate, message);
	}
};


BHttpRequest::BHttpRequest(const BUrl& url, bool ssl, const BHttpMethod method)
	: fUrl(url),
	fSSL(ssl),
	fRequestMethod(method),
	fHttpVersion(B_HTTP_11),
	fRequestStatus(kRequestInitialState),
	fOptHeaders(NULL),
	fOptPostFields(NULL),
	fOptInputData(NULL),
	fOptInputDataSize(-1),
	fOptRangeStart(-1),
	fOptRangeEnd(-1),
	fOptFollowLocation(true)
{
	_ResetOptions();
}


BHttpRequest::~BHttpRequest()
{

}


/*static*/ Expected<BHttpRequest, BError>
BHttpRequest::Get(const BUrl& url)
{
	if (!url.IsValid())
		return Unexpected<BError>(BError(B_BAD_VALUE, "Invalid URL"));

	if (url.Protocol() == "http")
		return BHttpRequest(url, false, BHttpMethod::Get());
	else if (url.Protocol() == "https")
		return BHttpRequest(url, true, BHttpMethod::Get());
	return Unexpected<BError>(BError(B_BAD_VALUE, "Unsupported protocol"));
}


/*static*/ bool
BHttpRequest::IsInformationalStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__INFORMATIONAL_BASE)
		&& (code <  B_HTTP_STATUS__INFORMATIONAL_END);
}


/*static*/ bool
BHttpRequest::IsSuccessStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__SUCCESS_BASE)
		&& (code <  B_HTTP_STATUS__SUCCESS_END);
}


/*static*/ bool
BHttpRequest::IsRedirectionStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__REDIRECTION_BASE)
		&& (code <  B_HTTP_STATUS__REDIRECTION_END);
}


/*static*/ bool
BHttpRequest::IsClientErrorStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__CLIENT_ERROR_BASE)
		&& (code <  B_HTTP_STATUS__CLIENT_ERROR_END);
}


/*static*/ bool
BHttpRequest::IsServerErrorStatusCode(int16 code)
{
	return (code >= B_HTTP_STATUS__SERVER_ERROR_BASE)
		&& (code <  B_HTTP_STATUS__SERVER_ERROR_END);
}


/*static*/ int16
BHttpRequest::StatusCodeClass(int16 code)
{
	if (BHttpRequest::IsInformationalStatusCode(code))
		return B_HTTP_STATUS_CLASS_INFORMATIONAL;
	else if (BHttpRequest::IsSuccessStatusCode(code))
		return B_HTTP_STATUS_CLASS_SUCCESS;
	else if (BHttpRequest::IsRedirectionStatusCode(code))
		return B_HTTP_STATUS_CLASS_REDIRECTION;
	else if (BHttpRequest::IsClientErrorStatusCode(code))
		return B_HTTP_STATUS_CLASS_CLIENT_ERROR;
	else if (BHttpRequest::IsServerErrorStatusCode(code))
		return B_HTTP_STATUS_CLASS_SERVER_ERROR;

	return B_HTTP_STATUS_CLASS_INVALID;
}


void
BHttpRequest::_ResetOptions()
{
	delete fOptPostFields;
	delete fOptHeaders;

	fOptFollowLocation = true;
	fOptMaxRedirs = 8;
	fOptReferer = "";
	fOptUserAgent = "Services Kit (Haiku)";
	fOptUsername = "";
	fOptPassword = "";
	fOptAuthMethods = B_HTTP_AUTHENTICATION_BASIC | B_HTTP_AUTHENTICATION_DIGEST
		| B_HTTP_AUTHENTICATION_IE_DIGEST;
	fOptHeaders = NULL;
	fOptPostFields = NULL;
	fOptSetCookies = true;
	fOptDiscardData = false;
	fOptDisableListener = false;
	fOptAutoReferer = true;
}


status_t
BHttpRequest::_ProtocolLoop()
{
	return B_OK;
}


status_t
BHttpRequest::_MakeRequest()
{
	return B_OK;
}


bool
BHttpRequest::_CertificateVerificationFailed(BCertificate& certificate,
	const char* message)
{
	// TODO
/*	if (fContext->HasCertificateException(certificate))
		return true;

	if (fListener != NULL
		&& fListener->CertificateVerificationFailed(this, certificate, message)) {
		// User asked us to continue anyway, let's add a temporary exception for this certificate
		fContext->AddCertificateException(certificate);
		return true;
	}*/

	return false;
}
