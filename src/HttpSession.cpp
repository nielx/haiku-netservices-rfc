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
#include <StackOrHeapArray.h>

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

	// Connection
	BHttpResponse					response;
	BNetworkAddress					remoteAddress;
	std::unique_ptr<BSocket>		socket;

	// Receive state
	bool							receiveEnd = false;
	bool							parseEnd = false;
	BNetBuffer						inputBuffer;
	size_t							previousBufferSize = 0;
	off_t							bytesReceived;
	off_t							bytesTotal;
	// TODO: reset method to reset Connection and Receive State when redirected
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
					// TODO: write Post Data (if applicable)
					while (!_RequestRead(request)) {
						// wait
					}
					request.response.status_code = 5;
					request.response.error = BError();
					request.promise.set_value(std::move(request.response));
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


static const size_t kHttpBufferSize = 4096;


/*static*/ bool
BHttpSession::_RequestRead(Wrapper& request)
{
	// There are two actions combined; one is to receive data, the other is to
	// decode downloaded data in the buffer.
	//
	// Once all data is received (or there is an error), return true to
	// indicate that all the reading and processing is completed.

	// First check if we are going to download data
	size_t bytesRead = 0;
	if ((!request.receiveEnd) && (request.inputBuffer.Size() == request.previousBufferSize)) {
		std::array<char, kHttpBufferSize> chunk;
		bytesRead = request.socket->Read(chunk.data(), kHttpBufferSize);
		std::cout << "_RequestRead bytesRead: " << bytesRead << std::endl;

		if (bytesRead < 0) {
			request.response.status_code = 0;
			request.response.error = BError(bytesRead, "Error reading data from host");
			return true;
		} else if (bytesRead == 0) {
			// Check if we got the expected number of bytes.
			// Exceptions:
			// - If the content-length is not known (bytesTotal is 0), for
			//   example in the case of a chunked transfer, we can't know
			// - If the request method is "HEAD" which explicitly asks the
			//   server to not send any data (only the headers)
			if (request.bytesTotal > 0 && request.bytesReceived != request.bytesTotal) {
				request.response.status_code = 0;
				request.response.error = BError(B_IO_ERROR, "Error reading data from host: unexpected end of data");
				return true;
			}
			request.receiveEnd = true;
		}
		request.inputBuffer.AppendData(chunk.data(), bytesRead);
	} else
		bytesRead = 0;
	
	request.previousBufferSize = request.inputBuffer.Size();
	
	if (request.requestStatus < Wrapper::kRequestStatusReceived) {
		_ParseStatus(request);

		if (request.request.fOptFollowLocation
			&& request.request.IsRedirectionStatusCode(request.response.status_code))
				// do nothing for now, in the original code this disables the listener
				;

		if (request.request.fOptStopOnError
			&& request.response.status_code >= B_HTTP_STATUS_CLASS_CLIENT_ERROR)
				return true; // we will not continue anymore

		// TODO: inform listeners of receiving the status code
	}

	// Set temporary error
	request.response.status_code = 5;
	request.response.error = BError();
	return true;
}


// Helper to extract a single line from a BNetBuffer
static Expected<std::string, status_t>
GetLine(BNetBuffer& buffer)
{
	size_t characterIndex = 0;
	while ((characterIndex < buffer.Size())
		&& ((buffer.Data())[characterIndex] != '\n'))
		characterIndex++;

	if (characterIndex == buffer.Size()) {
		return Unexpected<status_t>(B_ERROR);
	}

	// FUTURE: BNetBuffer requires an extra copy. It should be possible
	// to copy data directly from the pointer to the buffer, and then ask
	// the object to drop the number of bytes instead of forcing a double
	// copy or some evil casting.
	BStackOrHeapArray<char, 4096> tempData(characterIndex + 1);
	if (!tempData.IsValid())
		return Unexpected<status_t>(B_NO_MEMORY);

	buffer.RemoveData(tempData, characterIndex + 1);

	if (characterIndex != 0 && tempData[characterIndex -1] == '\r')
		return std::string(tempData, characterIndex -1);
	else
		return std::string(tempData, characterIndex);
}


/*static*/ void
BHttpSession::_ParseStatus(Wrapper& request)
{
	auto statusLine = GetLine(request.inputBuffer);
	if (!statusLine)
		return;
	if (statusLine.value().size() < 12)
		return;

	std::string statusCodeStr(statusLine.value(), 9, 3);
	try {
		request.response.status_code = std::stol(statusCodeStr);
	} catch (std::invalid_argument) {
		std::cout << "Error getting status code" << std::endl;
		return;
	}
	
	request.response.status_text = std::string(statusLine.value().begin() + 13, statusLine.value().end());

	// TODO: EmitDebug
	std::cout << "Status line received: Code " << request.response.status_code
		<< " (" << request.response.status_text << ")" << std::endl;

	request.requestStatus = Wrapper::kRequestStatusReceived;
}
