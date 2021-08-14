/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <deque>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <DynamicBuffer.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <Locker.h>
#include <NetBuffer.h>
#include <NetServices.h>
#include <NetworkAddress.h>
#include <OS.h>
#include <StackOrHeapArray.h>
#include <ZlibCompressionAlgorithm.h>

#include "AutoLocker.h"
#include "HttpResultPrivate.h"

using namespace BPrivate::Network;


namespace BPrivate::Network::UrlEventData {
	const char* HttpStatus = "url:httpstatus";
	const char* SSLCertificate = "url:sslcertificate";
	const char* SSLMessage = "url:sslmessage";
}


struct BHttpSession::Data {
	// constants (does not need to be locked to be accessed)
	thread_id							controlThread;
	thread_id							dataThread;
	sem_id								controlQueueSem;
	sem_id								dataQueueSem;
	// locking mechanism
	BLocker								lock;
	// queues
	std::deque<BHttpSession::Wrapper>	controlQueue;
	std::deque<BHttpSession::Wrapper>	dataQueue;
	std::vector<int32>					cancelList;
	// data owned by the dataThread
	std::map<int,BHttpSession::Wrapper>	connectionMap;
	std::vector<object_wait_info>		objectList;
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
	BMessenger						observer;
	std::shared_ptr<HttpResultPrivate> result;

	// Connection
	BNetworkAddress					remoteAddress;
	std::unique_ptr<BSocket>		socket;

	// Receive state
	bool							receiveEnd = false;
	bool							parseEnd = false;
	BNetBuffer						inputBuffer;
	size_t							previousBufferSize = 0;
	off_t							bytesReceived = 0;
	off_t							bytesTotal = 0;
	BHttpHeaders					headers;
	bool							readByChunks = false;
	bool							decompress = false;
	DynamicBuffer					decompressorStorage;
	std::unique_ptr<BDataIO>		decompressingStream = nullptr;
	std::vector<char>				inputTempBuffer = std::vector<char>(4096);
	BHttpStatus						status;
	std::string						body;
		// TODO: is this the best way of allocating this temp buffer?
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


BHttpResult
BHttpSession::AddRequest(BHttpRequest request, BMessenger observer)
{
	BHttpSession::Wrapper wRequest{std::move(request)};
	wRequest.observer = observer;
	auto identifier = get_netservices_request_identifier();

	// create shared data
	wRequest.result = std::make_shared<HttpResultPrivate>(identifier);

	auto retval = BHttpResult(wRequest.result);
	AutoLocker<BLocker>(fData->lock);
	fData->controlQueue.push_back(std::move(wRequest));
	release_sem(fData->controlQueueSem);
	return retval;
}


void
BHttpSession::Cancel(int32 identifier)
{
	AutoLocker<BLocker>(fData->lock);
	fData->cancelList.push_back(identifier);
	release_sem(fData->dataQueueSem);
}


void
BHttpSession::Cancel(const BHttpResult& result)
{
	Cancel(result.Identity());
}


static void
SetSocketNonBlocking(int socket)
{
	if (socket < 0)
		throw std::runtime_error("Invalid socket");

	int flags = fcntl(socket, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("Error getting socket flags");
	if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) != 0)
		throw std::runtime_error("Error setting non-blocking flag on socket");
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
					bool hasError = false;
					try {
						_ResolveHostName(request);
						_OpenConnection(request);
					} catch (BError &e) {
						request.result->SetError(e);
						hasError = true;
					}

					if (hasError) {
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


static const uint16 EVENT_CANCELLED = 0x4000;


/*static*/ status_t
BHttpSession::DataThreadFunc(void* arg)
{
	BHttpSession::Data* data = static_cast<BHttpSession::Data*>(arg);
	// initial initialization of wait list
	data->objectList.push_back(object_wait_info{data->dataQueueSem,
		B_OBJECT_TYPE_SEMAPHORE, B_EVENT_ACQUIRE_SEMAPHORE
	});

	while (true) {
		if (auto status = wait_for_objects(data->objectList.data(),
			data->objectList.size()); status < B_OK)
		{
			// Something went unexplicably wrong
			throw std::runtime_error("BHttpSession[dataThread]: error waiting for objects");
		}

		std::cout << "dataThread: objects ready" << std::endl;

		// First check if the change is in acquiring the sem, meaning that
		// there are new requests to be scheduled
		if (data->objectList[0].events == B_EVENT_ACQUIRE_SEMAPHORE) {
			if (auto status = acquire_sem(data->dataQueueSem); status == B_INTERRUPTED)
				continue;
			else if (status != B_OK) {
				// Most likely B_BAD_SEM_ID indicating that the sem was deleted
				break;
			}

			// Process the cancelList and dataQueue. Note that there might
			// be a situation where a request is cancelled and added in the
			// same iteration, but that is taken care by this algorithm.
			data->lock.Lock();
			while (!data->dataQueue.empty()) {
				auto request = std::move(data->dataQueue.front());
				data->dataQueue.pop_front();
				auto socket = request.socket->Socket();
				
				data->connectionMap.insert(std::make_pair(socket, std::move(request)));

				// Add to objectList
				data->objectList.push_back(object_wait_info{socket,
					B_OBJECT_TYPE_FD, B_EVENT_WRITE
				});
			}
		
			for (auto id: data->cancelList) {
				// To cancel, we set a special event status on the
				// object_wait_info list so that we can handle it below.
				// Also: the first item in the waitlist is always the semaphore
				// so the fun starts at offset 1.
				size_t offset = 0;
				for (auto it = data->connectionMap.cbegin(); it != data->connectionMap.cend(); it++) {
					offset++;
					if (it->second.result->id == id) {
						data->objectList[offset].events = EVENT_CANCELLED;
						break;
					}
				}
			}
			data->cancelList.clear();
			data->lock.Unlock();
		} else if ((data->objectList[0].events & B_EVENT_INVALID) == B_EVENT_INVALID) {
			// The semaphore has been deleted. Start the cleanup
			break;
		}

		// Process all objects that are ready
		bool resizeObjectList = false;
		for(auto& item: data->objectList) {
			if (item.type != B_OBJECT_TYPE_FD)
				continue;
			if ((item.events & B_EVENT_WRITE) == B_EVENT_WRITE) {
				// TODO: this is currently not fully in line with non-blocking IO
				std::cout << "> Processing write for " << item.object << std::endl;
				auto& request = data->connectionMap.find(item.object)->second;
				switch (request.requestStatus) {
					case Wrapper::kRequestConnected: 
					{
						// Start writing data
						std::string requestHeaders = _CreateRequestHeaders(request);
						request.socket->Write(requestHeaders.data(), requestHeaders.size());
						//		Idea: add a status kRequestFirstHeaderWritten, and do a non-blocking Write call
						//		to write the rest.
						// TODO: write Post Data (if applicable)
						break;
					}
					default:
						throw std::runtime_error("Write status for object that is in the wrong state");
				}
			} else if ((item.events & B_EVENT_READ) == B_EVENT_READ) {
				std::cout << "Processing read for " << item.object << std::endl;	
				auto& request = data->connectionMap.find(item.object)->second;
				auto finished = false;
				auto success = false;
				try {
					finished = _RequestRead(request);
					if (finished)
						request.result->SetBody(std::move(request.body));
					success = true;
				} catch (BError &e) {
					request.result->SetError(e);
					finished = true;
				}

				if (request.result->CanCancel()) {
					std::cout << "Canceling request because no one is listening" << std::endl;
					// This could be done earlier, but this seems cleaner for the flow
					request.socket->Disconnect();
					data->connectionMap.erase(item.object);
					resizeObjectList = true;
				} else if (finished) {
					request.socket->Disconnect();
					if (request.observer.IsValid()) {
						BMessage msg(UrlEvent::RequestCompleted);
						msg.AddInt32(UrlEventData::Id, request.result->id);
						msg.AddBool(UrlEventData::Success, success);
						request.observer.SendMessage(&msg);
					}
					data->connectionMap.erase(item.object);
					resizeObjectList = true;
				}
			} else if ((item.events & B_EVENT_DISCONNECTED) == B_EVENT_DISCONNECTED) {
				std::cout << "Unexpected disconnect for " << item.object << std::endl;
				auto& request = data->connectionMap.find(item.object)->second;
				request.result->SetError(BError(B_IO_ERROR, "Connection was closed unexpectedly"));
				if (request.observer.IsValid()) {
					BMessage msg(UrlEvent::RequestCompleted);
					msg.AddInt32(UrlEventData::Id, request.result->id);
					msg.AddBool(UrlEventData::Success, false);
					request.observer.SendMessage(&msg);
				}
				data->connectionMap.erase(item.object);
				resizeObjectList = true;
			} else if ((item.events & EVENT_CANCELLED) == EVENT_CANCELLED) {
				std::cout << "Cancel request for " << item.object << std::endl;
				auto& request = data->connectionMap.find(item.object)->second;
				request.socket->Disconnect();
				request.result->SetError(BError(B_CANCELED, "Request cancelled by user"));
				if (request.observer.IsValid()) {
					BMessage msg(UrlEvent::RequestCompleted);
					msg.AddInt32(UrlEventData::Id, request.result->id);
					msg.AddBool(UrlEventData::Success, false);
					request.observer.SendMessage(&msg);
				}
				data->connectionMap.erase(item.object);
				resizeObjectList = true;
			} else {
				// Likely to be B_EVENT_INVALID. This should not happen
				throw std::runtime_error("Socket was deleted at an unexpected time");
			}
		}

		// Reset objectList
		data->objectList[0].events = B_EVENT_ACQUIRE_SEMAPHORE;
		if (resizeObjectList) {
			data->objectList.resize(data->connectionMap.size() + 1);
		}
		auto i = 1;
		for (auto it = data->connectionMap.cbegin(); it != data->connectionMap.cend(); it++) {
			data->objectList[i].object = it->first;
			if (it->second.requestStatus == Wrapper::kRequestInitialState)
				throw std::runtime_error("Invalid state of request");
			else if (it->second.requestStatus == Wrapper::kRequestInitialState)
				data->objectList[i].events = B_EVENT_WRITE | B_EVENT_DISCONNECTED;
			else
				data->objectList[i].events = B_EVENT_READ | B_EVENT_DISCONNECTED;
			i++;
		}
	}
	return B_OK;
}


/*static*/ void
BHttpSession::_ResolveHostName(Wrapper& request)
{
	// This helper resolves the address for a given request
	int port = request.request.fSSL ? 443 : 80;
	if (request.request.fUrl.HasPort())
		port = request.request.fUrl.Port();

	// TODO: proxy
	request.remoteAddress.SetTo(request.request.fUrl.Host(), port);
	if (auto status = request.remoteAddress.InitCheck(); status != B_OK)
		throw BError(B_SERVER_NOT_FOUND, "Cannot resolve hostname");
}


/*static*/ void
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
		throw BError(status, "Cannot connect to host");
	}

	// Make the rest of the interaction non-blocking
	SetSocketNonBlocking(socket->Socket());

	// TODO: inform the listeners that the connection was opened.
	request.socket = std::move(socket);
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
		if (bytesRead == B_WOULD_BLOCK) {
			std::cout << "WOULD BLOCK" << std::endl;
			return false;
		}
		std::cout << "_RequestRead bytesRead: " << bytesRead << std::endl;

		if (bytesRead < 0) {
			throw BError(bytesRead, "Error reading data from host");
		} else if (bytesRead == 0) {
			// Check if we got the expected number of bytes.
			// Exceptions:
			// - If the content-length is not known (bytesTotal is 0), for
			//   example in the case of a chunked transfer, we can't know
			// - If the request method is "HEAD" which explicitly asks the
			//   server to not send any data (only the headers)
			if (request.bytesTotal > 0 && request.bytesReceived != request.bytesTotal) {
				throw BError(B_IO_ERROR, "Error reading data from host: unexpected end of data");
			}
			request.receiveEnd = true;
		}
		request.inputBuffer.AppendData(chunk.data(), bytesRead);
	} else
		bytesRead = 0;

	request.previousBufferSize = request.inputBuffer.Size();

	if (request.requestStatus < Wrapper::kRequestStatusReceived) {
		_ParseStatus(request);

		if (request.status.code != 0) {
			// the status headers are now received, decide what to do next

			if (request.request.fOptFollowLocation
				&& request.request.IsRedirectionStatusCode(request.status.code))
					// do nothing for now, in the original code this disables the listener
					;

			// TODO: move?
			request.result->SetStatus(BHttpStatus(request.status));

			if (request.request.fOptStopOnError
				&& request.status.code >= B_HTTP_STATUS_CLASS_CLIENT_ERROR)
			{
				return true; // we will not continue anymore
			}
			// TODO: inform listeners of receiving the status code
		}
	}

	if (request.requestStatus < Wrapper::kRequestHeadersReceived) {
		_ParseHeaders(request);

		if (request.requestStatus >= Wrapper::kRequestHeadersReceived) {
			// TODO: move headers instead???
			request.result->SetHeaders(BHttpHeaders(request.headers));

			// TODO: Parse received cookies

			// TODO: let the receivers know that the headers have been received

			// transfer-encoding
			try {
				if (std::string(request.headers["Transfer-Encoding"]) == "chunked")
					request.readByChunks = true;
			} catch (std::logic_error) {
				// header not found
			}

			// content-encoding
			try {
				std::string contentEncoding(request.headers["Content-Encoding"]);
				if (contentEncoding == "gzip" || contentEncoding == "deflate") {
					request.decompress = true;
					BDataIO* stream = nullptr;
					auto result = BZlibCompressionAlgorithm()
						.CreateDecompressingOutputStream(&request.decompressorStorage,
							NULL, stream);
					if (result != B_OK) {
						throw BError(result, "Could not create decompression stream");
					}
					request.decompressingStream = std::unique_ptr<BDataIO>(stream);
				}
			} catch (std::logic_error) {
				// header not found
			}

			// content-length
			try {
				std::string contentLength(request.headers["Content-Length"]);
				request.bytesTotal = std::stol(contentLength);
			} catch (std::logic_error) {
				// header not found or malformed
				request.bytesTotal = -1;
			}

			if (request.request.fRequestMethod == BHttpMethod::Head()
				|| request.status.code == 204) {
				// In the case of a HEAD request or if the server replies
				// 204 ("no content"), we don't expect to receive anything
				// more, and the socket will be closed.
				return true;
			}
		}
	}

	if (request.requestStatus >= Wrapper::kRequestHeadersReceived) {
		// If Transfer-Encoding is chunked, we should read a complete
		// chunk in buffer before handling it
		if (request.readByChunks)
			throw std::runtime_error("Not implemented");
		else {
			bytesRead = request.inputBuffer.Size();

			if (bytesRead > 0) {
				if (request.inputTempBuffer.size() < bytesRead)
					request.inputTempBuffer.resize(bytesRead);
				request.inputBuffer.RemoveData(request.inputTempBuffer.data(), bytesRead);
			}
		}

		if (bytesRead >= 0) {
			request.bytesReceived += bytesRead;

			// NOTE: the original version has a mode where the user does not
			// want the output and is not listening to the outcome, a sort of
			// zombie version. The new version will not support that.
			if (request.decompress) {
				auto status = request.decompressingStream->WriteExactly(
					request.inputTempBuffer.data(), bytesRead);
				if (status != B_OK) {
					throw BError(status, "Error decompressing data");
				}
				ssize_t size = request.decompressorStorage.Size();
				BStackOrHeapArray<char, 4096> buffer(size);
				size = request.decompressorStorage.Read(buffer, size);
				if (size > 0) {
					// TODO: support other output mechanisms
					request.body.append(buffer, size);
					// TODO: notify listeners
				}
			} else {
				request.body.append(request.inputTempBuffer.data(), bytesRead);
				// TODO: notify listener
			}
			
			if (request.bytesTotal >= 0 && request.bytesReceived >= request.bytesTotal)
				request.receiveEnd = true;

			if (request.decompress && request.receiveEnd) {
				auto status = request.decompressingStream->Flush();

				if (status != B_OK && status != B_BUFFER_OVERFLOW) {
					throw BError(status, "Error flushing decompression stream");
				}

				ssize_t size = request.decompressorStorage.Size();
				BStackOrHeapArray<char, 4096> buffer(size);
				size = request.decompressorStorage.Read(buffer, size);
				if (size > 0) {
					request.body.append(buffer, size);
					// TODO: notify listener
				}
			}
		}
		request.parseEnd = (request.inputBuffer.Size() == 0);
	}

	if (request.receiveEnd && request.parseEnd)
		return true; //done
	return false;
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
		request.status.code = std::stol(statusCodeStr);
	} catch (std::invalid_argument) {
		std::cout << "Error getting status code" << std::endl;
		return;
	}
	
	request.status.text = std::string(statusLine.value().begin() + 13, statusLine.value().end());

	// TODO: EmitDebug
	std::cout << "Status line received: Code " << request.status.code
		<< " (" << request.status.text << ")" << std::endl;
}

/*static*/ void
BHttpSession::_ParseHeaders(Wrapper& request)
{
	while (true) {
		auto currentHeader = GetLine(request.inputBuffer);
		if (!currentHeader)
			return;

		// An emtpy line means the end of the header section
		if (currentHeader.value().size() == 0) {
			std::cout << "End of Headers" << std::endl;
			request.requestStatus = Wrapper::kRequestHeadersReceived;
			return;
		}

		// TODO: EmitDebug
		std::cout << "Header Received: " << currentHeader.value() << std::endl;
		request.headers.AddHeader(currentHeader.value().c_str());
	}
}
