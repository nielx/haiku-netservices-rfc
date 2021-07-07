/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <stdexcept>

#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

using namespace BPrivate::Network;


std::unique_ptr<BUrlRequest>
BUrlProtocolRoster::MakeRequest(const BUrl& url)
{
	if (url.Protocol() == "http") {
		return std::make_unique<BUrlRequest>(url);
	} else if (url.Protocol() == "https") {
		return std::make_unique<BUrlRequest>(url);
	}
	throw std::invalid_argument("Protocol not supported");
}
