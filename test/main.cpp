#include <cassert>
#include <cstdint>

#include <HttpRequest.h>
#include <NetworkRequest.h>
#include <SupportDefs.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#include <Expected.h>

using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BNetworkRequest;
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
void test_http_get_synchronous() {
	auto url = BUrl("https://www.haiku-os.org/");
	assert(url.IsValid());
	auto request = BHttpRequest::Get(url);
	assert(request);
}


int
main(int argc, char** argv) {
	test_expected();
	test_http_get_synchronous();
	return 0;
}
