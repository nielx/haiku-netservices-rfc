/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _EXPECTED_H_
#define _EXPECTED_H_

#include <memory>

// Based on the std::expected proposal as detailed in P0323R10.

template<typename E> class Unexpected;


template<typename T, typename E>
class Expected {
	union { T fResult; E fError; };
	bool fOk = true;
public:
	// Constructors
	Expected() : fResult(T()) { }
	Expected(const T& other) : fResult(T(other)) { }
	Expected(T&& other) : fResult(std::move(other)) { }

	Expected(const Expected<T, E>& other) {
		fOk = other.fOk;
		if (fOk)
			fResult = T(other.fResult);
		else
			fError = E(other.fError);
	}

	Expected(Expected<T, E>&& other) {
		fOk = std::move(fOk);
		if (fOk)
			fResult = T(std::move(other.fResult));
		else
			fError = E(std::move(other.fError));
	}

	Expected(const Unexpected<E>& other) : fOk(false) {
		fError = E(other.value());
	}

	Expected(Unexpected<E>&& other) : fOk(false) {
		fError = E(std::move(other.value()));
	}

	// Destructor
	~Expected() {
		if (fOk)
			fResult.~T();
		else
			fError.~E();
	}

	// Observers (todo: move methods)
	const T* operator->() const {
		if (fOk) return std::addressof(fResult);
		throw fError;	
	}
	
	T* operator->() {
		if (fOk) return std::addressof(fResult);
		throw fError;
	}

	const T& operator*() const& {
		if (fOk) return fResult;
		throw fError;
	}

	T& operator*() & {
		if (fOk) return fResult;
		throw fError;
	}

	explicit operator bool() const noexcept {
		return fOk;
	}

	bool has_value() const noexcept {
		return fOk;
	}
	
	const T& value() const& {
		if (fOk) return fResult;
		throw fError;
	}

	T& value() & {
		if (fOk) return fResult;
		throw fError;
	}

	const E& error() const& {
		if (fOk) throw new std::runtime_error("Expected object does not have an error");
		return fError;
	}

	E& error() & {
		if (fOk) throw new std::runtime_error("Expected object does not have an error");
		return fError;
	}

	const E&& error() const&& {
		if (fOk) throw new std::runtime_error("Expected object does not have an error");
		return std::move(fError);
	}	

	E&& error() && {
		if (fOk) throw new std::runtime_error("Expected object does not have an error");
		return std::move(fError);
	}
};

template<typename E>
class Unexpected {
	E fError;

public:
	Unexpected() = delete;
	
	explicit Unexpected(const E& e) : fError(e) { }
	explicit Unexpected(E &&e) : fError(std::move(e)) { }

	const E& value() const& {
		return fError;
	}

	E& value() & {
		return fError;
	}

	E&& value() && {
		return std::move(fError);
	}
};


#endif

