/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <HttpHeaders.h>
#include <HttpResult.h>


namespace BPrivate {

namespace Network {


/*private*/
BHttpResult::BHttpResult(std::future<BHttpStatus>&& status,
	std::future<BHttpHeaders>&& headers, std::future<std::string>&& body,
	int32 id)
	: fStatusFuture(std::move(status)), fHeaders(std::move(headers)), fBody(std::move(body)), fID(id)
{
	
}


Expected<BHttpResult::StatusRef, BError>
BHttpResult::Status()
{
	if (fError)
		return Unexpected<BError>(*fError);
	else if (fStatus)
		return std::ref(*fStatus);
	try {
		fStatus = fStatusFuture.get();
		return std::ref(*fStatus);
	} catch (BError &e) {
		fError = e;
		return Unexpected<BError>(e);
	}		
}


} // namespace Network

} // namespace BPrivate
