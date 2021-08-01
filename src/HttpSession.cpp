/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <deque>
#include <iostream>

#include <HttpRequest.h>
#include <HttpSession.h>
#include <Locker.h>

#include "AutoLocker.h"

using namespace BPrivate::Network;


struct BHttpSession::Data {
	// constants (does not need to be locked to be accessed)
	thread_id							controlThread;
	thread_id							dataThread;
	sem_id								controlQueueSem;
	sem_id								dataQueueSem;
	// locking mechanism
	BLocker								lock;
	// control queue
	std::deque<BHttpSession::Wrapper>	controlQueue;
};


struct BHttpSession::Wrapper {
	BHttpRequest	request;
				// Request state/events
	enum {
		kRequestInitialState,
		kRequestStatusReceived,
		kRequestHeadersReceived,
		kRequestContentReceived,
		kRequestTrailingHeadersReceived
	}				requestStatus;
};


BHttpSession::BHttpSession()
	: fData(std::make_shared<Data>())
{
	// set up semaphores for synchronization between data and control thread
	fData->controlQueueSem = create_sem(0, "http:control");
	if (fData->controlQueueSem < 0)
		throw std::runtime_error("Cannot create control queue semaphore");
	fData->dataQueueSem = create_sem(0, "http:data");
	if (fData->dataQueueSem < 0)
		throw std::runtime_error("Cannot create data queue semaphore");
	
	// set up internal threads
	fData->controlThread = spawn_thread(ControlThreadFunc, "http:control", B_NORMAL_PRIORITY, fData.get());
	if (fData->controlThread < 0)
		throw std::runtime_error("Cannot create control thread");
	if (resume_thread(fData->controlThread) != B_OK)
		throw std::runtime_error("Cannot resume control thread");
		
	fData->dataThread = spawn_thread(DataThreadFunc, "http:data", B_NORMAL_PRIORITY, fData.get());
	if (fData->dataThread < 0)
		throw std::runtime_error("Cannot create data thread");
	if (resume_thread(fData->dataThread) != B_OK)
		throw std::runtime_error("Cannot resume data thread");
}


void
BHttpSession::AddRequest(BHttpRequest request)
{
	BHttpSession::Wrapper wRequest{std::move(request), Wrapper::kRequestInitialState};
	AutoLocker<BLocker>(fData->lock);
	fData->controlQueue.push_back(std::move(wRequest));
	release_sem(fData->controlQueueSem);
}


/*static*/ status_t
BHttpSession::ControlThreadFunc(void* arg)
{
	BHttpSession::Data* data = static_cast<BHttpSession::Data*>(arg);
	acquire_sem(data->controlQueueSem);
	std::cout << "New request" << std::endl;
	return B_OK;
}


/*static*/ status_t
BHttpSession::DataThreadFunc(void* arg)
{
	BHttpSession::Data* data = static_cast<BHttpSession::Data*>(arg);
	acquire_sem(data->dataQueueSem);
	return B_OK;
}