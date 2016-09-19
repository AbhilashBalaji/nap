#pragma once

// NAP Includes
#include <nap/coremodule.h>
#include <nap/component.h>
#include <nap/attribute.h>

#include <napofattributes.h>

// OF Includes
#include <ofFbo.h>
#include <Utils/ofVec2i.h>

namespace nap
{
	// Forward declares
	class OFService;

	class OFFrameBufferComponent : public Component
	{
		friend class OFService;

		RTTI_ENABLE_DERIVED_FROM(Component)
	public:
		// Default constructor
		OFFrameBufferComponent();

		// Attributes
		Attribute<ofVec2i> Resolution =			{ this, "Resolution", { 1024, 1024 } };

		// Utility
		void setSettings(const ofFbo::Settings& inSettings);
		ofFbo::Settings getSettings() const		{ return mSettings; }
		const ofTexture& getTexture() const		{ return mFrameBuffer.getTexture(); }
		ofTexture& getTextureRef()				{ return mFrameBuffer.getTextureReference(); }
		bool isAllocated () const				{ return mFrameBuffer.isAllocated(); }

		// Slots
		NSLOT(mResolutionChanged, const ofVec2i&, resolutionChanged)

	protected:
		// Only accessible by the OFService or derived classes
		void begin();
		void end()								{ mFrameBuffer.end(); }

	private:
		// Members
		ofFbo mFrameBuffer;
		ofFbo::Settings	mSettings;

		// Init of buffers
		void resolutionChanged(const ofVec2i& inValue);

		// Allocates the buffer
		void allocateBuffer();

		// Inits the buffer using default settings
		void initBuffer();
	};
}

RTTI_DECLARE(nap::OFFrameBufferComponent)
