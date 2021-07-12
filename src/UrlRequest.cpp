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
