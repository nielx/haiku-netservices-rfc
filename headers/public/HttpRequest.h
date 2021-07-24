/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_HTTP_H_
#define _B_URL_PROTOCOL_HTTP_H_

#include <Expected.h>
#include <ErrorsExt.h>
#include <HttpHeaders.h>
#include <NetworkRequest.h>


namespace BPrivate {

namespace Network {

class BHttpForm;

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
			void				_ResetOptions();
			status_t			_ProtocolLoop();

private:
			friend 				class BUrlProtocolRoster;
								BHttpRequest(const BUrl& url,
									bool ssl = false);

			bool				fSSL;
			BHttpMethod			fRequestMethod;
			int8				fHttpVersion;

			BHttpHeaders		fHeaders;
	// Request status

			//BHttpResult			fResult;

			// Request state/events
			enum {
				kRequestInitialState,
				kRequestStatusReceived,
				kRequestHeadersReceived,
				kRequestContentReceived,
				kRequestTrailingHeadersReceived
			}					fRequestStatus;

	// Protocol options
			uint8				fOptMaxRedirs;
			BString				fOptReferer;
			BString				fOptUserAgent;
			BString				fOptUsername;
			BString				fOptPassword;
			uint32				fOptAuthMethods;
			BHttpHeaders*		fOptHeaders;
			BHttpForm*			fOptPostFields;
			BDataIO*			fOptInputData;
			ssize_t				fOptInputDataSize;
			off_t				fOptRangeStart;
			off_t				fOptRangeEnd;
			bool				fOptSetCookies : 1;
			bool				fOptFollowLocation : 1;
			bool				fOptDiscardData : 1;
			bool				fOptDisableListener : 1;
			bool				fOptAutoReferer : 1;
			bool				fOptStopOnError : 1;
};

// HTTP Version
enum {
	B_HTTP_10 = 1,
	B_HTTP_11
};

} // namespace Network

} // namespace BPrivate

#endif
