/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <HttpHeaders.h>
#include <HttpResult.h>

#include "HttpResultPrivate.h"


namespace BPrivate {

namespace Network {


/*private*/
BHttpResult::BHttpResult(std::shared_ptr<HttpResultPrivate> data)
	: fData(data)
{

}


BHttpResult::~BHttpResult()
{
	if (fData)
		fData->SetCancel();
}


BHttpStatus&
BHttpResult::Status()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		if (dataStatus == HttpResultPrivate::kError)
			std::rethrow_exception(*(fData->error));

		if (dataStatus >= HttpResultPrivate::kStatusReady)
			return *(fData->status);

		status = acquire_sem(fData->data_wait);
	}
	throw std::runtime_error("Unexpected error waiting for status!");
}


BHttpHeaders&
BHttpResult::Headers()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		if (dataStatus == HttpResultPrivate::kError)
			std::rethrow_exception(*(fData->error));

		if (dataStatus >= HttpResultPrivate::kHeadersReady)
			return *(fData->headers);
		
		status = acquire_sem(fData->data_wait);
	}
	throw std::runtime_error("Unexpected error waiting for headers!");
}


BHttpBody&
BHttpResult::Body()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		if (dataStatus == HttpResultPrivate::kError)
			std::rethrow_exception(*(fData->error));

		if (dataStatus >= HttpResultPrivate::kBodyReady)
			return *(fData->body);
		
		status = acquire_sem(fData->data_wait);
	}
	throw std::runtime_error("Unexpected error waiting for body!");
}


bool
BHttpResult::HasStatus()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kStatusReady;
}


bool
BHttpResult::HasHeaders()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kHeadersReady;
}


bool
BHttpResult::HasBody()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kBodyReady;
}


bool
BHttpResult::IsCompleted()
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	return HasBody();
}


int32
BHttpResult::Identity() const
{
	if (!fData)
		throw std::runtime_error("The BHttpResult object is no longer valid");
	return fData->id;
}


} // namespace Network

} // namespace BPrivate
