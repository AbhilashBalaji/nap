#pragma once

#include <nap/resourceptr.h>
#include <component.h>
#include <parameternumeric.h>

#include "sequence.h"

namespace nap
{
	namespace timeline
	{
		class SequencePlayerComponentInstance;

		/**
		*	TimelineSequencePlayerComponent
		*/
		class NAPAPI SequencePlayerComponent : public Component
		{
			RTTI_ENABLE(Component)
				DECLARE_COMPONENT(SequencePlayerComponent, SequencePlayerComponentInstance)
		public:

			/**
			* Get a list of all component types that this component is dependent on (i.e. must be initialized before this one)
			* @param components the components this object depends on
			*/
			virtual void getDependentComponents(std::vector<rtti::TypeInfo>& components) const override;

			ResourcePtr<ParameterGroup> mParameterGroup;
		};


		/**
		* TimelineSequencePlayerComponentInstance
		*/
		class NAPAPI SequencePlayerComponentInstance : public ComponentInstance
		{
			RTTI_ENABLE(ComponentInstance)
		public:
			SequencePlayerComponentInstance(EntityInstance& entity, Component& resource) :
				ComponentInstance(entity, resource) { }

			/**
			* Initialize TimelineSequencePlayerComponentInstance based on the TimelineSequencePlayerComponent resource
			* @param entityCreationParams when dynamically creating entities on initialization, add them to this this list.
			* @param errorState should hold the error message when initialization fails
			* @return if the flexblocksequenceplayerInstance is initialized successfully
			*/
			virtual bool init(utility::ErrorState& errorState) override;

			/**
			* update flexblocksequenceplayerInstance. This is called by NAP core automatically
			* @param deltaTime time in between frames in seconds
			*/
			virtual void update(double deltaTime) override;

			/**
			* load a sequence
			* @param sequence, pointer to a sequence
			* @param error
			* @return returns true if succesfully loaded
			*/
			bool load(ResourcePtr<Sequence> sequence, utility::ErrorState& error);

			/**
			* play a sequence
			*/
			void play();

			/**
			* stop a sequence, unloads the sequence
			*/
			void stop();

			/**
			* pause the sequence
			*/
			void pause();

			/**
			* plays sequence from beginning when finished
			* @param value, true or false
			*/
			void setIsLooping(bool isLooping) { mIsLooping = isLooping; }

			/**
			* sets the current time of the sequence
			* @param time, time to set sequence to
			*/
			void setTime(double time);

			/**
			* @return true if loaded sequence
			*/
			const bool getIsLoaded() { return mSequence != nullptr; }

			/**
			* @return true if playing
			*/
			const bool getIsPlaying() { return mIsPlaying; }

			/**
			* @return true if paused
			*/
			const bool getIsPaused() { return mIsPaused; };

			/**
			* @return true if finished playing
			*/
			const bool getIsFinished() { return mIsFinished; }

			/**
			* @return pointer to current sequence
			*/
			const ResourcePtr<Sequence>& getCurrentSequence() { return mSequence; };

			/**
			* @return pointer to current element in sequence being played, nullptr if not available
			*/
			const ResourcePtr<SequenceElement>& getCurrentElement()
			{
				if (mSequence->mElements.size() > 0)
					return mSequence->mElements[mCurrentSequenceIndex];

				return nullptr;
			};

			/**
			* @return current time in sequence
			*/
			const double getCurrentTime() { return mTime; }

			/**
			* @return duration of sequence
			*/
			const double getDuration() { return mDuration; }

			/**
			* @return true if looping
			*/
			const bool getIsLooping() { return mIsLooping; }

			/**
			* @return vector of pointers to sequence elements
			*/
			const std::vector<ResourcePtr<SequenceElement>>& getElements();
		protected:
			//
			ResourcePtr<Sequence> mSequence = nullptr;

			double mTime = 0.0;
			bool mIsPlaying = false;
			bool mIsPaused = false;
			bool mIsFinished = false;
			bool mIsLooping = false;
			int mCurrentSequenceIndex = 0;
			double mDuration = 0.0;

			std::vector<ResourcePtr<Parameter>> mParameters = std::vector<ResourcePtr<Parameter>>();
		};
	}
}
