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

#include <cctype>

#include <HttpMethod.h>

using namespace BPrivate::Network;


// BHttpMethod
BHttpMethod::BHttpMethod(std::string method)
	: fMethod(std::move(method))
{
	// RFC 2616, section 5.1.1 defines 8 default methods, and allows extension methods.
	// The extension method must be a token, which in section 2.2 is defined as:
	//		1*<any CHAR except CTLs or separators>, where
	//     		- CHAR < US-ASCII character (octets 0-127) >
	//			- CTL < any US-ASCII control character (octets 0-31) and DEL (127)
	//     		- separators = (see list below)

	if (fMethod.size() == 0)
		throw invalid_method_exception{invalid_method_exception::Empty};

	for (auto it = fMethod.cbegin(); it < fMethod.cend(); it++) {
		if (*it <= 31 || *it == 127 || *it == '(' || *it == ')' || *it == '<' || *it == '>'
				|| *it == '@' || *it == ',' || *it == ';' || *it == '\\' || *it == '"'
				|| *it == '/' || *it == '[' || *it == ']' || *it == '?' || *it == '='
				|| *it == '{' || *it == '}' || *it == ' ')
			throw invalid_method_exception{invalid_method_exception::InvalidCharacter};
	}
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



bool
BHttpMethod::operator==(const BHttpMethod& other) const noexcept
{
	return fMethod == other.fMethod;
}
