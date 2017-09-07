// Local includes
#include "oscpacketlistener.h"
#include "oscevent.h"
#include "oscreceiver.h"

// External includes
#include <iostream>
#include <nap/logger.h>

namespace nap
{

	OSCPacketListener::OSCPacketListener(OSCReceiver& receiver) : mReceiver(receiver) 
	{	}


	void OSCPacketListener::ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint)
	{
		// Make our event
		OSCEventPtr event = std::make_unique<OSCEvent>(m.AddressPattern());

		// Process argument stream
		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		while (arg != m.ArgumentsEnd())
		{
			if (arg->IsFloat())
			{
				event->addValue<float>((arg++)->AsFloatUnchecked());
				continue;
			}

			if (arg->IsDouble())
			{
				event->addValue<double>((arg++)->AsDoubleUnchecked());
				continue;
			}

			if (arg->IsInt32())
			{
				event->addValue<int>(static_cast<int>((arg++)->AsInt32Unchecked()));
				continue;
			}

			if (arg->IsInt64())
			{
				event->addValue<int>(static_cast<int>((arg++)->AsInt64Unchecked()));
				continue;
			}

			if (arg->IsString())
			{
				event->addValue<std::string>((arg++)->AsStringUnchecked());
				continue;
			}

			if (arg->IsBool())
			{
				event->addValue<bool>((arg++)->AsBoolUnchecked());
				continue;
			}

			if (arg->IsChar())
			{
				event->addValue<char>((arg++)->AsCharUnchecked());
				continue;
			}

			if (arg->IsRgbaColor())
			{
				event->addArgument<OSCColor>((arg++)->AsRgbaColorUnchecked());
				continue;
			}

			if (arg->IsNil())
			{
				event->addArgument<OSCNil>();
				arg++;
				continue;
			}

			if (arg->IsBlob())
			{
				// Get blob data and size
				const void* blob_data = nullptr;
				osc::osc_bundle_element_size_t size;
				(arg++)->AsBlobUnchecked(blob_data, size);

				// Add blob
				event->addArgument<OSCBlob>(blob_data, sizeof(osc::OSC_SIZEOF_INT32) * size);
				continue;
			}

			nap::Logger::info("unknown argument in OSC message: %s", event->mAddress.c_str());
			arg++;
		}

		// Add event to receiver
		mReceiver.addEvent(std::move(event));
	}
}

