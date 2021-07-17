/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_HTTP_H_
#define _B_URL_PROTOCOL_HTTP_H_

#include <NetworkRequest.h>


namespace BPrivate {

namespace Network {


class BHttpRequest : public BNetworkRequest {
public:
	virtual						~BHttpRequest();

private:
			friend 				class BUrlProtocolRoster;
								BHttpRequest(const BUrl& url,
									bool ssl = false);

			bool				fSSL;
			std::string			fRequestMethod;
};


// Request method
const char* const B_HTTP_GET = "GET";
const char* const B_HTTP_POST = "POST";
const char* const B_HTTP_PUT = "PUT";
const char* const B_HTTP_HEAD = "HEAD";
const char* const B_HTTP_DELETE = "DELETE";
const char* const B_HTTP_OPTIONS = "OPTIONS";
const char* const B_HTTP_TRACE = "TRACE";
const char* const B_HTTP_CONNECT = "CONNECT";

}

}

#endif
