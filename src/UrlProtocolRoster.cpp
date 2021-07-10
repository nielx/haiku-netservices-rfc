/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <SupportDefs.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

using namespace BPrivate::Network;
using BPrivate::BError;


BUrlRequest*
BUrlProtocolRoster::_MakeRequest(const BUrl& url)
{
	if (url.Protocol() == "http") {
		return new BUrlRequest(url);
	} else if (url.Protocol() == "https") {
		return new BUrlRequest(url);
	}
	return nullptr;
}
