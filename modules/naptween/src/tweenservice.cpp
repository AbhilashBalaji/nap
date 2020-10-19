/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// External Includes
#include <nap/core.h>
#include <nap/logger.h>
#include <iostream>
#include <utility/stringutils.h>

// Local Includes
#include "tweenservice.h"
#include "tween.h"

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::TweenService)
RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
	static std::vector<std::unique_ptr<rtti::IObjectCreator>(*)(TweenService*)>& getObjectCreators()
	{
		static std::vector<std::unique_ptr<rtti::IObjectCreator>(*)(TweenService* service)> vector;
		return vector;
	}


	bool TweenService::registerObjectCreator(std::unique_ptr<rtti::IObjectCreator>(*objectCreator)(TweenService* service))
	{
		getObjectCreators().emplace_back(objectCreator);
		return true;
	}


	TweenService::TweenService(ServiceConfiguration* configuration) :
		Service(configuration)
	{
	}


	TweenService::~TweenService()
	{ }


	void TweenService::registerObjectCreators(rtti::Factory& factory)
	{
		for(auto& objectCreator : getObjectCreators())
		{
			factory.addObjectCreator(objectCreator(this));
		}
	}


	bool TweenService::init(nap::utility::ErrorState& errorState)
	{
		return true;
	}


	void TweenService::update(double deltaTime)
	{
		auto itr = mTweens.begin();
		while (itr!=mTweens.end())
		{
			(*itr)->update(deltaTime);
			++itr;
		}

		for(auto* tween : mTweensToRemove)
		{
			int index = -1;
			for(int i = 0 ; i < mTweens.size(); i++)
			{
				if(mTweens[i].get() == tween)
				{
					index = i;
					break;
				}
			}

			if(index != -1)
			{
				mTweens.erase(mTweens.begin() + index);
			}
		}
		mTweensToRemove.clear();
	}


	void TweenService::removeTween(TweenBase* tween)
	{
		mTweensToRemove.emplace_back(tween);
	}
}