#pragma once

// External Includes
#include <rtti/rtti.h>

// NAP Includes
#include "attribute.h"
#include "operator.h"
#include "signalslot.h"

namespace nap
{
    class Service;
    
    /**
     @brief Serviceable Operator
     
     A specialization of a operator that a service uses -> ie: client of a service
     This operator automatically registers itself with a service after being attached to an entity -> receives a parent
     When destructed the operator de-registers itself from a service
     **/
    
    class ServiceableOperator : public Operator
    {
        RTTI_ENABLE(Operator)
        
    public:
        ServiceableOperator();
        
    protected:
        // Called when the operator has been registered to a service
        virtual void registered()	{ }
        
        // Service to which this component is a client
        Service* mService = nullptr;
        
    private:
        // Slots that handle service registration / deregistration
        Slot<Object&> mAdded = { this, &ServiceableOperator::registerWithService };
        
        // Slot Calls
        void registerWithService(Object& object);
    };
}
