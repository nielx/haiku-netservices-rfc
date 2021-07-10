#include <cassert>
#include <cstdint>

#include <SupportDefs.h>

#include <Expected.h>


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
	} catch (status_t &e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}

int
main(int argc, char** argv) {
	test_expected();
	return 0;
}
