/*
 * Copyright 2013-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_URL_ROSTER_H_
#define _B_URL_ROSTER_H_


#include <memory>
#include <stdexcept>

#include <Expected.h>
#include <ErrorsExt.h>


class BDataIO;
class BUrl;

namespace BPrivate {

namespace Network {

class BUrlContext;
class BUrlProtocolListener;
class BUrlRequest;

class BUrlProtocolRoster {
public:

	template<class U>
	static Expected<std::unique_ptr<U>, BError>
		MakeRequest(const BUrl& url)
	{
		static_assert(std::is_base_of<BUrlRequest, U>::value, "It is only possible to retrieve a subclass of BUrlRequest");
		auto urlRequest = _MakeRequest(url);
		if (urlRequest == nullptr)
			return Unexpected<BError>(BError(B_NOT_SUPPORTED, "Protocol not supported"));

		U* request = dynamic_cast<U*>(urlRequest);
		if (request == nullptr) {
			delete urlRequest;
			return Unexpected<BError>(BError(B_BAD_VALUE, "The request is of a different type"));
		}
		return std::unique_ptr<U>(request);
	}

private:
	static BUrlRequest* _MakeRequest(const BUrl& url);
};


template<>
inline Expected<std::unique_ptr<BUrlRequest>, BError>
	BUrlProtocolRoster::MakeRequest<BUrlRequest>(const BUrl& url)
{	
	auto urlRequest = _MakeRequest(url);
	if (urlRequest == nullptr)
		return Unexpected<BError>(BError(B_NOT_SUPPORTED, "Protocol not supported"));
	return std::unique_ptr<BUrlRequest>(urlRequest);
}


} // namespace Network

} // namespace BPrivate

#endif
