#pragma once

// Local Includes
#include "emographyreading.h"

// External Includes
#include <utility/dllexport.h>
#include <rtti/rtti.h>

namespace nap
{
	namespace emography
	{
		/**
		 * Represents the various stress related stimulation states
		 */
		enum class EStressState : int
		{
			Under	=  0,			///< Under stimulated
			Normal	=  1,			///< Normally stimulated
			Over	=  2,			///< Over stimulated
			Unknown = -1,			///< Unknown stimulated state
		};


		/**
		 * Represents an emography stress related intensity value.
		 * Simple struct like object that has only 1 field but is serializable.
		 * Because the object is relatively light weight it can be both copy and move constructed or assigned on the fly.
		 */
		class NAPAPI StressIntensity final
		{
			RTTI_ENABLE()
		public:

			/**
			 * Default constructor	
			 */
			StressIntensity() = default;

			/**
			* Constructor
			* @param intensity stress intensity value
			*/
			StressIntensity(float intensity) : mValue(intensity)	{ }

			/**
			 * @return if this is a valid intensity reading, ie: intensity value is >= 0
			 */
			inline bool isValid() const					{ return mValue >= 0.0f; }

			float mValue = -1.0f;			///< Property: "Value" the stress related intensity value
		};

		using StressStateReading = Reading<EStressState>;
		using StressStateReadingSummary = ReadingSummary<EStressState>;

		using StressIntensityReading = Reading<StressIntensity>;
		using StressIntensityReadingSummary = ReadingSummary<StressIntensity>;
	}
}
