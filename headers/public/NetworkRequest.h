/*
 * Copyright 2014-2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NET_REQUEST_H_
#define _B_NET_REQUEST_H_

#include <memory>

#include <AbstractSocket.h>
#include <NetBuffer.h>
#include <NetworkAddress.h>
#include <UrlRequest.h>


namespace BPrivate {

namespace Network {


class BNetworkRequest : public BUrlRequest
{
public:
	void SetTimeout(bigtime_t timeout);
protected:
			friend class 		BUrlProtocolRoster;
								BNetworkRequest(const BUrl& url);

			bool 				_ResolveHostName(BString host, uint16_t port);

			void				_ProtocolSetup();
			status_t			_GetLine(BString& destString);

protected:
			std::unique_ptr<BAbstractSocket> fSocket;
			BNetworkAddress		fRemoteAddr;

			BNetBuffer			fInputBuffer;
};


} // namespace Network

} // namespace BPrivate

#endif
