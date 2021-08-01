/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ERRORS_EXT_H
#define _ERRORS_EXT_H

#include <string>

#include <SupportDefs.h>

namespace BPrivate {

class BError
{
public:
	BError()
		: fStatus(B_ERROR), fMessage("General System Error") { }

	BError(status_t status, const std::string& what)
		: fStatus(status), fMessage(what) { }

	BError(status_t status, const char* what)
		: fStatus(status), fMessage(what) { }

	BError(const BError& other)
		: fStatus(other.fStatus), fMessage(other.fMessage) { }

	BError& operator=(const BError& other) {
		if (&other != this) {
			fStatus = other.fStatus;
			fMessage = other.fMessage;
		}
		return *this;
	}

	BError(BError&& other)
		: fStatus(std::move(other.fStatus)), fMessage(std::move(other.fMessage)) { }

	BError& operator=(BError&& other) {
		if (&other != this) {
			fStatus = std::move(other.fStatus);
			fMessage = std::move(other.fMessage);
		}
		return *this;
	}

	status_t Code() const { return fStatus; };

private:
	status_t	fStatus;
	std::string fMessage;
};

} // namespace BPrivate

#endif
