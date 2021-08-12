#include <cassert>
#include <cstdint>
#include <iostream>

#include <Application.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <NetServices.h>
#include <SupportDefs.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#include <Expected.h>

using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BHttpSession;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BUrlProtocolRoster;
using BPrivate::Network::BUrlRequest;


void test_expected() {
	Expected<std::int8_t, status_t> result(14);
	assert(result);
	assert(result.has_value() == true);
	assert(*result == 14);

	status_t error = B_NOT_ALLOWED;
	auto failed = Expected<std::int8_t, status_t>(Unexpected<status_t>(error));
	assert(!failed.has_value());
	auto exception_thrown = false;
	try {
		auto x = failed.value();
	} catch (bad_expected_access<status_t> &e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}


// Test synchronous fetching of haiku-os.org
void test_http_get_synchronous(BHttpSession session) {
	auto url = BUrl("https://www.haiku-os.org/");
	assert(url.IsValid());
	auto request = BHttpRequest::Get(url);
	assert(request);

	auto result = session.AddRequest(std::move(request.value()));
	// get the response
	auto status = result.Status();
	if (status)
		assert(status.value().get().code == 200);
	assert(result.Body());
	std::string& body = result.Body().value();
	std::cout << body << std::endl;
}


// Test asynchronous fetch of haiku-os.org
class AsyncNetTestApp : public BApplication {
public:
	AsyncNetTestApp(BHttpSession session)
		: BApplication("application/x-nettest"), fSession(session)
	{
		auto url = BUrl("https://www.haiku-os.org/");
		assert(url.IsValid());
		auto request = BHttpRequest::Get(url);
		assert(request);
		fResult = fSession.AddRequest(std::move(request.value()), BMessenger(this));
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


int
main(int argc, char** argv) {
	test_expected();
	auto session = BHttpSession();
	test_http_get_synchronous(session);
	test_http_get_asynchronous(session);
	return 0;
}
