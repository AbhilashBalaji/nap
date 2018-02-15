#pragma once

// Nap includes
#include <component.h>
#include <componentptr.h>

// Audio includes
#include <audio/node/outputnode.h>
#include <audio/component/audiocomponentbase.h>

namespace nap
{
    
    namespace audio
    {
    
        class OutputComponentInstance;
        
        
        /**
         * Component that routs output from another audio component to the audio interface.
         */
        class NAPAPI OutputComponent : public Component
        {
            RTTI_ENABLE(nap::Component)
            DECLARE_COMPONENT(OutputComponent, OutputComponentInstance)
            
        public:
            OutputComponent() : nap::Component() { }
            
        public:
            // Properties
            nap::ComponentPtr<AudioComponentBase> mInput; /**<  The component whose audio output to rout to the interface */
            
            /**
             * The size of this vector indicates the number of channels this component outputs.
             * Each element in the array specifies which channel on the audio interface the corresponding input channel will be routed to.
             */
            std::vector<int> mChannelRouting = { 0 };
        };

        
        /**
         * Instance of component that routs output from another audio component to the audio interface
         */
        class NAPAPI OutputComponentInstance : public ComponentInstance
        {
            RTTI_ENABLE(nap::ComponentInstance)
        public:
            OutputComponentInstance(EntityInstance& entity, Component& resource) : nap::ComponentInstance(entity, resource) { }
            
            // Initialize the component
            bool init(utility::ErrorState& errorState) override;
            
        private:
            std::vector<std::unique_ptr<OutputNode>> mOutputs;
            nap::ComponentInstancePtr<AudioComponentBase> mInput = { this, &OutputComponent::mInput };
        };

    }
    
}
