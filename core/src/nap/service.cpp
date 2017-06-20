// Local Includes
#include "logger.h"
#include "service.h"
#include "configure.h"
#include "serviceablecomponent.h"

RTTI_DEFINE(nap::Service)

namespace nap
{
	/**
	@brief Destructor
	**/
	Service::~Service()
	{
		if (mIsRunning) stop();
	}


	/**
	@brief Returns the core in which service lives
	**/
	Core& Service::getCore()
	{
		assert(mCore);
		return *mCore;
	}


	/**
	@brief Starts the service
	**/
	void Service::start()
	{
		mIsRunning = true;
		onStart();
	}


	/**
	@brief Stops the service
	**/
	void Service::stop()
	{
		mIsRunning = false;
		onStop();
	}


	/**
	@brief Registers a new component associated with the service
	**/
	void Service::registerObject(Object& object)
	{ 
		// Get or create component using emplace
		auto it = mObjects.emplace(std::make_pair(object.get_type().get_raw_type(), ObjectList()));
		(it.first)->second.emplace_back(&object);

		// Make sure we know if it's removal
		object.removed.connect(mRemoved);
	}


	/**
	@brief De-registers an existing component associated with the service
	**/
	void Service::removeObject(Object& object)
	{
		// Find component container
		rtti::TypeInfo info = object.get_type().get_raw_type();
		std::string name = info.get_name().data();

		auto it = mObjects.find(info);
		if (it == mObjects.end())
			return;

		// Fetch component list associated with type
		ObjectList& ass_obj = (*it).second;

		// Find position
		auto cit = std::find_if(ass_obj.begin(), ass_obj.end(),
			[&](const auto& c_object) { return c_object == &object; });

		// Remove
		if (cit == ass_obj.end()) {
			Logger::warn("Unable to remove object: " + object.getName() + ", not registered with service: " +
						 this->getTypeName());
			return;
		}

		// Remove component at found position
		ass_obj.erase(cit);

		// Let derived services handle removal
		objectRemoved(object);
	}


	// Called just before a registered component is removed
	void Service::removed(nap::Object& object)
	{
		// Remove from list
		removeObject(object);
	}


	/**
	@brief Returns all valid components matching the @inInfo criteria

	Note that this function is fast, it uses the RTTI Map to figure out if components registered with the service are of type @inType
	It returns the total amount of compatible components + a list of components lists vector<vector<Component*>>
	**/
	size_t Service::getTypeFilteredObjects(const rtti::TypeInfo& inInfo, std::vector<ObjectList*>& outObjects)
	{
		// Get raw compare type
		outObjects.clear();
		rtti::TypeInfo search_type = inInfo.get_raw_type();
		if (!search_type.is_derived_from<nap::Object>())
		{
			Logger::warn("not of type: %s", search_type.get_name().data());
			return 0;
		}

		// Figure out size
		size_t size(0);
		for (auto& i : mObjects)
		{
			if (i.first.is_derived_from(search_type))
			{
				size += i.second.size();
				outObjects.emplace_back(&(i.second));
			}
		}
		return size;
	}


	/**
	@brief Get all components associated with RTTI inTypeInfo
	**/
	void Service::getObjects(const rtti::TypeInfo& inTypeInfo, std::vector<Object*> outObjects)
	{
		// Clear
		outObjects.clear();

		// Figure out size
		std::vector<ObjectList*> valid_objects;
		size_t size = getTypeFilteredObjects(inTypeInfo, valid_objects);

		// Return if empty
		if (size == 0) return;

		// Otherwise reserve and insert
		outObjects.reserve(size);
		
		// Add all valid components
		for (auto& c : valid_objects)
		{
			outObjects.insert(std::end(outObjects), std::begin(*c), std::end(*c));
		}
	}
}