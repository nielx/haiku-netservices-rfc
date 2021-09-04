/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_METHOD_H_
#define _B_HTTP_METHOD_H_


#include <string>
#include <string_view>


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

	// Exception
	struct invalid_method_exception {
		enum {Empty, InvalidCharacter} reason;
	};

	// Constructors and assignment
								BHttpMethod(std::string method);
								BHttpMethod(const BHttpMethod& other) = default;
								BHttpMethod(BHttpMethod&& other) = default;
			BHttpMethod&		operator=(const BHttpMethod& other) = default;
			BHttpMethod&		operator=(BHttpMethod&& other) = default;

	// Comparison
			bool				operator==(const BHttpMethod& other) const noexcept;

	// String representation
	const	std::string&		Method() const { return fMethod; }
private:
			std::string			fMethod;
};

} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_METHOD_H
