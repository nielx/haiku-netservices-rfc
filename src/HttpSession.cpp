/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <HttpSession.h>
#include <Locker.h>

using namespace BPrivate::Network;


struct BHttpSession::Data {
	BLocker		lock;
};


BHttpSession::BHttpSession()
	: fData(std::make_shared<Data>())
{

}
