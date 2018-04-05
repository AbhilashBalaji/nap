#include "outputcomponent.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>

// Audio includes
#include <audio/service/audioservice.h>
#include "audiocomponentbase.h"

// RTTI
RTTI_BEGIN_CLASS(nap::audio::OutputComponent)
    RTTI_PROPERTY("Input", &nap::audio::OutputComponent::mInput, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Routing", &nap::audio::OutputComponent::mChannelRouting, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::OutputComponentInstance)
    RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
    
    namespace audio
    {
    
        bool audio::OutputComponentInstance::init(utility::ErrorState& errorState)
        {
            OutputComponent* resource = getComponent<OutputComponent>();
            
            auto audioService = getEntityInstance()->getCore()->getService<AudioService>(rtti::ETypeCheck::EXACT_MATCH);
            auto& nodeManager = audioService->getNodeManager();
            
            auto channelCount = resource->mChannelRouting.size();
            if (channelCount > nodeManager.getOutputChannelCount())
            {
                errorState.fail("Trying to rout to output channel that is out of bounds.");
                return false;
            }
            
            for (auto channel = 0; channel < channelCount; ++channel)
            {
                if (resource->mChannelRouting[channel] < 0)
                    continue;
                
                if (resource->mChannelRouting[channel] >= mInput->getChannelCount())
                {
                    errorState.fail("Trying to rout input channel that is out of bounds.");
                    return false;
                }
                
                mOutputs.emplace_back(audioService->makeSafe<OutputNode>(*audioService));
                mOutputs.back()->setOutputChannel(channel);
                mOutputs.back()->audioInput.connect(mInput->getOutputForChannel(resource->mChannelRouting[channel]));
            }
            
            return true;
        }

    }
    
}
