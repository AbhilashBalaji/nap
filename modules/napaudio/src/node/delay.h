#pragma once

#include <utility/audiotypes.h>

namespace nap
{
    namespace audio
    {
        
        /**
         * Utility class representing a single delay that can be written and read from.
         * Supports interpolation between samples while reading.
         */
        class Delay
        {
        public:
            /**
             * The buffer size has to be a power of 2
             */
            Delay(unsigned int bufferSize);
            ~Delay();

            /**
             * Write a sample to the delay line at the current write position
             */
            void write(SampleValue sample);
            
            /**
             * Read a sample from the delay line at @time samples behind the write position.
             * Non interpolating.
             */
            SampleValue read(unsigned int time);
            
            /**
             * Same as @read() but with interpolation between samples
             */
            SampleValue readInterpolating(float sampleTime);
            
            /**
             * Clear the delay line by flushing its buffer.
             */
            void clear();
            
            /**
             * @return: return the maximum delay. (equalling the size of the buffer)
             */
            unsigned int getMaxDelay() { return mBufferSize; }
            
            /**
             * Operator to read from the delay line without interpolation at @index before the write position
             */
            inline SampleValue operator[](unsigned int index)
            {
                return read(index);
            }
            
        private:
            
            SampleValue* mBuffer = nullptr;
            unsigned int mBufferSize = 0;
            unsigned int mWriteIndex = 0;
        };
                
    }
}
