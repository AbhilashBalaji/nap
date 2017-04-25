#pragma once

// Local Includes
#include <nap/coreattributes.h>
#include "attributeobject.h"

// External Includes
#include <string>

namespace nap
{
	// Forward Declares
	class ResourceManagerService;
	class Core;

	struct InitResult
	{
		std::string mErrorString;

		bool check(bool successCondition, const std::string& errorString)
		{
			if (!successCondition)
				mErrorString = errorString;

			return successCondition;
		}

		template <typename... Args>
		bool check(bool successCondition, const std::string& format, Args&&... args)
		{
			return check(successCondition, stringFormat(format, std::forward<Args>(args)...));
		}
	};

	/**
	* Abstract base class for any Asset. Could be a TextureAsset, ModelAsset or AudioAsset for example.
	* WARNING: A resource may only be created through the ResourceManagerService providing a valid resource path
	*/
	class Resource : public AttributeObject
	{
		RTTI_ENABLE_DERIVED_FROM(AttributeObject)
	public:
		enum class EFinishMode : uint8
		{
			COMMIT,
			ROLLBACK
		};

		Resource() = default;

		/**
		* 
		*/
		virtual bool init(InitResult& initResult) = 0;

		/**
		* 
		*/
		virtual void finish(EFinishMode mode) = 0;

		/**
		* @return Human readable string representation of this path
		*/
		virtual const std::string getDisplayName() const = 0;
	};


}

RTTI_DECLARE_BASE(nap::Resource)
