/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "tweenapp.h"

// External Includes
#include <nap/core.h>
#include <nap/logger.h>
#include <renderablemeshcomponent.h>
#include <orthocameracomponent.h>
#include <mathutils.h>
#include <scene.h>
#include <perspcameracomponent.h>
#include <inputrouter.h>
#include <imgui/imgui.h>
#include <mathutils.h>
#include <meshutils.h>
#include <uniforminstance.h>
#include <tweenservice.h>
#include <tweenhandle.h>
#include <tween.h>

// Register this application with RTTI, this is required by the AppRunner to
// validate that this object is indeed an application
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::TweenApp)
		RTTI_CONSTRUCTOR(nap::Core&)
RTTI_END_CLASS

const char* ease_types[] =
{
	"LINEAR",
	"CUBIC_IN",
	"CUBIC_INOUT",
	"CUBIC_OUT",
	"BACK_IN",
	"BACK_INOUT",
	"BACK_OUT",
	"BOUNCE_IN",
	"BOUNCE_INOUT",
	"BOUNCE_OUT",
	"CIRC_IN",
	"CIRC_INOUT",
	"CIRC_OUT",
	"ELASTIC_IN",
	"ELASTIC_INOUT",
	"ELASTIC_OUT",
	"EXPO_IN",
	"EXPO_INOUT",
	"EXPO_OUT",
	"QUAD_IN",
	"QUAD_INOUT",
	"QUAD_OUT",
	"QUART_IN",
	"QUART_INOUT",
	"QUART_OUT",
	"QUINT_IN",
	"QUINT_INOUT",
	"QUINT_OUT",
	"SINE_IN",
	"SINE_INOUT",
	"SINE_OUT"
};

const char* tween_modes[] =
{
	"NORMAL",
	"LOOP",
	"PING_PONG",
	"REVERSE"
};

namespace nap
{
	/**
	 * Initialize all the resources and store the objects we need later on
	 */
	bool TweenApp::init(utility::ErrorState& error)
	{
		// Retrieve services
		mRenderService	= getCore().getService<nap::RenderService>();
		mSceneService	= getCore().getService<nap::SceneService>();
		mInputService	= getCore().getService<nap::InputService>();
		mGuiService		= getCore().getService<nap::IMGuiService>();
		mTweenService	= getCore().getService<nap::TweenService>();

		// Get resource manager and load curveball json file
		mResourceManager = getCore().getResourceManager();
		if (!mResourceManager->loadFile("tween.json", error))
			return false;

		// Extract loaded resources
		mRenderWindow = mResourceManager->findObject<nap::RenderWindow>("Window0");

		// Extract the only scene
		ObjectPtr<Scene> scene = mResourceManager->findObject<Scene>("Scene");

		// Find the entities we're interested in
		mCameraEntity = scene->findEntity("Camera");
		mSphereEntity = scene->findEntity("Sphere");
		mPlaneEntity = scene->findEntity("Plane");

		mGuiService->selectWindow(mRenderWindow);

		createTween();

		return true;
	}


	/**
	* Forward all the received input messages to the camera input components.
	* The input router is used to filter the input events and to forward them
	* to the input components of a set of entities, in this case our camera.
	* After that we setup the gui.
	*/
	void TweenApp::update(double deltaTime)
	{
		// The default input router forwards messages to key and mouse input components
		// attached to a set of entities.
		nap::DefaultInputRouter input_router;

		// Forward all input events associated with the first window to the listening components
		std::vector<nap::EntityInstance*> entities = { mCameraEntity.get() };
		mInputService->processWindowEvents(*mRenderWindow, input_router, entities);

		// Setup some gui elements to be drawn later
		ImGui::Begin("Controls");
		ImGui::Text(getCurrentDateTime().toString().c_str());
		RGBAColorFloat clr = mTextHighlightColor.convert<RGBAColorFloat>();
		ImGui::TextColored(clr, "left mouse button to rotate, right mouse button to zoom");
		ImGui::Text(utility::stringFormat("Framerate: %.02f", getCore().getFramerate()).c_str());
		ImGui::End();

		// Find the camera location uniform in the material of the sphere
		nap::RenderableMeshComponentInstance& sphere_mesh = mSphereEntity->getComponent<nap::RenderableMeshComponentInstance>();
		nap::UniformStructInstance* uniform_instance =  sphere_mesh.getMaterialInstance().getOrCreateUniform("UBO");
		UniformVec3Instance* cam_loc_uniform = uniform_instance->getOrCreateUniform<UniformVec3Instance>("inCameraPosition");

		// Set it to the current camera location
		nap::TransformComponentInstance& cam_xform = mCameraEntity->getComponent<nap::TransformComponentInstance>();
		glm::vec3 global_pos = math::extractPosition(cam_xform.getGlobalTransform());
		cam_loc_uniform->setValue(global_pos);

		// Find the animation uniform in the material of the plane.
		nap::RenderableMeshComponentInstance& plane_mesh = mPlaneEntity->getComponent<nap::RenderableMeshComponentInstance>();
		uniform_instance = plane_mesh.getMaterialInstance().getOrCreateUniform("UBO");
		UniformFloatInstance* animator_uniform = uniform_instance->getOrCreateUniform<nap::UniformFloatInstance>("animationValue");

		// draw the GUI for editing tween properties
		if( ImGui::Begin("Tween") )
		{
			// input for tween duration
			if( ImGui::InputFloat("Duration", &mTweenDuration) )
			{
				mTweenDuration = math::max<float>(0.0f, mTweenDuration);
			}

			// change tween target
			if( ImGui::SliderFloat3("Target", &mTarget.x, 0.0f, 1.0f))
			{
			}

			// change tween ease type
			if( ImGui::Combo("Ease", &mCurrentTweenType, ease_types, IM_ARRAYSIZE(ease_types)))
			{
			}

			// change tween mode
			if( ImGui::Combo("Mode", &mCurrentTweenMode, tween_modes, IM_ARRAYSIZE(tween_modes)))
			{
			}

			// press button to create tween
			if( ImGui::Button("Do Tween") )
			{
				createTween();
			}
		}
		ImGui::End();
	}


	void TweenApp::createTween()
	{
		// obtain sphere transform
		auto& sphere_transform = mSphereEntity->getComponent<TransformComponentInstance>();

		// extract world position
		glm::vec3 sphere_position = math::extractPosition(sphere_transform.getGlobalTransform());

		// create tween, app keeps handle in scope
		mActiveTweenHandle = mTweenService->createTween<glm::vec3>(sphere_position, mTarget, mTweenDuration, (ETweenEasing)mCurrentTweenType, (ETweenMode)mCurrentTweenMode);

		// reference to tween
		Tween<glm::vec3>& tween = mActiveTweenHandle->getTween();

		// connect to update signal
		tween.UpdateSignal.connect([this](const glm::vec3& value){
		  	auto& sphere_transform = mSphereEntity->getComponent<TransformComponentInstance>();
			sphere_transform.setTranslate(value);
		});

		// connect to complete signal
		tween.CompleteSignal.connect([this](const glm::vec3& value){
			auto& sphere_transform = mSphereEntity->getComponent<TransformComponentInstance>();
		  	sphere_transform.setTranslate(value);

			nap::Logger::info("tween complete");
		});

		// connect to killed signal
		tween.KilledSignal.connect([this](){
			nap::Logger::info("tween killed");
		});
	}

	/**
	 * Render loop is rather straight forward.
	 * All the objects in the scene are rendered at once including the sphere and plane.
	 * This demo doesn't require special render steps.
	 */
	void TweenApp::render()
	{
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording the render commands for the main render window
		// This prepares a command buffer and starts a render pass
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Start render pass
			mRenderWindow->beginRendering();

			// Render all objects in the scene at once
			// This includes the line + normals and the laser canvas
			mRenderService->renderObjects(*mRenderWindow, mCameraEntity->getComponent<PerspCameraComponentInstance>());

			// Draw gui to screen
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// Stop recording commands
			mRenderService->endRecording();
		}

		mRenderService->endFrame();
	}


	/**
	 * Occurs when the event handler receives a window message.
	 * You generally give it to the render service which in turn forwards it to the right internal window.
	 * On the next update the render service automatically processes all window events.
	 * If you want to listen to specific events associated with a window it's best to listen to a window's mWindowEvent signal
	 */
	void TweenApp::windowMessageReceived(WindowEventPtr windowEvent)
	{
		mRenderService->addEvent(std::move(windowEvent));
	}


	/**
	 * Called by the app loop. It's best to forward messages to the input service for further processing later on
	 * In this case we also check if we need to toggle full-screen or exit the running app
	 */
	void TweenApp::inputMessageReceived(InputEventPtr inputEvent)
	{
		// Escape the loop when esc is pressed
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
				quit();

			// If 'f' is pressed toggle fullscreen
			if (press_event->mKey == nap::EKeyCode::KEY_f)
			{
				mRenderWindow->toggleFullscreen();
			}
		}

		mInputService->addEvent(std::move(inputEvent));
	}


	int TweenApp::shutdown()
	{
		return 0;
	}

}