#pragma once

// Mod nap render includes
#include <renderablemeshcomponent.h>
#include <renderwindow.h>

// Nap includes
#include <nap/resourcemanager.h>
#include <sceneservice.h>
#include <inputservice.h>
#include <inputrouter.h>
#include <app.h>
#include <scene.h>

namespace nap
{
	/**
	 * Main application that is called from within the main loop
	 */
	class TommyApp : public App
	{
		RTTI_ENABLE(App)
	public:
		TommyApp(nap::Core& core) : App(core) { }
		
		/**
		 *	Initialize all the services and app specific data structures
		 */
		bool init(utility::ErrorState& error) override;
		
		/**
		 *	Update is called before render, performs all the app logic
		 */
		void update(double deltaTime) override;

		/**
		 *	Render is called after update, pushes all renderable objects to the GPU
		 */
		void render() override;

		/**
		 *	Called when a window event is received
		 */
		void handleWindowEvent(const WindowEvent& windowEvent);

		/**
		 *	Forwards the received window event to the render service
		 */
		void windowMessageReceived(WindowEventPtr windowEvent) override;
		
		/**
		 *  Forwards the received input event to the input service
		 */
		void inputMessageReceived(InputEventPtr inputEvent) override;
		
		/**
		 *	Toggles full screen
		 */
		void setWindowFullscreen(std::string windowIdentifier, bool fullscreen);
		
		/**
		 *	Called when loop finishes
		 */
		int shutdown() override;
		
		
	private:
		/**
		 *  Cycle-right button clicked
		 */
		void rightButtonClicked(const PointerPressEvent& evt);
		
		/**
		 *  Cycle-left button clicked
		 */
		void leftButtonClicked(const PointerPressEvent& evt);
		
		
		// Nap Services
		RenderService* mRenderService = nullptr;					//< Render Service that handles render calls
		ResourceManager* mResourceManager = nullptr;				//< Manages all the loaded resources
		SceneService* mSceneService = nullptr;						//< Manages all the objects in the scene
		
		InputService* mInputService = nullptr;						//< Input service for processing input
		
		std::vector<rtti::ObjectPtr<RenderWindow>> mRenderWindows;		//< Vector holding pointers to the spawned render windows
		
		rtti::ObjectPtr<EntityInstance> mCameraEntity;					//< The entity that holds the camera
		
		rtti::ObjectPtr<EntityInstance> mRootLayoutEntity;				//< Entity at the root of the layout
		rtti::ObjectPtr<EntityInstance> mSlideShowEntity;					//< The slideshow entity
		rtti::ObjectPtr<EntityInstance> mUiInputRouter;					//< Our UI input router entity

		rtti::ObjectPtr<Scene>		mScene;								//< Scene in json file
	};
}
