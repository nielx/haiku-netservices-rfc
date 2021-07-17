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


#include <HttpRequest.h>


using namespace BPrivate::Network;


BHttpRequest::BHttpRequest(const BUrl& url, bool ssl)
	: BNetworkRequest(url), fSSL(ssl), fRequestMethod(B_HTTP_GET)
{
	
}


BHttpRequest::~BHttpRequest()
{

}
