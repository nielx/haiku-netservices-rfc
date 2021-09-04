/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NET_SERVICES_H_
#define _NET_SERVICES_H_

#include <String.h>


namespace BPrivate {

namespace Network {


namespace UrlEvent {
	enum {
		HostnameResolved = '_NHR',
		ConnectionOpened = '_NCO',
		UploadProgress = '_NUP',
		ResponseStarted = '_NRS',
		DownloadProgress = '_NDP',
		BytesWritten = '_NBW',
		RequestCompleted = '_NRC',
		DebugMessage = '_NDB'
	};
}


namespace UrlEventData {
	extern const char* Id;
	extern const char* HostName;
	extern const char* NumBytes;
	extern const char* TotalBytes;
	extern const char* Success;
	extern const char* DebugType;
	extern const char* DebugMessage;
}


// Standard Exceptions
struct unsupported_protocol_exception {
	BUrl 	url;
};


struct invalid_url_exception {
	BUrl	url;
};


struct url_request_exception {
	enum { HostnameError, NetworkError, ProtocolError, SystemError, Canceled } error_type;
	status_t system_error;
	BString error_message;
};


// Private helper to generate a unique identifier for a request
int32 get_netservices_request_identifier();


} // namespace Network

} // namespace BPrivate

#endif // _NET_SERVICES_H_

