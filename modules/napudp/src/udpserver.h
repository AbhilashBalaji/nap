/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <nap/device.h>
#include <thread>
#include <mutex>

// NAP includes
#include <nap/numeric.h>
#include <concurrentqueue.h>
#include <nap/signalslot.h>

// ASIO includes
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/io_service.hpp>
#include <asio/system_error.hpp>

// Local includes
#include "udpadapter.h"
#include "udppacket.h"

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	/**
	 * The UDPServer connects to an endpoint and receives any UDP packets copyQueuePacket to the endpoint
	 * The server will invoke the packetReceived signal when packets are received
	 * The signal will be fired on the thread this UDPServer is registered to, see UDPThread
	 */
	class NAPAPI UDPServer : public UDPAdapter
	{
		RTTI_ENABLE(UDPAdapter)
	public:
		/**
		 * initialization
		 * @param error contains error information
		 * @return true on succes
		 */
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * called on destruction
		 */
		virtual void onDestroy() override;

		/**
		 * Enqueues a task and makes sure it is excecuted on server thread
		 * @param task the task to be excecuted
		 */
		void enqueueTask(std::function<void()> task);
	public:
		// properties
		int mPort 						= 13251;		///< Property: 'Port' the port the server socket binds to
		std::string mIPRemoteEndpoint 	= "127.0.0.1";  ///< Property: 'Endpoint' the ip adress the server socket binds to
		int mBufferSize 				= 1024;			///< Property: 'BufferSize' the size of the buffer the server writes to
		bool mThrowOnInitError 			= true;			///< Property: 'ThrowOnFailure' when server fails to bind socket, return false on start
	public:
		/**
		 * packet received signal will be dispatched on the thread this UDPServer is registered to, see UDPThread
		 */
		Signal<const UDPPacket&> packetReceived;
	protected:
		/**
		 * The process function
		 */
		void process() override;
	private:
		// ASIO
		asio::io_service 			mIOService;
		asio::ip::udp::socket 		mSocket{mIOService};
		std::vector<nap::uint8>		mBuffer;
		asio::ip::udp::endpoint 	mRemoteEndpoint;
	};
}
