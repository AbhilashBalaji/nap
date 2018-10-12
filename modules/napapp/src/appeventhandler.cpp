// Local Includes
#include "appeventhandler.h"
#include "sdlinputservice.h"

// External includes
#include <sdleventconverter.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::BaseAppEventHandler)
	RTTI_CONSTRUCTOR(nap::BaseApp&)
RTTI_END_CLASS	

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::AppEventHandler)
	RTTI_CONSTRUCTOR(nap::App&)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::GUIAppEventHandler)
	RTTI_CONSTRUCTOR(nap::App&)
RTTI_END_CLASS


namespace nap
{
	BaseAppEventHandler::BaseAppEventHandler(BaseApp& app) : mApp(app)
	{ }


	AppEventHandler::AppEventHandler(App& app) : BaseAppEventHandler(app)
	{ }


	void AppEventHandler::start()
	{
		SDLInputService* input_service = mApp.getCore().getService<nap::SDLInputService>();
		assert(input_service);
		mEventConverter = std::make_unique<SDLEventConverter>(*input_service);
	}


	GUIAppEventHandler::GUIAppEventHandler(App& app) : BaseAppEventHandler(app)
	{ }


	void GUIAppEventHandler::start()
	{
		SDLInputService* input_service = mApp.getCore().getService<nap::SDLInputService>();
		assert(input_service);
		mEventConverter = std::make_unique<SDLEventConverter>(*input_service);
	}


	void AppEventHandler::process()
	{
		opengl::Event event;
		while (opengl::pollEvent(event))
		{
			// Check if we are dealing with an input event (mouse / keyboard)
			if (mEventConverter->isInputEvent(event))
			{
				nap::InputEventPtr input_event = mEventConverter->translateInputEvent(event);
				if (input_event != nullptr)
				{
					getApp<App>().inputMessageReceived(std::move(input_event));
				}
			}

			// Check if we're dealing with a window event
			else if (mEventConverter->isWindowEvent(event))
			{
				nap::WindowEventPtr window_event = mEventConverter->translateWindowEvent(event);
				if (window_event != nullptr)
				{
					getApp<App>().windowMessageReceived(std::move(window_event));
				}
			}

			// Check if we need to quit the app from running
			// -1 signals a quit cancellation
			else if (event.type == SDL_QUIT)
			{
				if (getApp<App>().shutdownRequested())
				{
					getApp<App>().quit();
				}
			}

		}
	}


	void AppEventHandler::shutdown()
	{
		mEventConverter.reset(nullptr);
	}


	void GUIAppEventHandler::process()
	{
		// Poll for events
		opengl::Event event;
		while (opengl::pollEvent(event))
		{
			// Get the interface
			ImGuiIO& io = ImGui::GetIO();
			
			// Process event for imgui
			ImGui_ImplSdlGL3_ProcessEvent(&event);

			// Forward if we're not capturing mouse and it's a pointer event
			if (mEventConverter->isMouseEvent(event))
			{
				if (!io.WantCaptureMouse)
				{
					nap::InputEventPtr input_event = mEventConverter->translateMouseEvent(event);
					if (input_event != nullptr)
					{
						getApp<App>().inputMessageReceived(std::move(input_event));
					}
				}
			}

			// Forward if we're not capturing keyboard and it's a key event
			else if (mEventConverter->isKeyEvent(event))
			{
				if (!io.WantCaptureKeyboard)
				{
					nap::InputEventPtr input_event = mEventConverter->translateKeyEvent(event);
					if (input_event != nullptr)
					{
						getApp<App>().inputMessageReceived(std::move(input_event));
					}
				}
			}

			// Always forward controller events
			else if (mEventConverter->isControllerEvent(event))
			{
				nap::InputEventPtr input_event = mEventConverter->translateControllerEvent(event);
				if (input_event != nullptr)
				{
					getApp<App>().inputMessageReceived(std::move(input_event));
				}
			}

			// Always forward window events
			else if (mEventConverter->isWindowEvent(event))
			{
				nap::WindowEventPtr window_event = mEventConverter->translateWindowEvent(event);
				if (window_event != nullptr)
				{
					getApp<App>().windowMessageReceived(std::move(window_event));
				}
			}
				 
			// Stop if the event tells us to quit
			else if (event.type == SDL_QUIT)
			{
				if (getApp<App>().shutdownRequested())
				{
					getApp<App>().quit();
				}
			}
		}
	}


	void GUIAppEventHandler::shutdown()
	{
		mEventConverter.reset(nullptr);
	}

}