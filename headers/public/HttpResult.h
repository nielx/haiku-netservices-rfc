/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_RESULT_H_
#define _B_HTTP_RESULT_H_


#include <functional>

#include <ErrorsExt.h>
#include <Expected.h>
#include <SupportDefs.h>


namespace BPrivate {

namespace Network {

class BHttpSession;
class BHttpHeaders;
struct HttpResultPrivate;


struct BHttpStatus {
	int32			code = 0;
	std::string		text;
};


struct BHttpBody {
	std::unique_ptr<BDataIO>	target = nullptr;
	std::string					text;
};


class BHttpResult {
public:
	typedef std::reference_wrapper<BHttpStatus> StatusRef;
	typedef std::reference_wrapper<BHttpHeaders> HeadersRef;
	typedef std::reference_wrapper<BHttpBody> BodyRef;

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
	int32							Identity() const;

	// Destructor, copy and move
									~BHttpResult();
									BHttpResult(const BHttpResult& other) = delete;
									BHttpResult(BHttpResult&& other) = default;
	BHttpResult&					operator=(const BHttpResult& other) = delete;
	BHttpResult&					operator=(BHttpResult&& other) = default;
private:
	friend class BHttpSession;
									BHttpResult(std::shared_ptr<HttpResultPrivate> data);
	std::shared_ptr<HttpResultPrivate>	fData;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_Result_H_
