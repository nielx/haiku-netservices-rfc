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


Expected<BUrlResult, BError>
BUrlProtocolRoster::RunRequest(std::unique_ptr<BUrlRequest> request) {
	//TODO
	return Unexpected<BError>(BError(B_NOT_SUPPORTED, "Not implemented"));
}


BUrlRequest*
BUrlProtocolRoster::_MakeRequest(const BUrl& url)
{
	return nullptr;
}


