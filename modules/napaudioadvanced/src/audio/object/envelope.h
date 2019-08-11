#pragma once

// Nap includes
#include <rtti/objectptr.h>

// Audio includes
#include <audio/core/audioobject.h>
#include <audio/node/envelopenode.h>

namespace nap
{
    
    namespace audio
    {
        
        class EnvelopeInstance;
        
        /**
         * Audio object resource for an envelope generator.
         */
        class Envelope : public AudioObject
        {
            RTTI_ENABLE(AudioObject)
        public:
            Envelope() = default;
            
            EnvelopeGenerator::Envelope mSegments; ///< The segments that define the envelope's shape.
            
            bool mAutoTrigger = false; ///< If true the envelope will be triggered automatically on initialization
            
        private:
            // Inherited from AudioObject
            std::unique_ptr<AudioObjectInstance> createInstance();
        };
        
        
        
        /**
         * Instance of an envelope generator.
         */
        class EnvelopeInstance : public AudioObjectInstance
        {
            RTTI_ENABLE(AudioObjectInstance)
        public:
            EnvelopeInstance(Envelope& resource) : AudioObjectInstance(resource) { }
            
            // Inherited from AudioObjectInstance
            bool init(AudioService& audioService, utility::ErrorState& errorState) override;
            OutputPin& getOutputForChannel(int channel) override { return mEnvelopeGenerator->output; }
            int getChannelCount() const override { return 1; }
            
            /**
             * Triggers the envelope to start playing from the start segment.
             * If totalDuration does not equal zero the relative durations in the segments will be scaled in order to get the total duration of the envelope to match this parameter.
			 * @param totalDuration how to scale the segments to get the total duration
             */
            void trigger(TimeValue totalDuration = 0)
            {
                mEnvelopeGenerator->trigger(mSegments.get(), totalDuration);
            }
            
            /**
             * Stops playing the envelope by fading to zero within the given rampTime.
             */
            void stop(TimeValue rampTime) { mEnvelopeGenerator->stop(rampTime); }
            
            /**
             * Returns the current output value of the envelope generator.
             */
            ControllerValue getValue() const { return mEnvelopeGenerator->getValue(); }

            /**
             * Returns a signal that will be emitted when the total envelope shape has finished and the generator outputs zero again.
             */
            nap::Signal<EnvelopeGenerator&>& getEnvelopeFinishedSignal() { return mEnvelopeGenerator->envelopeFinishedSignal; }
            
        private:
            SafeOwner<EnvelopeGenerator> mEnvelopeGenerator = nullptr;
            SafeOwner<EnvelopeGenerator::Envelope> mSegments = nullptr;
        };
        
    }
    
}
