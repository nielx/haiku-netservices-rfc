/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <deque>
#include <iostream>
#include <sstream>

#include <HttpRequest.h>
#include <HttpResponse.h>
#include <HttpSession.h>
#include <Locker.h>
#include <NetworkAddress.h>

#include "AutoLocker.h"

using namespace BPrivate::Network;


struct BHttpSession::Data {
	// constants (does not need to be locked to be accessed)
	thread_id							controlThread;
	thread_id							dataThread;
	sem_id								controlQueueSem;
	sem_id								dataQueueSem;
	// locking mechanism
	BLocker								lock;
	// control queue
	std::deque<BHttpSession::Wrapper>	controlQueue;
	std::deque<BHttpSession::Wrapper>	dataQueue;
};


struct BHttpSession::Wrapper {
	BHttpRequest					request;
	// Request state/events
	enum {
		kRequestInitialState,
		kRequestConnected,
		kRequestStatusReceived,
		kRequestHeadersReceived,
		kRequestContentReceived,
		kRequestTrailingHeadersReceived
	}				requestStatus = kRequestInitialState;
	// Communication
	std::promise<BHttpResponse>		promise;
	// Data
	BHttpResponse					response;
	BNetworkAddress					remoteAddress;
	std::unique_ptr<BSocket>		socket;
};


BHttpSession::BHttpSession()
	: fData(std::make_shared<Data>())
{
	// set up semaphores for synchronization between data and control thread
	fData->controlQueueSem = create_sem(0, "http:control");
	if (fData->controlQueueSem < 0)
		throw std::runtime_error("Cannot create control queue semaphore");
	fData->dataQueueSem = create_sem(0, "http:data");
	if (fData->dataQueueSem < 0)
		throw std::runtime_error("Cannot create data queue semaphore");
	
	// set up internal threads
	fData->controlThread = spawn_thread(ControlThreadFunc, "http:control", B_NORMAL_PRIORITY, fData.get());
	if (fData->controlThread < 0)
		throw std::runtime_error("Cannot create control thread");
	if (resume_thread(fData->controlThread) != B_OK)
		throw std::runtime_error("Cannot resume control thread");
		
	fData->dataThread = spawn_thread(DataThreadFunc, "http:data", B_NORMAL_PRIORITY, fData.get());
	if (fData->dataThread < 0)
		throw std::runtime_error("Cannot create data thread");
	if (resume_thread(fData->dataThread) != B_OK)
		throw std::runtime_error("Cannot resume data thread");
}


std::future<BHttpResponse>
BHttpSession::AddRequest(BHttpRequest request)
{
	BHttpSession::Wrapper wRequest{std::move(request)};
	auto retval = wRequest.promise.get_future();
	AutoLocker<BLocker>(fData->lock);
	fData->controlQueue.push_back(std::move(wRequest));
	release_sem(fData->controlQueueSem);
	return retval;
}


/*static*/ status_t
BHttpSession::ControlThreadFunc(void* arg)
{
	BHttpSession::Data* data = static_cast<BHttpSession::Data*>(arg);
	while (true) {
		if (auto status = acquire_sem(data->controlQueueSem); status == B_INTERRUPTED)
			continue;
		else if (status != B_OK) {
			// Most likely B_BAD_SEM_ID indicating that the sem was deleted
			break;
		}

		// Process items on the queue
		while (true) {
			data->lock.Lock();
			if (data->controlQueue.empty()){
				data->lock.Unlock();
				break;
			}
			auto request = std::move(data->controlQueue.front());
			data->controlQueue.pop_front();
			data->lock.Unlock();
			
			switch (request.requestStatus) {
				case Wrapper::kRequestInitialState:
				{
					std::cout << "Processing new request" << std::endl;
					if (!_ResolveHostName(request)) {
						// Resolving the hostname failed
						request.promise.set_value(std::move(request.response));
						break;
					}

					if (!_OpenConnection(request)) {
						// Connecting failed
						request.promise.set_value(std::move(request.response));
						break;
					}

					// TODO: further serialization (?)
					
					request.requestStatus = Wrapper::kRequestConnected;
					data->lock.Lock();
					data->dataQueue.push_back(std::move(request));
					data->lock.Unlock();
					release_sem(data->dataQueueSem);
					break;
				}
				default:
				{
					// not handled at this stage
					break;
				}
			}
		}
	}
	return B_OK;
}


/*static*/ status_t
BHttpSession::DataThreadFunc(void* arg)
{
	BHttpSession::Data* data = static_cast<BHttpSession::Data*>(arg);
	while (true) {
		if (auto status = acquire_sem(data->dataQueueSem); status == B_INTERRUPTED)
			continue;
		else if (status != B_OK) {
			// Most likely B_BAD_SEM_ID indicating that the sem was deleted
			break;
		}

		// TODO: good implementation, this is the stupid implementation.
		while (true) {
			data->lock.Lock();
			if (data->dataQueue.empty()) {
				data->lock.Unlock();
				break;
			}
			auto request = std::move(data->dataQueue.front());
			data->dataQueue.pop_front();
			data->lock.Unlock();

			switch (request.requestStatus) {
				case Wrapper::kRequestConnected:
				{
					// Start writing data
					std::string requestHeaders = _CreateRequestHeaders(request);
					request.socket->Write(requestHeaders.data(), requestHeaders.size());
					request.response.status_code = 5;
					request.response.error = BError();
					request.promise.set_value(std::move(request.response));
					// TODO: write Post Data (if applicable)
					//		Idea: add a status kRequestFirstHeaderWritten, and do a non-blocking Write call
					//		to write the rest.
					break;
				}
			}
		}
	}
	return B_OK;
}


/*static*/ bool
BHttpSession::_ResolveHostName(Wrapper& request)
{
	// This helper resolves the address for a given request
	int port = request.request.fSSL ? 443 : 80;
	if (request.request.fUrl.HasPort())
		port = request.request.fUrl.Port();

	// TODO: proxy
	request.remoteAddress.SetTo(request.request.fUrl.Host(), port);
	if (auto status = request.remoteAddress.InitCheck(); status != B_OK) {
		request.response.status_code = 0;
		request.response.error = BError(B_SERVER_NOT_FOUND, "Cannot resolve hostname");\
		return false;
	}

	// TODO: inform any listeners, though maybe that is not the job of this helper?
	return true;
}


/*static*/ bool
BHttpSession::_OpenConnection(Wrapper& request)
{
	// Set up the socket
	std::unique_ptr<BSocket> socket = nullptr;
	if (request.request.fSSL) {
		// To do: secure socket with callbacks to check certificates
		socket = std::make_unique<BSecureSocket>();
	} else {
		socket = std::make_unique<BSocket>();
	}

	// Open connection
	if (auto status = socket->Connect(request.remoteAddress); status != B_OK) {
		// TODO: inform listeners that the connection failed
		request.response.status_code = 0;
		request.response.error = BError(status, "Cannot connect to host");
		return false;
	}

	// TODO: inform the listeners that the connection was opened.
	request.socket = std::move(socket);
	return true;
}

/*static*/ std::string
BHttpSession::_CreateRequestHeaders(Wrapper& request)
{
	std::stringstream headerStream;
	const auto& httpRequest = request.request;

	// Create the first line of the request header
	headerStream << httpRequest.fRequestMethod.Method();
	headerStream << ' ';

	// TODO: proxy

	if (httpRequest.fUrl.HasPath() && httpRequest.fUrl.Path().Length() > 0)
		headerStream << httpRequest.fUrl.Path();
	else
		headerStream << '/';

	switch (httpRequest.fHttpVersion) {
		case B_HTTP_11:
			headerStream << " HTTP/1.1\r\n";
			break;

		default:
		case B_HTTP_10:
			headerStream << " HTTP/1.0\r\n";
			break;
	}

	BHttpHeaders outputHeaders;

	// HTTP 1.1 additional headers
	if (httpRequest.fHttpVersion == B_HTTP_11) {
		BString host = httpRequest.fUrl.Host();
		int defaultPort = httpRequest.fSSL ? 443 : 80;
		if (httpRequest.fUrl.HasPort() && httpRequest.fUrl.Port() != defaultPort)
			host << ':' << httpRequest.fUrl.Port();

		outputHeaders.AddHeader("Host", host);

		outputHeaders.AddHeader("Accept", "*/*");
		outputHeaders.AddHeader("Accept-Encoding", "gzip");
			// Allows the server to compress data using the "gzip" format.
			// "deflate" is not supported, because there are two interpretations
			// of what it means (the RFC and Microsoft products), and we don't
			// want to handle this. Very few websites support only deflate,
			// and most of them will send gzip, or at worst, uncompressed data.

		outputHeaders.AddHeader("Connection", "close");
			// Let the remote server close the connection after response since
			// we don't handle multiple request on a single connection
	}

	// Classic HTTP headers
	if (httpRequest.fOptUserAgent.CountChars() > 0)
		outputHeaders.AddHeader("User-Agent", httpRequest.fOptUserAgent.String());

	if (httpRequest.fOptReferer.CountChars() > 0)
		outputHeaders.AddHeader("Referer", httpRequest.fOptReferer.String());

	// TODO: Optional range requests headers

	// TODO: Authentication

	// TODO: Required headers for POST data

	// TODO: Optional headers specified by the user

	// TODO: Context cookies

	// TODO: proper debug

	// Write output headers to output stream
	for (int32 headerIndex = 0; headerIndex < outputHeaders.CountHeaders();
			headerIndex++) {
		const char* header = outputHeaders.HeaderAt(headerIndex).Header();

		headerStream << header;
		headerStream << "\r\n";
	}

	// End of header text
	headerStream << "\r\n";

	// TODO: proper debug
	std::cout << headerStream.str();

	return headerStream.str();
}	
