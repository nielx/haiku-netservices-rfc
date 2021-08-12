/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <SupportDefs.h>


namespace BPrivate {

namespace Network {

namespace UrlEventData {
	const char* Id = "url:identifier";
	const char* HostName = "url:hostname";
	const char* NumBytes = "url:numbytes";
	const char* TotalBytes = "url:totalbytes";
	const char* Success = "url:success";
	const char* DebugType = "url:debugtype";
	const char* DebugMessage = "url:debugmessage";
}


static int32 gRequestIdentifier = 1;


int32
get_netservices_request_identifier()
{
	return atomic_add(&gRequestIdentifier, 1);
}


} // namespace Network

} // namespace BPrivate
