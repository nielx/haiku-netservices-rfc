#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>

#include <Application.h>
#include <HttpMethod.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <NetServices.h>
#include <SupportDefs.h>
#include <Url.h>

using BPrivate::Network::BHttpMethod;
using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BHttpSession;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::url_request_exception;


// Test synchronous fetching of haiku-os.org
// This function should not throw exceptions
void test_http_get_synchronous(BHttpSession session) {
	auto url = BUrl("https://www.haiku-os.org/");
	auto request = BHttpRequest(url, BHttpMethod::Get());

	auto result = session.AddRequest(std::move(request));
	// get the response
	assert(result.Status().code == 200);
	assert(result.Body().text.size() > 0);
}


// Test asynchronous fetch of haiku-os.org
class AsyncNetTestApp : public BApplication {
public:
	AsyncNetTestApp(BHttpSession session)
		: BApplication("application/x-nettest"), fSession(session)
	{
		auto url = BUrl("https://www.haiku-os.org/");
		auto request = BHttpRequest(url, BHttpMethod::Get());
		fResult = fSession.AddRequest(std::move(request), nullptr, BMessenger(this));
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
			using namespace BPrivate::Network;
			case UrlEvent::RequestCompleted:
			{
				auto id = msg->GetInt32(UrlEventData::Id, -1);
				assert(id == fResult->Identity());
				auto success = msg->GetBool(UrlEventData::Success, false);
				assert(success);
				assert(fResult->HasBody());
				Quit();
				return;
			}
		}
		BApplication::MessageReceived(msg);
	}

private:
	BHttpSession fSession;
	std::optional<BHttpResult> fResult;
};


void
test_http_get_asynchronous(BHttpSession &session)
{
	AsyncNetTestApp app(session);
	app.Run();
}


void
test_http_implicit_cancel(BHttpSession& session)
{
	auto url = BUrl("https://speed.hetzner.de/100MB.bin");
	auto request = BHttpRequest(url, BHttpMethod::Get());

	auto result = session.AddRequest(std::move(request));
	// get the status before we cancel
	assert(result.Status().code == 200);
}


void
test_http_explicit_cancel(BHttpSession& session)
{
	auto url = BUrl("https://speed.hetzner.de/100MB.bin");
	auto request = BHttpRequest(url, BHttpMethod::Get());

	auto result = session.AddRequest(std::move(request));
	// get the status before we cancel
	assert(result.Status().code == 200);
	session.Cancel(result);
	bool exception_caught = false;
	try {
		result.Body();
	} catch (const url_request_exception &e) {
		assert(e.error_type == url_request_exception::Canceled);
		exception_caught = true;
	}
	assert(exception_caught);
}


int
main(int argc, char** argv) {
	auto session = BHttpSession();
	test_http_get_synchronous(session);
	test_http_get_asynchronous(session);
	test_http_implicit_cancel(session);
	test_http_explicit_cancel(session);
	return 0;
}
