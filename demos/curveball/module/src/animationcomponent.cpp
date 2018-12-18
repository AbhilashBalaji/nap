#include "animationcomponent.h"

#include <entity.h>
#include <transformcomponent.h>
#include <rect.h>
#include <glm/gtx/transform.hpp>
#include <mathutils.h>

RTTI_BEGIN_CLASS(nap::AnimatorComponent)
	RTTI_PROPERTY("Curve", &nap::AnimatorComponent::mCurve, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::AnimatorComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&) 
RTTI_END_CLASS

namespace nap
{
	AnimatorComponentInstance::AnimatorComponentInstance(EntityInstance& entity, Component& resource) :
		ComponentInstance(entity, resource)
	{ }


	bool AnimatorComponentInstance::init(utility::ErrorState& errorState)
	{
		// Fetch transform component 
		mTransform = getEntityInstance()->findComponent<TransformComponentInstance>();

		// Fetch curve resource we want to evaluate every frame
		mCurve = getComponent<AnimatorComponent>()->mCurve.get();

		return true;
	}


	void AnimatorComponentInstance::update(double deltaTime)
	{
		// Evaluate curve at current time
		mCurveValue = mCurve->evaluate(mLocalTime);

		// Set as transform
		mTransform->setTranslate({0.0f, mCurveValue, 0.0f});

		// Update local time
		mLocalTime = fmod(mLocalTime + deltaTime, 1);
	}
}