/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_TASK_H_
#define _B_HTTP_TASK_H_


#include <functional>
#include <future>
#include <optional>

#include <ErrorsExt.h>
#include <Expected.h>
#include <SupportDefs.h>


namespace BPrivate {

namespace Network {

class BHttpSession;
class BHttpHeaders;


struct BHttpStatus {
	int32			code = 0;
	std::string		text;
};


class BHttpResult {
public:
	typedef std::reference_wrapper<BHttpStatus> StatusRef;
	typedef std::reference_wrapper<BHttpHeaders> HeadersRef;
	typedef std::reference_wrapper<std::string> BodyRef;

	// Blocking Access Functions
	Expected<StatusRef, BError>		Status();
	Expected<HeadersRef, BError>	Headers();
	Expected<BodyRef, BError>		Body();

	// Check if data is available yet
	bool							HasStatus();
	bool							HasHeaders();
	bool							HasBody();
	bool							IsCompleted();

	// Identity
	int32							Identity();

private:
	friend class BHttpSession;
									BHttpResult(std::future<BHttpStatus>&& status,
										std::future<BHttpHeaders>&& headers,
										std::future<std::string>&& body,
										int32 id);
	std::future<BHttpStatus>		fStatusFuture;
	std::future<BHttpHeaders>		fHeadersFuture;
	std::future<std::string>		fBodyFuture;
	std::optional<BHttpStatus>		fStatus;
	std::optional<BHttpHeaders>		fHeaders;
	std::optional<std::string>		fBody;
	std::optional<BError>			fError;
	int32							fID;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_TASK_H_
