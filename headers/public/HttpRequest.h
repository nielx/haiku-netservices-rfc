/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_HTTP_H_
#define _B_URL_PROTOCOL_HTTP_H_

#include <Expected.h>
#include <ErrorsExt.h>
#include <NetworkRequest.h>


namespace BPrivate {

namespace Network {


// Request method
class BHttpMethod {
public:
	// Some standard methods
	static	BHttpMethod			Get();
	static	BHttpMethod			Post();
	static	BHttpMethod			Put();
	static	BHttpMethod			Head();
	static	BHttpMethod			Delete();
	static	BHttpMethod			Options();
	static	BHttpMethod			Trace();
	static	BHttpMethod			Connect();

	// Custom methods
	static	Expected<BHttpMethod, BError>
								Make(std::string method);


	// Constructors and assignment
								BHttpMethod(const BHttpMethod& other) = default;
								BHttpMethod(BHttpMethod&& other) = default;
			BHttpMethod&		operator=(const BHttpMethod& other) = default;
			BHttpMethod&		operator=(BHttpMethod&& other) = default;
private:
								BHttpMethod(std::string method);
			std::string			fMethod;
};


class BHttpRequest : public BNetworkRequest {
public:
	virtual						~BHttpRequest();

protected:
			status_t			_ProtocolLoop();

private:
			friend 				class BUrlProtocolRoster;
								BHttpRequest(const BUrl& url,
									bool ssl = false);

			bool				fSSL;
			BHttpMethod			fRequestMethod;
};

} // namespace Network

} // namespace BPrivate

#endif
