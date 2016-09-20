#pragma once

// Nap Includes
#include <nap/service.h>
#include <nap/configure.h>
#include <napofsplinecomponent.h>
#include <napoftracecomponent.h>
#include <napoftransform.h>

// Include rtti
#include <rtti/rtti.h>

// Local inlcudes
#include <napethercamera.h>

// Naivi OF Includes
#include <ofetherdream.h>

// Std Includes
#include <vector>

namespace nap
{
	/**
	@brief nap etherdream service tied to openframeworks
	**/
	class EtherDreamService : public Service
	{
		// Enable RTTI
		RTTI_ENABLE_DERIVED_FROM(Service);

		// Declare itself as service
		NAP_DECLARE_SERVICE()

	public:
		EtherDreamService();

		// Draws current scene to laser
		void draw();

		// Max points to send over every frame
		uint mMaxLaserPoints = 1000;

		// Flip axis
		bool mFlipX = true;
		bool mFlipY = false;

		void Stop()							{ mEtherdream.Kill(); }

	private:
		//Laser interface
		nofNEtherDream			mEtherdream;				//< Ether-dream threaded service
		std::vector<EAD_Pnt_s>	mLaserPoints;	//< Points that will be send to the laser
		uint					mMinPointCount = 450;
		uint					mMaxPointCount = 600;

		// Returns a set of valid drawable laser components, @return total amount of points to draw
		nap::OFSplineComponent* getRenderableSplineComponent();

		// Returns a valid trace component
		nap::OFTraceComponent*  getRenderableTraceComponent();

		// Populate Laser Buffer
		void populateLaserBuffer(const nap::OFTransform& inLaserTransform, const nap::EtherDreamCamera& inCamera, const std::vector<ofVec3f>& inVerts, const std::vector<ofFloatColor>& inColors, const nap::OFTransform& inObjectTransform);
	};
}

RTTI_DECLARE(nap::EtherDreamService)
