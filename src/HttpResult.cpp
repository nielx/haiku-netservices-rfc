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
	: fStatusFuture(std::move(status)), fHeadersFuture(std::move(headers)), fBodyFuture(std::move(body)), fID(id)
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


Expected<BHttpResult::HeadersRef, BError>
BHttpResult::Headers()
{
	if (fError)
		return Unexpected<BError>(*fError);
	else if (fHeaders)
		return std::ref(*fHeaders);
	try {
		fHeaders = fHeadersFuture.get();
		return std::ref(*fHeaders);
	} catch (BError &e) {
		fError = e;
		return Unexpected<BError>(e);
	}
}


Expected<BHttpResult::BodyRef, BError>
BHttpResult::Body()
{
	if (fError)
		return Unexpected<BError>(*fError);
	else if (fBody)
		return std::ref(*fBody);
	try {
		fBody = fBodyFuture.get();
		return std::ref(*fBody);
	} catch (BError &e) {
		fError = e;
		return Unexpected<BError>(e);
	}
}


bool
BHttpResult::HasStatus()
{
	if (fStatus)
		return true;
	return fStatusFuture.wait_for(std::chrono::nanoseconds::zero()) != std::future_status::timeout;
}


bool
BHttpResult::HasHeaders()
{
	if (fHeaders)
		return true;
	return fHeadersFuture.wait_for(std::chrono::nanoseconds::zero()) != std::future_status::timeout;
}


bool
BHttpResult::HasBody()
{
	if (fBody)
		return true;
	return fBodyFuture.wait_for(std::chrono::nanoseconds::zero()) != std::future_status::timeout;
}


bool
BHttpResult::IsCompleted()
{
	return HasStatus() && HasHeaders() && HasBody();
}


} // namespace Network

} // namespace BPrivate
