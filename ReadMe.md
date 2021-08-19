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

### A.3 Errors and Error Handling using `Expected<T, E>` on x86 and x86_64 (C++17)

The proposed API takes the approach that when an object is created, it should be in a valid state. The traditional Haiku API has several classes where one create an object, but is then obliged to use an `InitCheck()` method to make sure that the object is actually in a valid state. The proposed API ditches that pattern on modern compilers, by using static factory class methods to construct new objects. The result of calling those methods is either a valid object, or an error. In order to accommodate that pattern, the `Expected<T, E>` helper type is provided. This class either holds a valid object of type `T`,  or an error of type `E`. The type itself is modeled after the `std::expected<T, E>` proposal that is not yet part of the formal standard (it did not make C++20). See the links at the bottom of this document for more information.

The experimental API introduces the `BError` type, which enriches the standard `status_t` error code with a string error message for further information.

> For now, it is proposed that the GCC2 variant of these libraries will continue with the pattern of providing objects that may be in an invalid state. The InitCheck() for those methods will be made available.

### A.4 Use of modern C++ (C++17)

The Network Services Kit API proposal makes the conscious choice to start implementing modern C++, both in the public API, as well as in the implementation. This means amongst other things, that the following language features will be used:

* Move semantics will be used where possible, reducing the number of objects created using `new` and reducing the risk of memory leaks.
* All publicly classes will have explicit implementations of copy/move constructors and assignment (rule of 5).
* Smart pointers such as `std::unique_ptr` and `std::shared_ptr` are used, even in public interfaces and preferred over existing homegrown helpers like `BReferencable`. 
* Containers from the ISO C++ standard, like `std::vector` will be used, over homegrown implementations.

Because the API will have to be made available to GCC2-based applications as well, the implementation will need to have implement an alternative public API for older compilers. For now, the choice is to have feature parity for the HTTP protocol (because of its use in the package kit and HaikuDepot) and for the general URL download API (see part C) that supports HTTP. Implementation of other protocols for GCC2 will be decided on a per-protocol basis. Additionally, because in the modern implementation has a reliance on move semantics, a choice will have to be made on whether or not the GCC2 implementation will do local objects (which will cause additional copying in cases) or heap allocations. The choice will have to be made on a case by case basis, but memory usage optimization is not the primary focus.

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
//  x86 and x86_64 with C++17 support
auto url = BUrl("https://www.haiku-os.org");
auto request = BHttpRequest::Get(url);
if (!request) {
    std::cout << "Error creating http request: " << request.error().Error() << std::endl;
	return;
}
```

On legacy systems, the process is similar:

```c++
// legacy systems
BUrl url("https://www.haiku-os.org");
BHttpRequest request = BHttpRequest::Get(url);
if (request.InitCheck() != B_OK) {
    std::cout << "Error creating http request: " <<  request.InitCheck() << std::endl;
    return;
}
```

> Note that on modern compilers, the `Expected<>` construct is used to either give you a valid `BHttpRequest`,  or a `BError`  object describing the error. The legacy systems use the traditional Be API construct of returning an object that is in an (internally) invalid state.

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

On legacy systems, the API is similar. The memory management here is similar (see section B.8 for more).

```c++
// legacy systems
class BHttpSession {
    // ...
    BHttpResult AddRequest(BHttpRequest request, BDataIO* target = NULL,
                         BMessenger observer = BMessenger());
    BHttpResult AddRequest(BHttpRequest request, BReference<BMemoryRingIO>,
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
// x86 and x86_64 with C++17 support
auto url = BUrl("http://obviouslyinvalidhost.invalid/");
auto request = HttpRequest::Get(url);
if (!request) {
    // error handling, but this will succeed because the url is of the right protocol
    ...
}
auto result = session.AddRequest(std::move(request));
if (auto body = result.Body(); body) {
    // if the URL would be valid, you could process the body here
} else {
    // this will be called, because the body will be the error state of the Expected<>
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
                    auto body = fResult.Body();
                    if (body)
                        _DisplayHttpResult(body.value());
                   	else
                        _DisplayError(body.error());
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
    static Expected<BUrlDownload, BError>	Download(const BUrl& url,
                                             std::unique_ptr<BDataIO> target = nullptr,
                                             BMessenger observer = BMessenger());
}

// legacy systems
class BUrlDownload {
public:
    static BUrlDownload	Download(const BUrl& url, BDataIO* target = NULL,
                             BMessenger observer = BMessenger());
}
```

This can then be used as follows:

```c++
// x86 and x86_64 with C++17 support
auto url = BUrl("https://www.haiku-os.org");
auto download = BUrlDownload::Download(url);
if (!download) {
    std::cout << "Error creating download: " << download.error().Error() << std::endl;
	return;
}
if (auto body = download.Body(); body) {
    // if the download was succesful, the resulting body can be used here
} else {
    // this will be called, because the body will be the error state of the Expected<>
}
```

On legacy systems, the syntax is as follows:

```c++
// legacy systems
BUrl url("https://www.haiku-os.org");
BUrlDownload download = BUrlDownload::Download(url);
if (download.InitCheck() != B_OK) {
    std::cout << "Error creating download: " <<  download.InitCheck() << std::endl;
    return;
}
BUrlBody* body = download.Body();
if (body) {
    // if the download was succesful, the resulting body can be used here
} else {
    std::cout << "Error downloading url: " << download.ErrorText() << std::endl;
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

On expected:

* [P0323R10: std::expected (open-std.org)](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0323r10.html)
* [TartanLlama/expected: C++11/14/17 std::expected with functional-style extensions (github.com)](https://github.com/TartanLlama/expected)
* [CppCon 2018: Andrei Alexandrescu "Expect the expected" (YouTube)](https://youtu.be/PH4WBuE1BHI)
* [Introduction to proposed std::expected - Niall Douglas - Meeting C++ 2017](https://youtu.be/JfMBLx7qE0I)
* [std-make/include/experimental/fundamental/v3/expected2 at master · viboes/std-make (github.com)](https://github.com/viboes/std-make/tree/master/include/experimental/fundamental/v3/expected2)

Other work:

* [cpr - C++ Requests (whoshuu.github.io)](https://whoshuu.github.io/cpr/)
* [Projects/libsoup - GNOME Wiki!](https://wiki.gnome.org/Projects/libsoup)
