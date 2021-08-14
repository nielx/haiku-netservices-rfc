/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#ifndef _HTTP_RESULT_PRIVATE_H_
#define _HTTP_RESULT_PRIVATE_H_


#include <optional>


namespace BPrivate {

namespace Network {

struct HttpResultPrivate {
	// Read-only properties (multi-thread safe)
	const	int32						id;

	// Locking
			sem_id						data_wait;
			enum {
				kNoData = 0,
				kStatusReady,
				kHeadersReady,
				kBodyReady,
				kError
			};
			int32						requestStatus = kNoData;

	// Data
			std::optional<BHttpStatus>	status;
			std::optional<BHttpHeaders>	headers;
			std::optional<std::string>	body;
			std::optional<BError>		error;

	// Utility functions
										HttpResultPrivate(int32 identifier);
			int32						GetStatusAtomic();
			void						SetError(const BError& e);
			void						SetStatus(BHttpStatus&& s);
			void						SetHeaders(BHttpHeaders&& h);
			void						SetBody(std::string&& b);
};


inline
HttpResultPrivate::HttpResultPrivate(int32 identifier)
	: id(identifier)
{
	std::string name = "httpresult:" + std::to_string(identifier);
	data_wait = create_sem(1, name.c_str());
	if (data_wait < B_OK)
		throw std::runtime_error("Cannot create internal sem for httpresult");
}


inline int32
HttpResultPrivate::GetStatusAtomic()
{
	return atomic_get(&requestStatus);
}


inline void
HttpResultPrivate::SetError(const BError& e)
{
	error = e;
	atomic_set(&requestStatus, kError);
	release_sem(data_wait);
}


inline void
HttpResultPrivate::SetStatus(BHttpStatus&& s)
{
	status = std::move(s);
	atomic_set(&requestStatus, kStatusReady);
	release_sem(data_wait);
}


inline void
HttpResultPrivate::SetHeaders(BHttpHeaders&& h)
{
	headers = std::move(h);
	atomic_set(&requestStatus, kHeadersReady);
	release_sem(data_wait);
}


inline void
HttpResultPrivate::SetBody(std::string&& b)
{
	body = std::move(b);
	atomic_set(&requestStatus, kBodyReady);
	release_sem(data_wait);
}


} // namespace Network

} // namespace BPrivate

#endif // _HTTP_RESULT_PRIVATE_H_
