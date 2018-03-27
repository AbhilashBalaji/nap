#include "gain.h"

RTTI_BEGIN_CLASS(nap::audio::Gain)
    RTTI_PROPERTY("ChannelCount", &nap::audio::Gain::mChannelCount, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Gain", &nap::audio::Gain::mGain, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Inputs", &nap::audio::Gain::mInputs, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

namespace nap
{
    
    namespace audio
    {
        
        NodePtr<Node> Gain::createNode(int channel, NodeManager& nodeManager)
        {
            auto node = make_node<GainNode>(nodeManager);
            node->setGain(mGain[channel % mGain.size()]);
            for (auto& input : mInputs)
                if (input != nullptr)
                {
                    node->inputs.connect(input->getInstance()->getOutputForChannel(channel % input->getInstance()->getChannelCount()));
                }
            
            return std::move(node);
        }
    }
    
}
