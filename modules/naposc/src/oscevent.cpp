#include "oscevent.h"

// RTTI Definitions
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::OSCEvent)
	RTTI_CONSTRUCTOR(const std::string&)
RTTI_END_CLASS

namespace nap
{

	OSCEvent::OSCEvent(const std::string& address) : mAddress(address)
	{

	}


	nap::OSCArgument* OSCEvent::addString(const std::string& string)
	{
		return addArgument<nap::OSCString>(string);
	}


	const OSCArgument& OSCEvent::getArgument(int index) const
	{
		assert(index < mArguments.size() && index >= 0);
		return *(mArguments[index]);
	}


	nap::OSCArgument& OSCEvent::getArgument(int index)
	{
		assert(index < mArguments.size() && index >= 0);
		return *(mArguments[index]);
	}


	std::size_t OSCEvent::getSize() const
	{
		std::size_t event_size(0);
		for (const auto& arg : mArguments)
		{
			event_size += arg->size();
		}
		return event_size + mAddress.length();
	}
}