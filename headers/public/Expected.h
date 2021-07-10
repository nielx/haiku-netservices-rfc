/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _EXPECTED_H_
#define _EXPECTED_H_

#include <memory>

// Based on the std::expected proposal as detailed in P0323R10.

template<typename E>
class bad_expected_access;


template<>
class bad_expected_access<void> : public std::exception {
public:
	bad_expected_access() : std::exception() {}
};


template<typename E>
class bad_expected_access : public bad_expected_access<void> {
	E fError;

public:
	bad_expected_access(E error) : fError(error) { }

	virtual char const *what() const noexcept override {
		return "bad_expected_access";
	}

	E& error() & { return fError; }

	E const& error() const& { return fError;}

	E&& error() && { return std::move(fError); }

	E const&& error() const&& { return std::move(fError); }
}; 


template<typename E> class Unexpected;


template<typename T, typename E>
class Expected {
	union { T fResult; E fError; };
	bool fOk = true;
public:
	// Constructors
	Expected() {
		new(&fResult) T();
	}

	Expected(const T& other) {
		new(&fResult) T(other);
	}

	Expected(T&& other) {
		new(&fResult) T(std::move(other));
	}

	Expected(const Expected<T, E>& other) {
		fOk = other.fOk;
		if (fOk)
			new(&fResult) T(other.fResult);
		else
			new(&fError) E(other.fError);
	}

	Expected(Expected<T, E>&& other) {
		fOk = std::move(other.fOk);
		if (fOk)
			new(&fResult) T(std::move(other.fResult));
		else
			new(&fError) E(std::move(other.fError));
	}

	Expected(const Unexpected<E>& other) : fOk(false) {
		new(&fError) E(other.value());
	}

	Expected(Unexpected<E>&& other) : fOk(false) {
		new(&fError) E(std::move(other.value()));
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
		throw bad_expected_access<E>(fError);	
	}
	
	T* operator->() {
		if (fOk) return std::addressof(fResult);
		throw bad_expected_access<E>(fError);
	}

	const T& operator*() const& {
		if (fOk) return fResult;
		throw bad_expected_access<E>(fError);
	}

	T& operator*() & {
		if (fOk) return fResult;
		throw bad_expected_access<E>(fError);
	}

	explicit operator bool() const noexcept {
		return fOk;
	}

	bool has_value() const noexcept {
		return fOk;
	}
	
	const T& value() const& {
		if (fOk) return fResult;
		throw bad_expected_access<E>(fError);
	}

	T& value() & {
		if (fOk) return fResult;
		throw bad_expected_access<E>(fError);
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

