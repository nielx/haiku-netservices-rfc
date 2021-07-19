/*
 * Copyright 2013-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <UrlResult.h>


using namespace BPrivate::Network;


BUrlResult::BUrlResult()
	:
	fContentType(),
	fLength(0)
{
}


BUrlResult::~BUrlResult()
{
}


void
BUrlResult::SetContentType(const std::string& contentType)
{
	fContentType = contentType;
}


void
BUrlResult::SetLength(off_t length)
{
	fLength = length;
}


std::string
BUrlResult::ContentType() const
{
	return fContentType;
}


off_t
BUrlResult::Length() const
{
	return fLength;
}
