#include <cassert>
#include <cstdint>

#include <SupportDefs.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#include <Expected.h>

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

int
main(int argc, char** argv) {
	test_expected();
	test_unknown_protocol();
	return 0;
}
