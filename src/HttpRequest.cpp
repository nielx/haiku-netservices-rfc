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


using BPrivate::BError;
using namespace BPrivate::Network;


BHttpRequest::BHttpRequest(const BUrl& url, bool ssl)
	: BNetworkRequest(url),
	fSSL(ssl),
	fRequestMethod(BHttpMethod::Get()),
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


// BHttpMethod
BHttpMethod::BHttpMethod(std::string method)
	: fMethod(method)
{

}


BHttpMethod
BHttpMethod::Get()
{
	return BHttpMethod("GET");
}


BHttpMethod
BHttpMethod::Post()
{
	return BHttpMethod("POST");
}


BHttpMethod
BHttpMethod::Put()
{
	return BHttpMethod("PUT");
}


BHttpMethod
BHttpMethod::Head()
{
	return BHttpMethod("HEAD");
}


BHttpMethod
BHttpMethod::Delete()
{
	return BHttpMethod("DELETE");
}


BHttpMethod
BHttpMethod::Options()
{
	return BHttpMethod("OPTIONS");
}


BHttpMethod
BHttpMethod::Trace()
{
	return BHttpMethod("TRACE");
}


BHttpMethod
BHttpMethod::Connect()
{
	return BHttpMethod("CONNECT");
}


Expected<BHttpMethod, BError>
BHttpMethod::Make(std::string method)
{
	 // Todo: check http spec
	 return BHttpMethod(method);
}
