/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <UrlRequest.h>

using namespace BPrivate::Network;


BUrlRequest::BUrlRequest(const BUrl& url)
	: fUrl(url)
{
}


BUrlRequest::~BUrlRequest()
{
}


void
BUrlRequest::_EmitDebug(BUrlProtocolDebugMessage type,
	const char* format, ...)
{
//	if (fListener == NULL)
		return;

/*	va_list arguments;
	va_start(arguments, format);

	char debugMsg[1024];
	vsnprintf(debugMsg, sizeof(debugMsg), format, arguments);
	fListener->DebugMessage(this, type, debugMsg);
	va_end(arguments);*/
}
