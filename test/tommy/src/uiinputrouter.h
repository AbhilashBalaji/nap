#pragma once

#include "inputrouter.h"
#include "nap/component.h"
#include "nap/entityptr.h"
#include "nap/componentptr.h"
#include "orthocameracomponent.h"

namespace nap
{
	class OrthoCameraComponent;
	class CameraComponentInstance;
	class UIInputRouterComponentInstance;

	/**
	 * Implementation of InputRouter that knows how to route input events to the correct UI element, based on its screen-space position/size
	 */
	class UIInputRouter : public InputRouter
	{
	public:
		/**
		 * Set the camera used to perform depth sorting
		 */
		bool init(CameraComponentInstance& camera, utility::ErrorState& errorState) { mCamera = &camera; return true; }

		/**
		 * Route the specified input event to the correct InputComponent in the specified list of entities
		 *
		 * @param event The input event
		 * @param entities The entities to consider for routing input to 
		 */
		virtual void routeEvent(const InputEvent& event, const EntityList& entities) override;

	private:
		CameraComponentInstance* mCamera;	// The camera used for depth sorting
	};


	/**
	 * This component holds a pointer to the camera entity so that the instance can 
	 * access the camera entity to initialize it's camera.
	 */
	class UIInputRouterComponent : public Component
	{
		RTTI_ENABLE(Component)
		DECLARE_COMPONENT(UIInputRouterComponent, UIInputRouterComponentInstance)

	public:
		ComponentPtr<OrthoCameraComponent> mCameraComponent;		// Pointer to camera entity resource and instance
	};


	/**
	 * Wrapper for a UIInputRouter. Retrieves the camera entity and it's camera component to initialize
	 * the input router.
	 */
	class UIInputRouterComponentInstance : public ComponentInstance
	{
		RTTI_ENABLE(ComponentInstance)

	public:
		/**
		 * Retrieves the camera entity and it's camera component to initialize the input router.
		 * @param resource UIInputRouterComponentResource
		 * @return true on success, other false.
		 */
		virtual bool init(EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState) override;

		UIInputRouterComponentInstance(EntityInstance& entity, Component& resource) :
			ComponentInstance(entity, resource)
		{
		}

		UIInputRouter mInputRouter;		// UI input router
	};
}