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


enum BUrlProtocolDebugMessage {
	B_URL_PROTOCOL_DEBUG_TEXT,
	B_URL_PROTOCOL_DEBUG_ERROR,
	B_URL_PROTOCOL_DEBUG_HEADER_IN,
	B_URL_PROTOCOL_DEBUG_HEADER_OUT,
	B_URL_PROTOCOL_DEBUG_TRANSFER_IN,
	B_URL_PROTOCOL_DEBUG_TRANSFER_OUT
};


class BUrlRequest {
public:
	virtual							~BUrlRequest();

protected:
	friend 	class BUrlProtocolRoster;
	virtual	void					_ProtocolSetup() { };
	virtual status_t				_ProtocolLoop() = 0;
	virtual void					_EmitDebug(BUrlProtocolDebugMessage type,
										const char* format, ...);

protected:
									BUrlRequest(const BUrl &url);

			BUrl					fUrl;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_URL_REQUEST_H_
