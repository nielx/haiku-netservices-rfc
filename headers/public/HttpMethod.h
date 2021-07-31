/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_METHOD_H_
#define _B_HTTP_METHOD_H_


#include <ErrorsExt.h>
#include <Expected.h>


namespace BPrivate {

namespace Network {


// Request method
class BHttpMethod {
public:
	// Some standard methods
	static	BHttpMethod			Get();
	static	BHttpMethod			Post();
	static	BHttpMethod			Put();
	static	BHttpMethod			Head();
	static	BHttpMethod			Delete();
	static	BHttpMethod			Options();
	static	BHttpMethod			Trace();
	static	BHttpMethod			Connect();

	// Custom methods
	static	Expected<BHttpMethod, BError>
								Make(std::string method);


	// Constructors and assignment
								BHttpMethod(const BHttpMethod& other) = default;
								BHttpMethod(BHttpMethod&& other) = default;
			BHttpMethod&		operator=(const BHttpMethod& other) = default;
			BHttpMethod&		operator=(BHttpMethod&& other) = default;
private:
								BHttpMethod(std::string method);
			std::string			fMethod;
};

} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_METHOD_H
