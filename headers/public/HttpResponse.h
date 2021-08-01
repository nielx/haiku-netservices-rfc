/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_RESPONSE_H_
#define _B_HTTP_RESPONSE_H_

namespace BPrivate {

namespace Network {


struct BHttpResponse {
	int32		status_code;
	BError		error;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_RESPONSE_H_
