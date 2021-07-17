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


// Test whether we are correctly getting a B_NOT_SUPPORTED error when calling
// BUrlProtocolRoster::MakeRequest<...>(...) with an unsupported protocol.
void test_unknown_protocol() {
	auto url = BUrl("httpx://unknown.protocol.com/");
	assert(url.IsValid());
	auto request = BUrlProtocolRoster::MakeRequest<BUrlRequest>(url);
	assert(!request.has_value());
	assert(request.error().Code() == B_NOT_SUPPORTED);
}


// Test whether we can effectively get a BNetworkRequest and a BHttpRequest
// when passing in a HTTPS URL
void test_request_type() {
	auto url = BUrl("https://www.haiku-os.org/");
	assert(url.IsValid());
	auto networkRequest =
		BUrlProtocolRoster::MakeRequest<BNetworkRequest>(url);
	assert(networkRequest.has_value());
	auto httpRequest =
		BUrlProtocolRoster::MakeRequest<BHttpRequest>(url);
	assert(httpRequest.has_value());
}


int
main(int argc, char** argv) {
	test_expected();
	test_unknown_protocol();
	test_request_type();
	return 0;
}
