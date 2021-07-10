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
	static Expected<std::unique_ptr<BUrlRequest>, BError>
		MakeRequest(const BUrl& url);
};

} // namespace Network

} // namespace BPrivate

#endif
