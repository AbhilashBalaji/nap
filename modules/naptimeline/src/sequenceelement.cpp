#include "sequenceelement.h"

#include <rtti/rttiutilities.h>
#include <rtti/jsonreader.h>
#include <rtti/defaultlinkresolver.h>
#include <rtti/jsonwriter.h>
#include <rtti/epropertyvalidationmode.h>
#include <utility/fileutils.h>
#include <fstream>


RTTI_DEFINE_BASE(nap::timeline::SequenceElement)

RTTI_BEGIN_CLASS(nap::timeline::SequenceElement)
// Put additional properties here

RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

namespace nap
{
	namespace timeline
	{
		SequenceElement::~SequenceElement() { }


		bool SequenceElement::init(utility::ErrorState& errorState)
		{
			if (!errorState.check(mDuration >= 0.0f,
				"duration must be bigger or equal then 0 %s", this->mID.c_str()))
				return false;

			if (mUsePreset)
			{
				rtti::Factory factory;
				rtti::DeserializeResult result;

				bool success = rtti::readJSONFile("presets/inputs/" + std::string(mPreset), rtti::EPropertyValidationMode::DisallowMissingProperties, factory, result, errorState);
				if (!errorState.check(success,
					"error loading preset %s", this->mID.c_str()))
					return false;

				// Resolve links
				if (!rtti::DefaultLinkResolver::sResolveLinks(result.mReadObjects, result.mUnresolvedPointers, errorState))
					return false;

				//
				if (!errorState.check(result.mReadObjects.size() > 0,
					"empty preset %s", this->mID.c_str()))
					return false;

				// first object must be parametergroup
				std::unique_ptr<rtti::Object>& object = result.mReadObjects[0];

				// 
				if (!errorState.check(object->get_type().is_derived_from<ParameterGroup>(),
					"object must be derived from ParameterGroup %s", this->mID.c_str()))
					return false;

				// get group
				ParameterGroup* group = rtti_cast<ParameterGroup>(object.get());
				mObject = std::move(object);

				mEndParameters.clear();
				for (int i = 0; i < group->mParameters.size(); i++)
				{
					mEndParameters.push_back(std::move(group->mParameters[i]));
				}
			}
			else
			{
			}

			return true;
		}

		bool SequenceElement::process(double time, std::vector<ResourcePtr<Parameter>>& outParameters)
		{
			return (time >= mStartTime && time < mStartTime + mDuration);
		}

		void SequenceElement::setStartParameters(const std::vector<ResourcePtr<Parameter>>& startParameters)
		{
			mStartParameters = startParameters;
		}
	}
}