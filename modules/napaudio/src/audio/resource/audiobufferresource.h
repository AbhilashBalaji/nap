#pragma once

// nap includes
#include <nap/resource.h>

// audio includes
#include <audio/utility/audiotypes.h>

namespace nap
{
    
    namespace audio
    {
        
        /**
         * A buffer of multichannel audio data in memory.
         */
        class AudioBufferResource : public Resource
        {
            RTTI_ENABLE(Resource)
        public:
            AudioBufferResource() = default;
            
            /**
             * @return: sample rate at which the audio material in the buffer was sampled.
             */
            float getSampleRate() const { return mSampleRate; }
            
            /**
             * @return: size of the buffer in samples
             */
            DiscreteTimeValue getSize() const { return mBuffer.getSize(); }
            
            /**
             * @return: number of channels in the buffer
             */
            unsigned int getChannelCount() const { return mBuffer.getChannelCount(); }
            
            /**
             * @return: access the actual data in the buffer
             */
            MultiSampleBuffer& getBuffer() { return mBuffer; }
            
        protected:
            /**
             * Sets the sample rate at which the audio material in the buffer was sampled.
             */
            void setSampleRate(float sampleRate) { mSampleRate = sampleRate; }            

        private:
            float mSampleRate = 0;
            MultiSampleBuffer mBuffer;
            
        };
        
        
    }
    
}