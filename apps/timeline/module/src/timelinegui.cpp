// local includes
#include "timelinegui.h"
#include "napcolors.h"

// External Includes
#include <entity.h>
#include <imgui/imgui.h>

RTTI_BEGIN_CLASS(nap::TimelineGUI)
RTTI_PROPERTY("Timeline", &nap::TimelineGUI::mTimelineHolder, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	void TimelineGUI::construct()
	{
		//
		auto& timeline = mTimelineHolder->getTimelineRef();

		// push id
		ImGui::PushID(mID.c_str());

		// 100 px per second, default
		float stepSize = 100.0f;

		// calc width of content in timeline window
		float timelineWidth = stepSize * timeline.mDuration + 150.0f;

		// set content width of next window
		ImGui::SetNextWindowContentWidth(timelineWidth);

		// begin window
		if (ImGui::Begin(
			timeline.mName.c_str(), // id
			(bool*)0, // open
			ImGuiWindowFlags_HorizontalScrollbar)) // window flags
		{
			// check if window has focus
			bool windowHasFocus = ImGui::IsWindowFocused();

			//
			if (ImGui::Button("Save"))
			{
				utility::ErrorState errorState;
				timeline.save("test.json", errorState);
			}

			for (const auto& track : timeline.mTracks)
			{
				// push id
				ImGui::PushID(track->mID.c_str());

				// construct dropdown
				int currentItemIndex = std::distance(mParameterIDs.cbegin(), std::find(mParameterIDs.cbegin(), mParameterIDs.cend(), track->mParameterID));
				
				ImGui::PushItemWidth(150.0f);
				if (ImGui::Combo(
					"", // label
					&currentItemIndex, // current item index
					&mParameterNames[0], // pointer to name array
					mParameterNames.size())) // size of name array items
				{
					track->mParameterID = mParameterIDs[currentItemIndex];
				}
				ImGui::PopItemWidth();

				
				ImGui::SameLine();

				//
				const float trackHeight = 100.0f;

				// begin track
				if (ImGui::BeginChild(
					track->mID.c_str(), // id
					ImVec2(timelineWidth, trackHeight), // size
					false)) // no border
				{
					// get child focus
					bool childHasFocus = windowHasFocus && ImGui::IsWindowFocused();

					// get window drawlist
					auto drawList = ImGui::GetWindowDrawList();

					// get current imgui cursor position
					auto cursorPos = ImGui::GetCursorPos();

					// get window position
					auto windowTopLeft = ImGui::GetWindowPos();

					// calc beginning of timeline graphic
					auto timelineTopLeft = ImVec2(windowTopLeft.x + cursorPos.x, windowTopLeft.y + cursorPos.y);

					// draw background of timeline
					drawList->AddRectFilled(
						timelineTopLeft, // top left position
						ImVec2(timelineTopLeft.x + timelineWidth, timelineTopLeft.y + trackHeight), // bottom right position
						guicolors::black); // color 

					// draw keyframes
					for (const auto& keyFrame : track->mKeyFrames)
					{
						// position of keyframe
						float x = keyFrame->mTime * stepSize;

						drawList->AddLine(
							ImVec2(timelineTopLeft.x + x, timelineTopLeft.y), // top left
							ImVec2(timelineTopLeft.x + x, timelineTopLeft.y + trackHeight), // bottom right
							guicolors::white, // color
							1.0f); // thickness
					}
				}

				// end track
				ImGui::EndChild();

				// pop id
				ImGui::PopID();
			}
		}

		// end window
		ImGui::End();

		// pop id
		ImGui::PopID();
	}


	void nap::TimelineGUI::setParameters(const std::vector<rtti::ObjectPtr<ParameterFloat>>& parameters)
	{
		mParameterIDs.clear();
		for (const auto& parameter : parameters)
		{
			mParameterIDs.emplace_back(parameter->mID);
		}

		mParameterNames.clear();
		for (const auto& parameter : mParameterIDs)
		{
			mParameterNames.emplace_back(parameter.c_str());
		}
	}


	std::string TimelineGUI::getName() const
	{
		auto& timeline = mTimelineHolder->getTimelineRef();
		return timeline.mName;
	}


	bool nap::TimelineGUI::init(utility::ErrorState & errorState)
	{
		if (!Resource::init(errorState))
		{
			return false;
		}

		return true;
	}
}
