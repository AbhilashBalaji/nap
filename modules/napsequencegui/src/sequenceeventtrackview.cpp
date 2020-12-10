/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "sequenceeventtrackview.h"
#include "napcolors.h"
#include "sequencecontrollerevent.h"
#include "sequenceeditorgui.h"
#include "sequenceplayereventoutput.h"
#include "sequencetrackevent.h"

#include <nap/logger.h>
#include <iostream>

namespace nap
{
	using namespace SequenceGUIActions;
	using namespace SequenceGUIClipboards;

	std::unordered_map<rttr::type, std::unique_ptr<SequenceEventTrackSegmentViewBase>>& SequenceEventTrackView::getSegmentViews()
	{
		static std::unordered_map<rttr::type, std::unique_ptr<SequenceEventTrackSegmentViewBase>> segment_views;
		return segment_views;
	}

	std::unordered_map<rttr::type, bool (SequenceEventTrackView::*)()>& SequenceEventTrackView::getEditEventHandlers()
	{
		static std::unordered_map<rttr::type, bool(SequenceEventTrackView::*)()> popup_handlers;
		return popup_handlers;
	}

	std::unordered_map<rttr::type, void (SequenceEventTrackView::*)(const std::string&, double)>& SequenceEventTrackView::getPasteEventMap()
	{
		static std::unordered_map<rttr::type, void(SequenceEventTrackView::*)(const std::string&, double)> paste_handlers;
		return paste_handlers;
	}

	static bool track_view_registration = SequenceEditorGUIView::registerTrackViewType(RTTI_OF(SequenceTrackEvent), RTTI_OF(SequenceEventTrackView));

	static bool register_track_view_factory = SequenceTrackView::registerFactory(RTTI_OF(SequenceEventTrackView), [](SequenceEditorGUIView& view, SequenceEditorGUIState& state)->std::unique_ptr<SequenceTrackView>
	{
		return std::make_unique<SequenceEventTrackView>(view, state);
	});

	std::vector<rttr::type>& SequenceEventTrackView::getEventTypesVector()
	{
		static std::vector<rttr::type> eventTypes;
		return eventTypes;
	}


	SequenceEventTrackView::SequenceEventTrackView(SequenceEditorGUIView& view, SequenceEditorGUIState& state)
		: SequenceTrackView(view, state)
	{
	}


	void SequenceEventTrackView::showInspectorContent(const SequenceTrack& track)
	{
		// draw the assigned receiver
		ImGui::Text("Assigned Output");

		ImVec2 inspector_cursor_pos = ImGui::GetCursorPos();
		inspector_cursor_pos.x += 5;
		inspector_cursor_pos.y += 5;
		ImGui::SetCursorPos(inspector_cursor_pos);

		bool assigned = false;
		std::string assigned_id;
		std::vector<std::string> event_outputs;
		int current_item = 0;
		event_outputs.emplace_back("none");
		int count = 0;
		const SequencePlayerEventOutput* event_output = nullptr;

		for (const auto& output : getEditor().mSequencePlayer->mOutputs)
		{
			if (output.get()->get_type() == RTTI_OF(SequencePlayerEventOutput))
			{
				count++;

				if (output->mID == track.mAssignedOutputID)
				{
					assigned = true;
					assigned_id = output->mID;
					current_item = count;

					assert(output.get()->get_type() == RTTI_OF(SequencePlayerEventOutput)); // type mismatch
					event_output = static_cast<SequencePlayerEventOutput*>(output.get());
				}

				event_outputs.emplace_back(output->mID);
			}
		}

		ImGui::PushItemWidth(200.0f);
		if (Combo(
			"",
			&current_item, event_outputs))
		{
			auto& event_controller = getEditor().getController<SequenceControllerEvent>();

			if (current_item != 0)
				event_controller.assignNewObjectID(track.mID, event_outputs[current_item]);
			else
				event_controller.assignNewObjectID(track.mID, "");

		}
		ImGui::PopItemWidth();
	}


	void SequenceEventTrackView::showTrackContent(const SequenceTrack& track, const ImVec2& trackTopLeft)
	{
		ImDrawList * draw_list = ImGui::GetWindowDrawList();

		if (mState.mIsWindowFocused)
		{
			// handle insertion of segment
			if (mState.mAction->isAction<None>())
			{
				if (ImGui::IsMouseHoveringRect(
					trackTopLeft, // top left position
					{ trackTopLeft.x + mState.mTimelineWidth, trackTopLeft.y + mState.mTrackHeight }))
				{
					// position of mouse in track
					draw_list->AddLine(
						{ mState.mMousePos.x, trackTopLeft.y }, // top left
						{ mState.mMousePos.x, trackTopLeft.y + mState.mTrackHeight }, // bottom right
						guicolors::lightGrey, // color
						1.0f); // thickness

					ImGui::BeginTooltip();
					ImGui::Text(formatTimeString(mState.mMouseCursorTime).c_str());
					ImGui::EndTooltip();

					// right mouse down
					if (ImGui::IsMouseClicked(1))
					{
						double time = mState.mMouseCursorTime;

						//
						mState.mAction = createAction<OpenInsertEventSegmentPopup>(
							track.mID,
							time);
					}
				}
			}

			// draw line in track while in inserting segment popup
			if (mState.mAction->isAction<OpenInsertEventSegmentPopup>())
			{
				auto* action = mState.mAction->getDerived<OpenInsertEventSegmentPopup>();
				if (action->mTrackID == track.mID)
				{
					// position of insertion in track
					draw_list->AddLine(
						{ trackTopLeft.x + (float)action->mTime * mState.mStepSize, trackTopLeft.y }, // top left
						{ trackTopLeft.x + (float)action->mTime * mState.mStepSize, trackTopLeft.y + mState.mTrackHeight }, // bottom right
						guicolors::lightGrey, // color
						1.0f); // thickness
				}
			}

			if ( mState.mAction->isAction<InsertingEventSegment>())
			{
				auto* action = mState.mAction->getDerived<InsertingEventSegment>();
				if (action->mTrackID == track.mID)
				{
					// position of insertion in track
					draw_list->AddLine(
						{ trackTopLeft.x + (float)action->mTime * mState.mStepSize, trackTopLeft.y }, // top left
						{ trackTopLeft.x + (float)action->mTime * mState.mStepSize, trackTopLeft.y + mState.mTrackHeight }, // bottom right
						guicolors::lightGrey, // color
						1.0f); // thickness
				}
			}
		}

		float prev_segment_x = 0.0f;

		int segment_count = 0;
		for (const auto& segment : track.mSegments)
		{
			float segment_x	   = (segment->mStartTime) * mState.mStepSize;

			// draw segment handlers
			drawSegmentHandler(
				track,
				*segment.get(),
				trackTopLeft, segment_x,
				0.0f, draw_list);

			// static map of draw functions for different event types


			//
			auto type = segment.get()->get_type();
			auto& segment_views = getSegmentViews();
			auto it = segment_views.find(type);
			assert(it != segment_views.end()); // type not found
			if( it != segment_views.end())
			{
				it->second->drawEvent(*segment.get(), draw_list, trackTopLeft, segment_x);
			}

			//
			prev_segment_x = segment_x;

			//
			segment_count++;
		}
	}


	bool SequenceEventTrackView::handlePopups()
	{
		if( handleInsertEventSegmentPopup() )
			return true;

		if( handleDeleteSegmentPopup() )
			return true;

		for(auto& type : getEventTypesVector())
		{
			auto& edit_popup_map = getEditEventHandlers();
			auto it = edit_popup_map.find(type);
			assert(it != edit_popup_map.end()); // type not found
			if( it != edit_popup_map.end())
			{
				if( (*this.*it->second)() )
				{
					return true;
				}
			}
		}

		return false;
	}


	bool SequenceEventTrackView::handleInsertEventSegmentPopup()
	{
		// popup handled
		bool handled = false;

		if (mState.mAction->isAction<OpenInsertEventSegmentPopup>())
		{
			// invoke insert sequence popup
			ImGui::OpenPopup("Insert Event");

			auto* action = mState.mAction->getDerived<OpenInsertEventSegmentPopup>();
			mState.mAction = createAction<InsertingEventSegment>(action->mTrackID, action->mTime);
		}

		// handle insert segment popup
		if (mState.mAction->isAction<InsertingEventSegment>())
		{
			if (ImGui::BeginPopup("Insert Event"))
			{
				handled = true;

				auto* action = mState.mAction->getDerived<InsertingEventSegment>();

				for(auto& type : getEventTypesVector())
				{
					std::string buttonString = "Insert " + type.get_name().to_string();
					if( ImGui::Button(buttonString.c_str()))
					{
						auto& views = getSegmentViews();
						auto it = views.find(type);
						assert(it!=views.end()); // type not found
						if( it != views.end() )
						{
							it->second->insertSegment(getEditor().getController<SequenceControllerEvent>(), action->mTrackID, action->mTime);
							ImGui::CloseCurrentPopup();
							mState.mAction = createAction<None>();
						}
					}
				}

				// handle paste if event segment is in clipboard
				if( mState.mClipboard->isClipboard<EventSegmentClipboard>())
				{
					if( ImGui::Button("Paste") )
					{
						// call appropriate paste method
						auto* event_segment_clipboard = mState.mClipboard->getDerived<EventSegmentClipboard>();
						auto& paste_event_map = getPasteEventMap();
						auto it = paste_event_map.find(event_segment_clipboard->getSegmentType());
						assert(it!=paste_event_map.end()); // type not found
						if( it != paste_event_map.end() )
						{
							(*this.*it->second)(action->mTrackID, action->mTime);
						}

						// exit popup
						ImGui::CloseCurrentPopup();
						mState.mAction = createAction<None>();
					}
				}

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
					mState.mAction = createAction<None>();
				}

				ImGui::EndPopup();
			}
			else
			{
				// click outside popup so cancel action
				mState.mAction = createAction<None>();
			}
		}

		return handled;
	}


	void SequenceEventTrackView::drawSegmentHandler(
		const SequenceTrack& track,
		const SequenceTrackSegment& segment,
		const ImVec2 &trackTopLeft,
		const float segmentX,
		const float segmentWidth,
		ImDrawList* drawList)
	{
		// segment handler
        if (((mState.mIsWindowFocused && ImGui::IsMouseHoveringRect(
            { trackTopLeft.x + segmentX - 10, trackTopLeft.y - 10 },
            { trackTopLeft.x + segmentX + 10, trackTopLeft.y + mState.mTrackHeight + 10 })) &&
             ( mState.mAction->isAction<None>() || ( mState.mAction->isAction<HoveringSegment>() && mState.mAction->getDerived<HoveringSegment>()->mSegmentID == segment.mID)))
			||
			( mState.mAction->isAction<DraggingSegment>() &&  mState.mAction->getDerived<DraggingSegment>()->mSegmentID == segment.mID))
		{
			bool isAlreadyHovering = false;
			if (mState.mAction->isAction<HoveringSegment>())
			{
				isAlreadyHovering = mState.mAction->getDerived<HoveringSegment>()->mSegmentID == segment.mID;
			}

			bool isAlreadyDragging = false;
			if (mState.mAction->isAction<DraggingSegment>())
			{
				isAlreadyDragging = mState.mAction->getDerived<DraggingSegment>()->mSegmentID == segment.mID;
			}

			// draw handler of segment duration
			drawList->AddLine(
				{ trackTopLeft.x + segmentX, trackTopLeft.y }, // top left
				{ trackTopLeft.x + segmentX, trackTopLeft.y + mState.mTrackHeight }, // bottom right
				guicolors::white, // color
				3.0f); // thickness

			if (!isAlreadyHovering && !isAlreadyDragging)
			{
				if(!ImGui::IsMouseDragging(0))
				{
					// we are hovering this segment with the mouse
					mState.mAction = createAction<HoveringSegment>(track.mID, segment.mID);
				}

			}

			ImGui::BeginTooltip();
			ImGui::Text(formatTimeString(segment.mStartTime).c_str());
			ImGui::EndTooltip();

			// left mouse is start dragging
			if (!isAlreadyDragging)
			{
				if(  !mState.mAction->isAction<DraggingSegment>() )
				{
					if (ImGui::IsMouseDown(0))
					{
						mState.mAction = createAction<DraggingSegment>(track.mID, segment.mID);
					}
				}
			}
			else
			{
				if (ImGui::IsMouseDown(0))
				{
					float amount = mState.mMouseDelta.x / mState.mStepSize;

					auto& editor = getEditor();
					SequenceControllerEvent& eventController = editor.getController<SequenceControllerEvent>();
					eventController.segmentEventStartTimeChange(track.mID, segment.mID, segment.mStartTime + amount);
				}
			}

			// right mouse in deletion popup
			if (ImGui::IsMouseDown(1))
			{
				mState.mAction = createAction<OpenEditSegmentValuePopup>(track.mID, segment.mID, segment.get_type());
			}
		}
		else 
		{
			// draw handler of segment duration
			drawList->AddLine(
			{ trackTopLeft.x + segmentX, trackTopLeft.y }, // top left
			{ trackTopLeft.x + segmentX, trackTopLeft.y + mState.mTrackHeight }, // bottom right
				guicolors::white, // color
				1.0f); // thickness

			if (mState.mAction->isAction<HoveringSegment>())
			{
				auto* action = mState.mAction->getDerived<HoveringSegment>();
				if (action->mSegmentID == segment.mID)
				{
					mState.mAction = createAction<None>();
				}
			}
		}
		
		if (ImGui::IsMouseReleased(0))
		{
			if (mState.mAction->isAction<DraggingSegment>())
			{
				if (mState.mAction->getDerived<DraggingSegment>()->mSegmentID == segment.mID)
				{
					mState.mAction = createAction<None>();
				}
			}
		}
	}


	bool SequenceEventTrackView::handleDeleteSegmentPopup()
	{
		//
		bool handled = false;

		if (mState.mAction->isAction<OpenEditSegmentValuePopup>())
		{
			// invoke insert sequence popup
			ImGui::OpenPopup("Edit Segment");

			auto* action = mState.mAction->getDerived<OpenEditSegmentValuePopup>();
			mState.mAction = createAction<EditingSegment>(action->mTrackID, action->mSegmentID, action->mSegmentType);
		}

		// handle delete segment popup
		if (mState.mAction->isAction<EditingSegment>())
		{
			if (ImGui::BeginPopup("Edit Segment"))
			{
				handled = true;

				auto* action = mState.mAction->getDerived<EditingSegment>();

				if(ImGui::Button("Copy"))
				{
					// create clipboard
					mState.mClipboard = createClipboard<EventSegmentClipboard>(action->mSegmentType);

					// serialize segment
					utility::ErrorState errorState;
					auto& controller = getEditor().getController<SequenceControllerEvent>();
					const auto* event_segment = controller.getSegment(action->mTrackID, action->mSegmentID);
					mState.mClipboard->addObject(event_segment, errorState);

					// exit popup
					ImGui::CloseCurrentPopup();
					mState.mAction = createAction<None>();
				}

				if (ImGui::Button("Delete"))
				{
					auto& controller = getEditor().getController<SequenceControllerEvent>();
					controller.deleteSegment(action->mTrackID, action->mSegmentID);
					mState.mDirty = true;

					ImGui::CloseCurrentPopup();
					mState.mAction = createAction<None>();
				}
				else
				{
					if (action->mSegmentType.is_derived_from<SequenceTrackSegmentEventBase>())
					{
						if (ImGui::Button("Edit"))
						{
							auto& eventController = getEditor().getController<SequenceControllerEvent>();
							const SequenceTrackSegmentEventBase *eventSegment = dynamic_cast<const SequenceTrackSegmentEventBase*>(eventController.getSegment(action->mTrackID, action->mSegmentID));
							assert(eventSegment != nullptr);

							if( eventSegment != nullptr)
							{
								rttr::type type = eventSegment->get_type();

								auto& views = getSegmentViews();
								auto it = views.find(type);
								assert(it!=views.end()); // type not found
								if(it!=views.end())
								{
									mState.mAction = it->second->createEditAction(eventSegment, action->mTrackID, action->mSegmentID);
								}
							}

							ImGui::CloseCurrentPopup();
						}
					}
				}

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
					mState.mAction = createAction<None>();
				}

				ImGui::EndPopup();
			}
			else
			{
				// click outside popup so cancel action
				mState.mAction = createAction<None>();
			}
		}

		return handled;
	}

	//////////////////////////////////////////////////////////////////////////
	// std::string event segment view specialization
	//////////////////////////////////////////////////////////////////////////

	template<>
	void SequenceEventTrackSegmentView<std::string>::handleEditPopupContent(SequenceGUIActions::Action& action)
	{
		auto* edit_action = action.getDerived<SequenceGUIActions::EditingEventSegment<std::string>>();
		std::string& message = static_cast<std::string&>(edit_action->mValue);

		char buffer[256];
		strcpy(buffer, message.c_str());

		if (ImGui::InputText("message", buffer, 256))
		{
			message = std::string(buffer);
		}
	}


	template<>
	void SequenceEventTrackSegmentView<std::string>::drawEvent(const SequenceTrackSegment& segment, ImDrawList* drawList, const ImVec2& topLeft, const float x)
	{
		const auto& segment_event = static_cast<const SequenceTrackSegmentEventString&>(segment);

		std::ostringstream string_stream;
		string_stream << "\"" << segment_event.mValue << "\"" ;

		drawList->AddText(
			{ topLeft.x + x + 5, topLeft.y + 5 },
			guicolors::red, string_stream.str().c_str());
	}


	static bool register_segment_view_string = SequenceEventTrackView::registerSegmentView<std::string>();


	//////////////////////////////////////////////////////////////////////////
	// float event segment view specialization
	//////////////////////////////////////////////////////////////////////////


	template<>
	void SequenceEventTrackSegmentView<float>::handleEditPopupContent(SequenceGUIActions::Action& action)
	{
		auto* editAction = action.getDerived<SequenceGUIActions::EditingEventSegment<float>>();
		float& value = static_cast<float&>(editAction->mValue);

		ImGui::InputFloat("Value", &value);
	}


	template<>
	void SequenceEventTrackSegmentView<float>::drawEvent(const SequenceTrackSegment& segment, ImDrawList* drawList, const ImVec2& topLeft, const float x)
	{
		const auto& segment_event = static_cast<const SequenceTrackSegmentEventFloat&>(segment);

		std::ostringstream string_stream;
		string_stream << segment_event.mValue;

		drawList->AddText(
			{ topLeft.x + x + 5, topLeft.y + 5 },
			guicolors::red,
			string_stream.str().c_str());
	}


	static bool register_segment_view_float = SequenceEventTrackView::registerSegmentView<float>();


	//////////////////////////////////////////////////////////////////////////
	// int event segment view specialization
	//////////////////////////////////////////////////////////////////////////


	template<>
	void SequenceEventTrackSegmentView<int>::handleEditPopupContent(SequenceGUIActions::Action& action)
	{
		auto* edit_action = action.getDerived<SequenceGUIActions::EditingEventSegment<int>>();
		int& value = static_cast<int&>(edit_action->mValue);

		ImGui::InputInt("Value", &value);
	}


	template<>
	void SequenceEventTrackSegmentView<int>::drawEvent(const SequenceTrackSegment& segment, ImDrawList* drawList, const ImVec2& topLeft, const float x)
	{
		const auto& segment_event = static_cast<const SequenceTrackSegmentEventInt&>(segment);

		std::ostringstream string_stream;
		string_stream << segment_event.mValue;

		drawList->AddText(
			{ topLeft.x + x + 5, topLeft.y + 5 },
			guicolors::red,
			string_stream.str().c_str());
	}


	static bool register_segment_view_int = SequenceEventTrackView::registerSegmentView<int>();


	//////////////////////////////////////////////////////////////////////////
	// glm::vec2 event segment view specialization
	//////////////////////////////////////////////////////////////////////////


	template<>
	void SequenceEventTrackSegmentView<glm::vec2>::handleEditPopupContent(SequenceGUIActions::Action& action)
	{
		auto* edit_action = action.getDerived<SequenceGUIActions::EditingEventSegment<glm::vec2>>();
		glm::vec2& value = static_cast<glm::vec2&>(edit_action->mValue);

		ImGui::InputFloat2("Value", &value.x);
	}


	template<>
	void SequenceEventTrackSegmentView<glm::vec2>::drawEvent(const SequenceTrackSegment& segment, ImDrawList* drawList, const ImVec2& topLeft, const float x)
	{
		const auto& segment_event = static_cast<const SequenceTrackSegmentEventVec2&>(segment);

		std::ostringstream string_stream;
		string_stream << "(" << segment_event.mValue.x << ", " << segment_event.mValue.y << ")";

		drawList->AddText({ topLeft.x + x + 5, topLeft.y + 5 },guicolors::red,string_stream.str().c_str());
	}


	static bool register_segment_view_vec2 = SequenceEventTrackView::registerSegmentView<glm::vec2>();


	//////////////////////////////////////////////////////////////////////////
	// glm::vec3 event segment view specialization
	//////////////////////////////////////////////////////////////////////////


	template<>
	void SequenceEventTrackSegmentView<glm::vec3>::handleEditPopupContent(SequenceGUIActions::Action& action)
	{
		auto* edit_action = action.getDerived<SequenceGUIActions::EditingEventSegment<glm::vec3>>();
		glm::vec3& value = static_cast<glm::vec3&>(edit_action->mValue);

		ImGui::InputFloat3("Value", &value.x);
	}


	template<>
	void SequenceEventTrackSegmentView<glm::vec3>::drawEvent(const SequenceTrackSegment& segment, ImDrawList* drawList, const ImVec2& topLeft, const float x)
	{
		const auto& segment_event = static_cast<const SequenceTrackSegmentEventVec3&>(segment);

		std::ostringstream string_stream;
		string_stream << "(" << segment_event.mValue.x << ", " << segment_event.mValue.y << ", " << segment_event.mValue.z << ")";

		drawList->AddText({ topLeft.x + x + 5, topLeft.y + 5 },guicolors::red,
						  string_stream.str().c_str());
	}


	static bool register_segment_view_vec3 = SequenceEventTrackView::registerSegmentView<glm::vec3>();
}
