#include "audionodemanager.h"
#include "audionode.h"

#include <nap/logger.h>
#include <nap/core.h>

namespace nap
{
    
    namespace audio
    {

        void NodeManager::process(float** inputBuffer, float** outputBuffer, unsigned long framesPerBuffer)
        {
            // clean the output buffers
            for (auto channel = 0; channel < mOutputChannelCount; ++channel)
                memset(outputBuffer[channel], 0, sizeof(float) * framesPerBuffer);
            
            mInputBuffer = inputBuffer;
            
            mInternalBufferOffset = 0;
            while (mInternalBufferOffset < framesPerBuffer)
            {
                for (auto& channelMapping : mOutputMapping)
                    channelMapping.clear();
                
                {
                    for (auto& root : mRootNodes)
                        root->process();
                }
                
                for (auto channel  = 0; channel < mOutputChannelCount; ++channel)
                {
                    for (auto& output : mOutputMapping[channel])
                        for (auto j = 0; j < mInternalBufferSize; ++j)
                            outputBuffer[channel][mInternalBufferOffset + j] += (*output)[j];
                }
                
                mInternalBufferOffset += mInternalBufferSize;
                mSampleTime += mInternalBufferSize;
                
                mTaskQueue.process();
            }
        }
        
        
        void NodeManager::setInputChannelCount(int inputChannelCount)
        {
            mInputChannelCount = inputChannelCount;
        }
        
        
        
        void NodeManager::setOutputChannelCount(int outputChannelCount)
        {
            mOutputChannelCount = outputChannelCount;
            mOutputMapping.clear();
            mOutputMapping.resize(mOutputChannelCount);
        }
        
        
        void NodeManager::setInternalBufferSize(int size)
        {
            mInternalBufferSize = size;
            for (auto& node : mNodes)
                node->setBufferSize(size);
        }
        
        
        void NodeManager::setSampleRate(float sampleRate)
        {
            mSampleRate = sampleRate;
            mSamplesPerMillisecond = sampleRate / 1000.;
            for (auto& node : mNodes)
                node->setSampleRate(sampleRate);
        }
        
        
        void NodeManager::registerNode(Node& node)
        {
            node.setSampleRate(mSampleRate);
            node.setBufferSize(mInternalBufferSize);
            auto oldSampleRate = mSampleRate;
            auto oldBufferSize = mInternalBufferSize;
            enqueueTask([&, oldSampleRate, oldBufferSize](){
                // In the extremely rare case the buffersize or the samplerate of the node manager have been changed in between the enqueueing of the task and its execution on the audio thread, we set them again.
                // However we prefer not to, in order to avoid memory allocation on the audio thread.
                if (oldSampleRate != mSampleRate)
                    node.setSampleRate(mSampleRate);
                if (oldBufferSize != mInternalBufferSize)
                    node.setBufferSize(mInternalBufferSize);
                mNodes.emplace(&node);
            });
        }
        
        
        void NodeManager::unregisterNode(Node& node)
        {
            mNodes.erase(&node);
        }
        
        
        void NodeManager::registerRootNode(Node& rootNode)
        {
            enqueueTask([&](){ mRootNodes.emplace(&rootNode); });
        }

        
        void NodeManager::unregisterRootNode(Node& rootNode)
        {
            mRootNodes.erase(&rootNode);
        }

        
        void NodeManager::provideOutputBufferForChannel(SampleBuffer* buffer, int channel)
        {
            assert(channel < mOutputMapping.size());
            mOutputMapping[channel].emplace_back(buffer);
        }
        
        
    }
    
}

