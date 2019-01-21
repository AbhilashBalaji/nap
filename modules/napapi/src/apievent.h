#pragma once

// Local  Includes
#include "apiargument.h"
#include "apisignature.h"

// External Includes
#include <nap/event.h>
#include <utility/uniqueptrvectoriterator.h>

namespace nap
{
	using APIArgumentList = std::vector<std::unique_ptr<APIArgument>>;

	/**
	 * API event that is created when calling NAP from an external environment.
	 * Contains a list of API arguments that carry the actual value.
	 * The NAP application decides what to do with these events.
	 * These events are forwarded to the running app by the APIService.
	 */
	class NAPAPI APIEvent : public Event
	{
		RTTI_ENABLE(Event)
	public:
		using ArgumentConstIterator = utility::UniquePtrConstVectorWrapper<APIArgumentList, APIArgument*>;

		/**
		 * Default constructor
		 */
		APIEvent() = default;

		/**
		 * Every API call needs to be associated with an action
		 * @param name identifier of this call
		 */
		APIEvent(const std::string& name);

		/**
		 * Every API call needs to be associated with an action
		 * @param name identifier of this call
		 */
		APIEvent(const std::string&& name);

		/**
		 * @return identifier of this call	
		 */
		const std::string& getID() const							{ return mName; }

		/**
		 * Adds an api argument to this event where T needs to be of type APIValue and args the associated value.
		 * @param args the template arguments used for constructing the argument. In case of an APIFloat the argument could be 1.0f etc.
		 * @return the newly created and added argument
		 */
		template<typename T, typename... Args>
		APIArgument* addArgument(Args&&... args);

		/**
		 * Adds an api argument to this event based on the given api value.
		 * @param value the api value to add as an argument.
		 * @return the added value as api argument.
		 */
		APIArgument* addArgument(std::unique_ptr<APIBaseValue> value);

		/**
		 * @return the number of arguments associated with this event
		 */
		int getCount() const										{ return static_cast<int>(mArguments.size()); }

		/**
		 *	@return the arguments of this osc event
		 */
		const ArgumentConstIterator getArguments() const			{ return ArgumentConstIterator(mArguments); }

		/**
		 * @return an argument based on @index
		 * @param index the index of the argument, will throw an exception when out of bounds
		 */
		const APIArgument* getArgument(int index) const;

		/**
		 * @return an argument based on @index
		 * @param index the index of the argument
		 */
		APIArgument* getArgument(int index);

		/**
		 * If the api arguments and order of arguments matches the given api signature.
		 * @param signature the method signature to validate
		 * @return if this event matches the given api signature.
		 */
		bool matches(const nap::APISignature& signature) const;

		/**
		 * Array [] subscript operator
		 * @return the osc argument at index
		 */
		APIArgument& operator[](std::size_t idx)					{ return *getArgument(static_cast<int>(idx)); }

		/**
		 * Array [] subscript operator
		 * @return the osc argument at index
		 */
		const APIArgument& operator[](std::size_t idx) const		{ return *getArgument(static_cast<int>(idx)); }

	private:
		std::string mName;				///< Name of the action associated with this call
		APIArgumentList mArguments;		// All the arguments associated with the event
	};

	using APIEventPtr = std::unique_ptr<nap::APIEvent>;


	//////////////////////////////////////////////////////////////////////////
	// Template definitions
	//////////////////////////////////////////////////////////////////////////

	template<typename T, typename... Args>
	APIArgument* nap::APIEvent::addArgument(Args&&... args)
	{
		assert(RTTI_OF(T).is_derived_from(RTTI_OF(nap::APIBaseValue)));

		// Create value
		std::unique_ptr<T> value = std::make_unique<T>(std::forward<Args>(args)...);

		// Create argument and move value
		std::unique_ptr<APIArgument> argument = std::make_unique<APIArgument>(std::move(value));

		mArguments.emplace_back(std::move(argument));
		return mArguments.back().get();
	}
}
