/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "computeparticlesapp.h"

// External Includes
#include <nap/core.h>
#include <nap/logger.h>
#include <perspcameracomponent.h>
#include <scene.h>
#include <imgui/imgui.h>
#include <particlevolumecomponent.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ComputeParticlesApp)
	RTTI_CONSTRUCTOR(nap::Core&)
RTTI_END_CLASS

namespace nap 
{
	/**
	* Initialize all the resources and store the objects we need later on
	*/
	bool ComputeParticlesApp::init(utility::ErrorState& error)
	{		
		// Create render service
		mRenderService	= getCore().getService<RenderService>();		
		mInputService	= getCore().getService<InputService>();
		mSceneService	= getCore().getService<SceneService>();
		mGuiService		= getCore().getService<IMGuiService>();

		// Get resource manager and find required resources and entities. 
		mResourceManager = getCore().getResourceManager();		
		ObjectPtr<Scene> scene		= mResourceManager->findObject<Scene>("Scene");
		mRenderWindow				= mResourceManager->findObject<RenderWindow>("Window0");
		mCameraEntity				= scene->findEntity("CameraEntity");
		mDefaultInputRouter			= scene->findEntity("DefaultInputRouterEntity");
		mParticleEntity				= scene->findEntity("ParticleVolumeEntity");

		mNumParticles = mParticleEntity->getComponent<ParticleVolumeComponentInstance>().getNumParticles();
		mGuiService->selectWindow(mRenderWindow);

		return true;
	}
	
	
	/**
	 * Forward all received input events to the input router. 
	 * The input router is used to filter the input events and to forward them
	 * to the input components of a set of entities, in this case our first person camera.
	 *
	 * We also set up our gui that is drawn at a later stage.
	 */
	void ComputeParticlesApp::update(double deltaTime)
	{
		// Update compute instance
		auto& volume = mParticleEntity->getComponent<ParticleVolumeComponentInstance>();

		// Update input
		DefaultInputRouter& input_router = mDefaultInputRouter->getComponent<DefaultInputRouterComponentInstance>().mInputRouter;
		{
			// Update input for first window
			std::vector<nap::EntityInstance*> entities;
			entities.push_back(mCameraEntity.get());

			Window* window = mRenderWindow.get();
			mInputService->processWindowEvents(*window, input_router, entities);
		}

		// Update GUI
		ImGui::Begin("Controls");

		ImGui::Text(getCurrentDateTime().toString().c_str());
		ImGui::TextColored(mGuiService->getPalette().mHighlightColor2, "wasd keys to move, mouse + left mouse button to look");
		ImGui::Text(utility::stringFormat("Framerate: %.02f", getCore().getFramerate()).c_str());
		ImGui::Text(utility::stringFormat("Particles: %d", mNumParticles).c_str());
		ImGui::SliderFloat("Speed", &volume.mTimeScale, 0.0f, 2.0f);
		ImGui::SliderFloat("Displacement", &volume.mDisplacement, 0.0, 2.0f);
		ImGui::SliderFloat("Size", &volume.mParticleSize, 0.0, 1.0f);
		ImGui::SliderFloat("Rotation Speed", &volume.mRotationSpeed, 0.0, 10.0f);
		ImGui::SliderFloat("Rotation Variation", &volume.mRotationVariation, 0.0, 1.0f);

		ImGui::End();
	}
	
	
	/**
	 * Render all objects to screen at once
	 * In this case that's only the particle mesh
	 */
	void ComputeParticlesApp::render()
	{
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording compute commands
		if (mRenderService->beginComputeRecording())
		{
			utility::ErrorState error_state;
			mParticleEntity->getComponent<ParticleVolumeComponentInstance>().compute();

			mRenderService->endComputeRecording();
		}

		// Begin recording the render commands for the main render window
		// This prepares a command buffer and starts a render pass
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Render all available geometry
			PerspCameraComponentInstance& frame_cam = mCameraEntity->getComponent<PerspCameraComponentInstance>();
			mRenderService->renderObjects(*mRenderWindow, frame_cam);

			// Render GUI elements
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Proceed to next frame
		mRenderService->endFrame();
	}
	

	/**
	* Occurs when the event handler receives a window message.
	* You generally give it to the render service which in turn forwards it to the right internal window.
	* On the next update the render service automatically processes all window events.
	* If you want to listen to specific events associated with a window it's best to listen to a window's mWindowEvent signal
	*/
	void ComputeParticlesApp::windowMessageReceived(WindowEventPtr windowEvent)
	{
		mRenderService->addEvent(std::move(windowEvent));
	}
	
	
	/**
	* Called by the app loop. It's best to forward messages to the input service for further processing later on
	* In this case we also check if we need to toggle full-screen or exit the running app
	*/
	void ComputeParticlesApp::inputMessageReceived(InputEventPtr inputEvent)
	{
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			// Escape the loop when esc is pressed
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
			{
				quit();
			}
			// Toggle fullscreen on 'f'
			if (press_event->mKey == nap::EKeyCode::KEY_f)
			{
				mRenderWindow->toggleFullscreen();
			}
		}
		mInputService->addEvent(std::move(inputEvent));
	}
}
