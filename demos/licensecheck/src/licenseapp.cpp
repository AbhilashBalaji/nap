/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "licenseapp.h"

// External Includes
#include <nap/core.h>
#include <nap/logger.h>
#include <renderable2dtextcomponent.h>
#include <inputrouter.h>

// Register this application with RTTI, this is required by the AppRunner to 
// validate that this object is indeed an application
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::LicenseApp)
	RTTI_CONSTRUCTOR(nap::Core&)
RTTI_END_CLASS

 // This is the public key, generated by the 'keygen' tool
inline constexpr char ApplicationPublicKey[] = "30819D300D06092A864886F70D010101050003818B0030818702818100C70E142DA6AEF574D2ECF57117F3380FEFE66CDC627D687915B3F6577BB24FCFF34CE602A13B17FC3439F8663F688C91EF03E36B26D31634141E07BFF9B0C3E34975759DB1F1E940C0ED25C92E65BCC884FDD3DEDF5D76753CE8C6F738683B8523D5D0DF139E04ECFD67A79262BC5DCD77801F6DA595FF129963AC51818D1D51020111";

namespace nap 
{
	/**
	 * Initialize all the resources and store the objects we need later on
	 */
	bool LicenseApp::init(utility::ErrorState& error)
	{		
		// Retrieve services
		mRenderService	= getCore().getService<nap::RenderService>();
		mSceneService	= getCore().getService<nap::SceneService>();
		mInputService	= getCore().getService<nap::InputService>();
		mGuiService		= getCore().getService<nap::IMGuiService>();
		mLicenseService = getCore().getService<nap::LicenseService>();

		// Get resource manager
		mResourceManager = getCore().getResourceManager();

		// License is generated by the 'licensegenerator' tool. Note the signing scheme passed to
		// validateLicense must match the signing scheme passed to licensegenerator during license
		// creation or license validation will fail.
		//
		// If no signing scheme is specified, RSASS_PKCS1v15_SHA1 is used for backward compatibility.
		//   licensegenerator -f John -l Doe -m john@doe.com -a licensedemo -t example -k keys/licensedemo.private -s RSASS_PKCS1v15_SHA512 -o license

		// Validate license using application public key
		if (!mLicenseService->validateLicense(ApplicationPublicKey, nap::ESigningScheme::RSASS_PKCS1v15_SHA512, mLicenseInfo, mLicenseError))
			mLicenseValid = false;

		// Get machine identifier (Linux, Win32), Apple not supported
#ifndef __APPLE__
		if (!mLicenseService->getMachineID(mMachineID, error))
			return false;
#endif // !__APPLE__

		// Extract loaded resources
		mRenderWindow = mResourceManager->findObject<nap::RenderWindow>("Window0");

		// Get the resource that manages all the entities
		mScene = mResourceManager->findObject<Scene>("Scene");

		// Fetch text
		mTextEntity = mScene->findEntity("Text");

		return true;
	}
	
	
	/**
	 * Show a modal dialog when there is no valid license.
	 * The only thing to do from there is quit the application.
	 * Otherwise display license information
	 */
	void LicenseApp::update(double deltaTime)
	{	
		// Forward all input events associated with the first window to all listening components
		nap::DefaultInputRouter input_router;
		std::vector<nap::EntityInstance*> entities = { &mScene->getRootEntity() };
		mInputService->processWindowEvents(*mRenderWindow, input_router, entities);

		// Show modal popup when there's no valid license
		// Notifies user and quits application
		if (!mLicenseValid)
			ImGui::OpenPopup("License Error");
		handleLicensePopup();

		// Add some GUI elements
		ImGui::Begin("Information");
		ImGui::Text(getCurrentDateTime().toString().c_str());
		if (mLicenseValid) {
			ImGui::TextColored(mGuiService->getPalette().mHighlightColor2, "License: valid");
		}
		else {
			ImGui::TextColored(mGuiService->getPalette().mHighlightColor1, "License: invalid");
		}
		ImGui::TextColored(mGuiService->getPalette().mHighlightColor3, utility::stringFormat("Machine ID: %llu", mMachineID).c_str());
		ImGui::Text(utility::stringFormat("Framerate: %.02f", getCore().getFramerate()).c_str());
		if (ImGui::CollapsingHeader("License"))
		{
			ImGui::Text(utility::stringFormat("App: %s", mLicenseInfo.mApp.c_str()).c_str());
			ImGui::Text(utility::stringFormat("Name: %s", mLicenseInfo.mName.c_str()).c_str());
			ImGui::Text(utility::stringFormat("Mail: %s", mLicenseInfo.mMail.c_str()).c_str());
			ImGui::Text(utility::stringFormat("Tag: %s", mLicenseInfo.mTag.c_str()).c_str());
			ImGui::Text(utility::stringFormat("Expires: %s", mLicenseInfo.mExpires ? mLicenseInfo.mTime.toString().c_str() : "Never").c_str());
		}
		ImGui::End();

		// Update text based on license validity
		utility::ErrorState text_error;
		Renderable2DTextComponentInstance& render_text = mTextEntity->getComponent<nap::Renderable2DTextComponentInstance>();
		if (!render_text.setText(mLicenseValid ? "License is valid: application unlocked" : 
			"License is invalid: application locked", text_error))
		{
			nap::Logger::warn(text_error.toString());
		}
	}

	
	/**
	 * Renders text to screen
	 */
	void LicenseApp::render()
	{
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording the render commands for the main render window
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin the render pass
			mRenderWindow->beginRendering();

			// Locate component that can render text to screen
			Renderable2DTextComponentInstance& render_text = mTextEntity->getComponent<nap::Renderable2DTextComponentInstance>();

			// Center text and render it using the given draw call, 
			render_text.setLocation({ mRenderWindow->getWidthPixels() / 2, mRenderWindow->getHeightPixels() / 2 });
			render_text.draw(*mRenderWindow);

			// Draw our GUI
			mGuiService->draw();
			
			// End the render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Signal the ending of the frame
		mRenderService->endFrame();
	}
	
	
	/**
	 * Occurs when the event handler receives a window message.
	 * You generally give it to the render service which in turn forwards it to the right internal window. 
	 * On the next update the render service automatically processes all window events. 
	 * If you want to listen to specific events associated with a window it's best to listen to a window's mWindowEvent signal
	 */
	void LicenseApp::windowMessageReceived(WindowEventPtr windowEvent)
	{
		mRenderService->addEvent(std::move(windowEvent));
	}


	/**
	 * Called by the app loop. It's best to forward messages to the input service for further processing later on
	 * In this case we also check if we need to toggle full-screen or exit the running app
	 */
	void LicenseApp::inputMessageReceived(InputEventPtr inputEvent)
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


	int LicenseApp::shutdown()
	{
		return 0;
	}


	void LicenseApp::handleLicensePopup()
	{
		if (ImGui::BeginPopupModal("License Error"))
		{
			ImGui::Text(mLicenseError.toString().c_str());
			if (ImGui::Button("Quit"))
			{
				this->quit();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}
