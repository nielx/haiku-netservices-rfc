/*
 * Copyright 2010-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_REQUEST_H_
#define _B_URL_REQUEST_H_


#include <Url.h>


namespace BPrivate {

namespace Network {
	
class BUrlProtocolRoster;


class BUrlRequest {
public:
	BUrlRequest(const BUrl &url) {};
	virtual ~BUrlRequest() {};

};


} // namespace Network

} // namespace BPrivate

#endif // _B_URL_REQUEST_H_
