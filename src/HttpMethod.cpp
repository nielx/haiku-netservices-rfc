/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Stephan Aßmus, superstippi@gmx.de
 */

#include <HttpMethod.h>

using namespace BPrivate::Network;
using BPrivate::BError;


// BHttpMethod
BHttpMethod::BHttpMethod(std::string method)
	: fMethod(method)
{

}


BHttpMethod
BHttpMethod::Get()
{
	return BHttpMethod("GET");
}


BHttpMethod
BHttpMethod::Post()
{
	return BHttpMethod("POST");
}


BHttpMethod
BHttpMethod::Put()
{
	return BHttpMethod("PUT");
}


BHttpMethod
BHttpMethod::Head()
{
	return BHttpMethod("HEAD");
}


BHttpMethod
BHttpMethod::Delete()
{
	return BHttpMethod("DELETE");
}


BHttpMethod
BHttpMethod::Options()
{
	return BHttpMethod("OPTIONS");
}


BHttpMethod
BHttpMethod::Trace()
{
	return BHttpMethod("TRACE");
}


BHttpMethod
BHttpMethod::Connect()
{
	return BHttpMethod("CONNECT");
}


Expected<BHttpMethod, BError>
BHttpMethod::Make(std::string method)
{
	 // Todo: check http spec
	 return BHttpMethod(method);
}


bool
BHttpMethod::operator==(const BHttpMethod& other) const
{
	return fMethod == other.fMethod;
}
