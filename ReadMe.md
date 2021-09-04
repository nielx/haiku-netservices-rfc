# Network Services Kit Overview

> This document proposes an updated design for the Haiku Network Services Kit. It is written as a usage document that guides future developers on how to use the kit in their applications, thus showcasing the general outline and not all the details of the API yet. The code examples are not final, and can change later on.

Haiku currently has a 'Network Services Kit' as part of it's standard library. The goal is to provide a standard interface to execute network requests to a variety of protocols, in a way that integrates cleanly with Haiku's API design. The current API has three problems:

1. While the API is designed to be asynchronous, every request spins up in a separate thread, which is an efficient way of using system resources.
2. The API design is quite prescriptive and does not work well for all protocols. There are known issues implementing the FTP protocol.
3. The API implemented a callback interface, which executed callbacks within the context of the dedicated request thread. This is problematic, as it does not impose any locking on the code that is executed, and thus invites data races.

This proposal tries to remedy these issues by giving more flexibility for specialized protocols that follow common API conventions as described in sections A.1 and A.2. Part B shows the example implementation for the HTTP protocol. There is also a common interface, the `BUrlDownload` interface, which abstracts over the underlying protocols and allows a user to fetch the data at a specified URL. This is described in section C. Additionally, this proposal aims to modernize some of the conventions used in the API for modern C++, as described in section A.3. It also proposes some changes for modern error handling, as described in section A.4.

> This proposal comes with some testing code. This is work in progress, and should be considered a test implementation rather than a reference implementation. It is not fully functional and it does not adhere to the Haiku coding standards. It is also modern C++ only.

## Part A: Principles & Conventions

This document proposes a few principles of the Network Services Kit.

### A.1 Specialized Protocols with Common API conventions

One of the major design choices is that the library will provide specialized implementations of each protocol, which are independent from one another and will offer different options and practices based on the particularities of the protocol. This differs from the implementation in the current library, where API uniformity was implemented through virtual interfaces. In the current library `BHttpRequest` implements the standard interface defined by `BUrlRequest` through inheritance. The issue there is that the nature of protocols varies, and as such this one size fits all may cause issues in particular cases. Even for the most common of all protocols - HTTP - the interface was suboptimal, making it difficult to implement the HTTP 2 and HTTP 3 protocols, or even to optimize the resource usage of existing calls.

The new library instead focuses on standardization of behavior and conventions, rather than standardizing through interfaces. The following components are expected to be part of each protocol implementation:

* A session object, like `BHttpSession`, with the following properties:
  * Scheduling and execute requests
  * Canceling requests.
  * Store and apply properties to more than one request (like cookies, authentication or SSL certificate exceptions)
  * Consistent interface that allows the user to determine how and where data from the network gets stored.
  * Internally thread-safe, shallowly copyable objects that can be used in different parts of the application.
* A type of request object, like `BHttpRequest`, that sets up the properties for individual requests. When setting properties, the API should be designed in such a way that as many compile time checks are done to make sure that all the individual options are valid, though it may be complex to validate the exact combination of parameters. It is not unimaginable that there can be protocols that do not need a request object because there are no parameters or options. In that case, one might be able to schedule a request by passing in a `BUrl` directly into the session.
* A type of result object, like `BHttpResult`. This container has a double function. First, it is a container for the result of the request, which for HTTP is the status, headers and the body. Secondly, it functions as a synchronization mechanism that allows you to wait for this data (or an error!) to become available. Running requests are identified by a unique `int32` identifier: the result object will contain that identifier as well so that it can be monitored by the asynchronous interface (see next).

### A.2 Standardized Asynchronous Callbacks

All protocols supported by the Haiku Network Service Kit support a uniform asynchronous event message interface, that can inform BLooper/BHandlers about the progress of the request. This allows, for example, a window to schedule a request, and then continue its normal event loop while it waits for the final data to become available. Section B.6 shows the types of messages, and their contents.

### A.3 Errors and Error Handling using exceptions

The API proposal uses exceptions for error handling. The exceptions are implemented using the following principles:

* The consumer of the API must assume that all methods and functions of this API can potentially throw an exception, including constructors, except for when they are explicitly marked as `noexcept`.
* All exceptions are implemented as classes. This means that `status_t` and other fundamentals will never be used as an exception base.
* In the API design, the principles of discoverability and utility are followed to determine the error system:
  * Discoverability means that it should be reasonably clear from the code and documentation which exceptions are thrown and under what circumstances.
  * Utility means that the exceptions themselves are useful for the user of the API to act on, both for simple use cases (something went wrong, so I need to recover from that) to more complex use cases where one may need to know the reason of the exception so that the program can act on that (or require the user to act on it). 
* In order to support that, there are three levels of exceptions defined;
  * The **local exceptions** are exceptions that will only be thrown by objects of a specific type. The example in this kit is the `BHttpMethod::invalid_method_exception` which will never be used outside of a `BHttpMethod` object.
  * **Kit exceptions** are exception types that can be thrown by multiple classes and methods in a single kit. Examples in this kit are the `unsupported_protocol_exception` and the `invalid_url_exception` (both can be found in `NetServices.h`).
  * **System exceptions** are a <u>limited</u> list of errors that can be thrown throughout the API, commonly when there are resource constraints around memory or other system resources. Currently the only accepted exception is the `std::bad_alloc` exception that is raised when an allocation fails.
* The public API will be implemented in such a way that these rules are followed.

### A.4 Use of modern C++ (C++17)

The Network Services Kit API proposal makes the conscious choice to start implementing modern C++, both in the public API, as well as in the implementation. This means amongst other things, that the following language features will be used:

* Move semantics will be used where possible, reducing the number of objects created using `new` and reducing the risk of memory leaks.
* All publicly classes will have explicit implementations of copy/move constructors and assignment (rule of 5).
* Smart pointers such as `std::unique_ptr` and `std::shared_ptr` are used, even in public interfaces and preferred over existing homegrown helpers like `BReferencable`. 

The result is that the library and its functions will *only* be available on *modern platforms* and **not x86_gcc2**.

> Haiku comes with GCC 8.3.0. This defaults to C++14 support. In order to use C++17, pass the -std=c++17 parameter.

## Part B: HTTP Sessions and HTTP Requests

The most common modern protocol is HTTP. Haiku therefore has a good support for building and running requests for this protocol.

### B.1 `BHttpSession` as the executor of requests

All requests start from a `BHttpSession`. This class has the following jobs:

* Store data used between various HTTP calls
  * Proxies
  * Cookies
  * Additional SSL certificates
  * Authentication Data
* Manage the scheduling and execution of HTTP requests.

Objects of the `BHttpSession` class can be shared between different parts of the application. They should be copied, rather than shared using pointers. This is because they have an inner state that is shared between the various objects.

```c++
// Creating and sharing a session
auto session = BHttpSession();

// A copy is passed to window1 and window2, which share the same session data
auto window1 = new WindowWithSession(session);
auto window2 = new WindowWithSession(session);

// Add a cookie to the session, this cookie will be used in window1 and window2
BNetworkCookie cookie("key", "value", BUrl("https://example.com/"));
session.AddCookie(std::move(cookie));

// The session data persists, even if the original session goes out of scope
```

### B.2 Creating HTTP Requests

In order to set up a HTTP request, you create a `BHttpRequest` object. 

```c++
auto url = BUrl("https://www.haiku-os.org");
	// BUrl is part of the legacy support kit, and does not use exceptions
try {
	auto request = BHttpRequest(url, BHttpMethod::Get());
	// ...
} catch (...) {
    // error handling
}
```

> Note that in the example above, wrapping this particular use of the API in a try block could be skipped, as we  know that the URL is valid (so `invalid_url_exception` will not be thrown and we know the protocol is supported `unsupported_protocol_exception`). While `std::bad_alloc` might be raised in low memory situations, it is arguably a choice whether the application developer really wants to put in the effort to recover in those particular conditions.

### B.3 Setting up the HTTP Request

The `HttpRequest` object can be used to set up various properties. For example, you may want to configure whether or not cookies should be set, or whether there may be redirections to be followed. You can set these options on the `BHttpRequest` object.

### B.4 Scheduling a HTTP Request

When you are done setting up all the options, you can start to schedule the request to be executed within the context of a `BHttpSession`. When you schedule a request, you will need to choose how you want to store the incoming data. There are three options:

1. You let the `BHttpSession` create an in-memory buffer with the response body. After the request has finished, you can then use this response buffer to further work with it.
2. You can provide an object that implements the `BDataIO` interface. While the request is being executed, this object is exclusively owned by the Network Services Kit. After the request is finished, you can take back ownership and process it further. You can use this to write the data directly to disk, by creating a `BFile` object.
3. You can provide a `BMemoryRingIO` object, which is designed to give thread-safe read and write access to a common buffer. You can use this construct for when you want to stream HTTP data, meaning that you want to process data while the request is running.

This translates in the following two methods on `BHttpSession`:

```c++
// x86 and x86_64 with C++17 support
class BHttpSession {
    // ...
    BHttpResult AddRequest(BHttpRequest request,
                           std::unique_ptr<BDataIO> target = nullptr,
                           BMessenger observer = BMessenger());
    BHttpResult AddRequest(BHttpRequest request, std::shared_ptr<BMemoryRingIO> target,
                           BMessenger observer = BMessenger());
};
```

### B.5 Synchronously Waiting for the HTTP response

Once a request has been added to a session, you will receive a `BHttpResult` handle. This object allows you to receive the parts HTTP response once they become available. The response is split up in three parts that can be accessed as they come available during the request in the following order:

1. The status, represented by a `BHttpStatus` object, by using the `BHttpResult::Status()` method.
2. The headers, represented by a`BHttpHeaders` object, by using the `BHttpResult::Headers()` method.
3. The body, represented by a`BHttpBody` object, by using the `BHttpResult::Body()` method.

Each of these calls will only return once the data is available, or when the request has ended because of an error. Note that for the error, it does not matter which part of the request has failed. That means that you can call `BHttpRequest::Body()` and you will still receive an error object when the request has failed.

```c++
auto url = BUrl("http://obviouslyinvalidhost.invalid/");
try {
	auto request = BHttpRequest(url, BHttpMethod::Get());
	auto result = session.AddRequest(std::move(request));
    auto& body_text = result.Body().text;
    
} catch (const invalid_url_exception&) {
    // should never be called as the URL is valid
} catch (const unsupported_protocol_exception&) {
    // should never be called as the protocol is supported
} catch (const url_request_exception& e) {
    // will be called, because the hostname will not be able to be resolved
    ...
}
```

> Note that the actual network requests are still handled in separate threads that are managed by the `BHttpSession` object, and are thus executed asynchronously. This has the added advantage that you can do 'other things' after kicking off the request, and then when you are done wait until the result is available, which might have been finished in parallel already.

### B.6 Asynchronous Handling of the Result

In GUI applications, networking operations are often triggered by a user action. For example, downloading a file will be initiated by the user clicking a button. When you initiate that act  ion in the window's thread, and you block the message loop until the request is finished, the user will be left with a non-responsive UI. That is why one would usually run a network request asynchronously. And instead of checking the status every few CPU cycles, you'd want to be proactively informed when something important happens, like the progress of the download or a signal when the request is finished.

The Network Services kit support using the Haiku API's Looper and Handler system to keep you up to date about relevant events that happen to the requests.

The following messages are available for all requests (HTTP and other). The messages below are in the order that they will arrive (when applicable).

| **Message Constant**         | Description                                                  | **Applies to**                                               | **Additional data**                                          |
| ---------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `UrlEvent::HostnameResolved` | The hostname has been resolved.<br />*This message is even sent when you set an IP-address in the URL object.* | All protocols that use network connections.                  | `UrlEventData::Id` (uint32) `UrlEventData::Hostname` (string) |
| `UrlEvent::ConnectionOpened` | The connection to the remote server is opened. After this data will be written. | All protocols that use network connections.                  | `UrlEventData::Id` (uint32)                                  |
| `UrlEvent::UploadProgress`   | If there is a request body to be sent, this informs you of the progress. | All protocols that use network connections and support writing data to the server (like HTTP(S)). | `UrlEventData::Id` (uint32)<br />`UrlEventData::NumBytes` (off_t)<br />`UrlEventData::TotalBytes` (off_t) |
| `UrlEvent::ResponseStarted`  | The data is about to be downloaded and stored in the target. | All protocols.                                               | `UrlEventData::Id` (uint32)                                  |
| `UrlEvent::DownloadProgress` | Data is coming in over the network                           | All protocols that use network connections.                  | `UrlEventData::Id` (uint32)<br />`UrlEventData::NumBytes` (off_t)<br />`UrlEventData::TotalBytes` (off_t) |
| `UrlEvent::BytesWritten`     | An interim update on how many bytes have been written to the target. | All protocols.                                               | `UrlEventData::Id` (uint32)<br />`UrlEventData::NumBytes` (off_t) |
| `UrlEvent::RequestCompleted` | The request is completed and all the data is written to the target, or there was an error. | All protocols.                                               | `UrlEventData::Id` (uint32)<br />`UrlEventData::Success` (bool) |
| `UrlEvent::DebugMessage`     | Additional debug information on the request.<br />*This is enabled or disabled per request.* | All protocols.                                               | `UrlEventData::Id` (uint32)<br />`UrlEventData::DebugType` (int)<br />`UrlEventData::DebugMessage` (string) |

In addition, the HTTP protocol defines the following three additional messages:

| **Message Constant**            | Description                                           | **Applies to** | **Additional data**                                          |
| ------------------------------- | ----------------------------------------------------- | -------------- | ------------------------------------------------------------ |
| `UrlEvent::HttpStatus`          | The status in the server response                     | HTTP(S)        | `UrlEventData::Id` (uint32) `UrlEventData::HttpStatus` (int32) |
| `UrlEvent::HttpHeaders`         | The HTTP headers have been received and are available | HTTP(S)        | `UrlEventData::Id` (uint32)                                  |
| `UrlEvent::SSLCertificateError` | There was an error validating the SSL certificate.    | HTTPS          | `UrlEventData::Id` (uint32) `UrlEventData::SSLCertificate` (BCertificate) `UrlEventData::SSLMessage` (string) |

Note that all messages have a `UrlEventData::Id` data field in the message. This matches up with the identifier in each of the URL requests. These will be unique, even if you are dealing with different protocols.

Example:

```c++
void
MyWindow::MessageReceived(BMessage *msg)
{
    switch (msg->what) {
        case UrlEvent::DownloadProgress:
            {
            	auto identifier = msg->GetInt32(UrlEventData::Id, -1);
                if (fResult.Identifier() == identifier) {
                    off_t numBytes = msg->GetInt64(UrlEventData::NumBytes, 0);
                    off_t totalBytes = msg->GetInt64(UrlEventData::TotalBytes, 0);
                    _UpdateProgress(numBytes, totalBytes);
                    	// notify user in UI
                }
                return;
            }
        case UrlEvent::RequestCompleted:
            {
            	auto identifier = msg->GetInt32(UrlEventData::Id, -1);
                if (fResult.Identifier() == identifier) {
                    // The following call will not block, because we have been notified
                    // that the request is done.
                    try {
                        _DisplayHttpResult(fResult.Body());
                    } catch (const url_request_exception& e) {
                        _DisplayError(e);
                    }
                }
            }
            return;
        }
    }
    BWindow::MessageReceived(msg); // call the parent handler for other messages
}
```

### B.7 Canceling a HTTP request

There may come the day that you are no longer interested in the outcome of a HTTP request you scheduled. In that case you can cancel it. There are two ways to cancel the request.

The first is to simply let the `BHttpResult` object get out of scope. The session that is running the request will (eventually) notice that there is no one listening anymore, and disconnect from the server and free the resources.

The alternative is to use the `BHttpSession::Cancel(int32 id)` or the `BHttpSession::Cancel(const BHttpRequest& request)` methods. These methods will actively cancel the request by closing the network connection as soon as possible. If you are really invested in seeing it all the way through, you can call `BHttpRequest::Body()` on your result object and wait for the request to be completed. You can also handle that asynchronously, as the `UrlEvent::RequestCompleted` message will be sent to an observer when it is done.

> The most common use case for using `BHttpSession::Cancel()` would be to be able to regain ownership of the `BDataIO*` object you passed for the data. Remember that it would be deleted if you just let the `BHttpResult` go out of scope.

### B.8 Handling the result data

In the previous sections, it was outlined how to make a request, how to set options, how to schedule it, and how to wait for the result. Now we are at the stage where it is time to consume the fruits of the labour.

When working with the `BHttpResult` object, know that there are different states:

- There could have been an error during the request. These are usually IO errors, such as errors on connecting to the server, or errors during downloading/uploading. When the result object is in such an error state, all the calls to access the result data will fail.
- The HTTP request itself resulted in an unsuccessful server result. The HTTP status code would be in the 4xx series if it was deemed a user error, or in the 5xx series if it was a server error. Note that the HTTP protocol does support transmitting data in those cases, thus there may be a body in that case.
- The HTTP request was successful resulting as demonstrated by a status code of 2xx.



## Part C: Introducing `BUrlDownload`

Often you want to be able to easily get data at a location identified by a URL. Many protocols provide an easy way to get data from a URL, without the need for complex configuration of the request. For example take HTTP, where a Get call which in a lot of cases just works, no configuration needed. An FTP transfer may be more complex under the hood, as it requires a back and forth between the client and server in order to establish the download, but on the face of it, for many calls one just needs a URL and a client that knows how to fetch it.

The Network Services kit implements the high level abstraction that allows you to easily fetch data from URLs, without having to be aware of the underlying protocol and its peculiarities. It provides an easy interface with the following functionality:

* Synchronously wait for completion of downloads (C1)
* Asynchronously download data from a URL and listen for progress (C2)
* Get access to some of the internals of the system to add some level of configuration (C3)

 ### C.1 Creating and synchronously wait for a download

Starting a download is as easy as creating a valid `BUrl` object, and asking the `BUrlDownload` class to start a download. All the static members of `BUrlDownload` are thread-safe and can be called from any of your threads. When successful, the `BUrlDownload` object will be the handle that can be used to retrieve the body.

The prototype of the static factory function looks like this:

```c++
// x86 and x86_64 with C++17 support
class BUrlDownload {
public:
    static BUrlDownload	Download(const BUrl& url,
                                 std::unique_ptr<BDataIO> target = nullptr,
                                 BMessenger observer = BMessenger());
}
```

This can then be used as follows:

```c++
// x86 and x86_64 with C++17 support
auto url = BUrl("https://www.haiku-os.org");
try {
	auto download = BUrlDownload::Download(url);
	auto body = download.Body();
    // if the download was succesful, the resulting body can be used here
} catch (...) {
    // handle errors
}
```

The three arguments are:

* The `url` of the resource to fetch.
* The `target` on where the data should be stored to. If there is no target given, the data will be made available as an `std::string`. 
* The `observer` to which event messages will be sent, in case you want to support an asynchronous workflow. See section C.2 for more info.

The factory function will return an error state when:

* The `url` is invalid.
* The protocol of the URL is not supported.
* Or there was another error in the protocol-specific layers for setting up the request.

Network errors and protocol errors will be made available when calling the `Body()` method.

> The `BUrlDownload` object is in ways similar to the `BHttpResult` in the way it looks and works. There are some differences though. The most obvious difference is that a `BUrlDownload` object contains less information. Where the `BHttpResult` gives you the status and the headers, the download only gives you the final data body. Under the hood, this simplification also means that some of the nuances of the underlying protocol are simplified as well. For example, if you perform a HTTP request that returns a 404 response, you may still get a body (usually a 404 page). With `BHttpResult` you can access the content. However, a `BUrlDownload` will interpret this as a failure, and will not make the content of the 404 response body available. 

 ### C.2 Asynchronous downloads

The `BUrlDownload::Download()` factory function takes an `observer` parameter. This observer will receive the `UrlEvent` messages as detailed in B.6.

> The underlying protocol will transmit the event messages to the observer. Since the `BUrlDownload` interface is a generalized interface, many of the protocol messages will not be useful. It is advised to only handle the general messages defined in B.6. Nonetheless, the underlying protocol will not filter out any protocol-specific events.

### C.3 Accessing the underlying session objects

While the `BUrlDownload` interface is designed to be abstract, it does expose which `B*Session` objects it supports, and allows you to make a copy of the session objects. This will allow you then to add data to them, that will apply to the `BUrlDownload` requests. For example, you might want to add authentication information to `BFtpSession` or add additional valid certificates to `BHttpSession`.

# Other Notes

Other work:

* [cpr - C++ Requests (whoshuu.github.io)](https://whoshuu.github.io/cpr/)
* [Projects/libsoup - GNOME Wiki!](https://wiki.gnome.org/Projects/libsoup)
