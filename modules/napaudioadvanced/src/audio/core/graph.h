#pragma once

// Std includes
#include <vector>

// Nap includes
#include <nap/resourceptr.h>
#include <rtti/object.h>
#include <rtti/factory.h>

// Audio includes
#include <audio/core/audioobject.h>


namespace nap
{
    
    namespace audio
    {
        
        using AudioObjectPtr = ResourcePtr<AudioObject>;
        
        
        /**
         * The Graph manages a number of different audio objects that are connected together to represent a DSP network to perform a specific task of mono or multichannel audio processing.
         */
        class NAPAPI Graph : public Resource
        {
            RTTI_ENABLE(Resource)
        public:
            Graph(AudioService& service) : mAudioService(service)  { }

            /**
             * The audio objects managed by the graph that are part of it's DSP network.
             */
            std::vector<AudioObjectPtr> mObjects;
            
            /**
             * Has to point to one of the objects in mObjects.
             * This is normally the "root" of the graph's network and is used to poll output from the graph.
             */
            AudioObjectPtr mOutput;
            
            /**
             * Returns the audio service that instances of this graph will perform their DSP processing on.
             */
            AudioService& getAudioService() { return mAudioService; }
            
        private:
            AudioService& mAudioService;
        };
        
        
        /**
         * Instance of Graph that manages a number of different audio objects, connected together to represent a DSP network to perform a specific task of mono or multichannel audio processing.
         */
        class NAPAPI GraphInstance : rtti::Object
        {
            RTTI_ENABLE()
        public:
            GraphInstance() = default;
            
            bool init(Graph& resource, utility::ErrorState& errorState);
            
            Graph& getResource() { return *mResource; }

            /**
             * @return: the object that outputs the output channels of the graph.
             */
            AudioObjectInstance& getOutput() { return *mOutput; }
            
            /**
             * @return: the object that outputs the output channels of the graph.
             */
            const AudioObjectInstance& getOutput() const { return *mOutput; }
            
            /**
             * @return: an object within this graph by ID.
             */
            AudioObjectInstance* getObject(const std::string& mID);
            
        protected:
            /**
             * @return: all objects within the graph.
             */
            const std::vector<std::unique_ptr<AudioObjectInstance>>& getObjects() const { return mObjects; }

        private:
            std::vector<std::unique_ptr<AudioObjectInstance>> mObjects;
            AudioObjectInstance* mOutput = nullptr;
            Graph* mResource = nullptr;
        };
        
        
        using GraphObjectCreator = rtti::ObjectCreator<Graph, AudioService>;
        
    }
    
}
