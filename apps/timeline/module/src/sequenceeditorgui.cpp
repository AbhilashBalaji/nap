// local includes
#include "sequenceeditorgui.h"
#include "napcolors.h"
#include "sequencetracksegmentnumeric.h"

// External Includes
#include <entity.h>
#include <imgui/imgui.h>
#include <nap/logger.h>
#include <utility/fileutils.h>
#include <iomanip>

RTTI_BEGIN_CLASS(nap::SequenceEditorGUI)
	RTTI_PROPERTY("Sequence Editor", &nap::SequenceEditorGUI::mSequenceEditor, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	bool SequenceEditorGUI::init(utility::ErrorState& errorState)
	{
		if (!Resource::init(errorState))
		{
			return false;
		}

		mView = std::make_unique<SequenceEditorGUIView>(
			mSequenceEditor->getController(),
			mID);

		return true;
	}


	void SequenceEditorGUI::onDestroy()
	{
	}


	void SequenceEditorGUI::draw()
	{
		//
		mView->draw();
	}


	SequenceEditorGUIView::SequenceEditorGUIView(
		SequenceEditorController& controller,
		std::string id) : SequenceEditorView(controller) {
		mID = id;
	}


	void SequenceEditorGUIView::draw()
	{
		//
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 mouseDelta = { mousePos.x - mPreviousMousePos.x, mousePos.y - mPreviousMousePos.y };
		mPreviousMousePos = mousePos;

		//
		const Sequence& sequence = mController.getSequence();
		SequencePlayer& sequencePlayer = mController.getSequencePlayer();

		// push id
		ImGui::PushID(mID.c_str());

		// 100 px per second, default
		float stepSize = mHorizontalResolution;

		// calc width of content in timeline window
		const float timelineWidth =
			stepSize * sequence.mDuration;

		const float trackInspectorWidth = 200.0f;
		
		// set content width of next window
		ImGui::SetNextWindowContentWidth(timelineWidth + trackInspectorWidth + mVerticalResolution);

		// begin window
		if (ImGui::Begin(
			mID.c_str(), // id
			(bool*)0, // open
			ImGuiWindowFlags_HorizontalScrollbar)) // window flags
		{
			// clear curve cache if we move the window
			ImVec2 windowPos = ImGui::GetWindowPos();
			if (windowPos.x != mPrevWindowPos.x || windowPos.y != mPrevWindowPos.y)
			{
				mCurveCache.clear();
			}
			mPrevWindowPos = windowPos;

			// clear curve cache if we scroll inside the window
			ImVec2 scroll = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
			if (scroll.x != mPrevScroll.x || scroll.y != mPrevScroll.y)
			{
				mCurveCache.clear();
			}
			mPrevScroll = scroll;


			ImGui::SetCursorPosX(ImGui::GetCursorPosX());

			//
			if (ImGui::Button("Save"))
			{
				mController.save();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save As"))
			{
				ImGui::OpenPopup("Save As");
				mState.currentAction = SAVE_AS;
				mState.currentActionData = std::make_unique<SequenceGUISaveShowData>();
			}

			ImGui::SameLine();

			if (ImGui::Button("Load"))
			{
				ImGui::OpenPopup("Load");
				mState.currentAction = LOAD;
				mState.currentActionData = std::make_unique<SequenceGUILoadShowData>();
			}

			ImGui::SameLine();

			if (sequencePlayer.getIsPlaying())
			{
				if (ImGui::Button("Stop"))
				{
					sequencePlayer.stop();
				}
			}
			else
			{
				if (ImGui::Button("Play"))
				{
					sequencePlayer.play();
				}
			}

			ImGui::SameLine();
			if (sequencePlayer.getIsPaused() && sequencePlayer.getIsPlaying())
			{
				if (ImGui::Button("Play"))
				{
					sequencePlayer.play();
				}
			}
			else
			{
				if (ImGui::Button("Pause"))
				{
					sequencePlayer.pause();
				}

			}

			ImGui::SameLine();
			if (ImGui::Button("Rewind"))
			{
				sequencePlayer.setPlayerTime(0.0);
			}

			ImGui::SameLine();
			bool isLooping = sequencePlayer.getIsLooping();
			if (ImGui::Checkbox("Loop", &isLooping))
			{
				sequencePlayer.setIsLooping(isLooping);
			}

			ImGui::SameLine();
			float playbackSpeed = sequencePlayer.getPlaybackSpeed();
			ImGui::PushItemWidth(50.0f);
			if (ImGui::DragFloat("speed", &playbackSpeed, 0.01f, -10.0f, 10.0f, "%.1f"))
			{
				playbackSpeed = math::clamp(playbackSpeed, -10.0f, 10.0f);
				sequencePlayer.setPlaybackSpeed(playbackSpeed);
			}
			ImGui::PopItemWidth();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::PushItemWidth(200.0f);
			if (ImGui::DragFloat("H-Zoom", &mHorizontalResolution, 0.5f, 10, 1000, "%0.1f"))
				mCurveCache.clear();
			ImGui::SameLine();
			if (ImGui::DragFloat("V-Zoom", &mVerticalResolution, 0.5f, 100, 1000, "%0.1f"))
				mCurveCache.clear();
			ImGui::PopItemWidth();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// we want to know if this window is focused in order to handle mouseinput
			// in child windows or not
			bool windowIsFocused = ImGui::IsRootWindowOrAnyChildFocused();
			
			// store position of next window ( player controller ), we need it later to draw the timelineplayer position 
			const ImVec2 timelineControllerWindowPosition = ImGui::GetCursorPos();
			drawPlayerController(
				windowIsFocused,
				sequencePlayer,
				trackInspectorWidth + 5.0f,
				timelineWidth, 
				mouseDelta);

			// move a little bit more up to align tracks nicely with timelinecontroller
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - mVerticalResolution - 10);

			// draw tracks
			drawTracks(
				sequencePlayer,
				windowIsFocused,
				sequence,
				trackInspectorWidth,
				timelineWidth,
				mousePos,
				stepSize,
				mouseDelta);
				
			// on top of everything, draw time line player position
			drawTimelinePlayerPosition(
				sequence,
				sequencePlayer,
				timelineControllerWindowPosition,
				trackInspectorWidth,
				timelineWidth);
			
			// handle insert segment popup
			handleInsertSegmentPopup();

			// handle delete segment popup
			handleDeleteSegmentPopup();

			// move the cursor below the tracks
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + mVerticalResolution + 10.0f);
			if (ImGui::Button("Insert New Track"))
			{
				mController.addNewTrackVector<glm::vec3>();
				mCurveCache.clear();
			}

			//
			handleLoadPopup();

			//
			handleSaveAsPopup();
		}

		ImGui::End();

		// pop id
		ImGui::PopID();
	}


	void SequenceEditorGUIView::drawTracks(
		const SequencePlayer& sequencePlayer,
		const bool isWindowFocused,
		const Sequence &sequence,
		const float inspectorWidth,
		const float timelineWidth,
		const ImVec2 &mousePos,
		const float stepSize,
		const ImVec2 &mouseDelta)
	{
		//
		bool deleteTrack = false;
		std::string deleteTrackID = "";

		// get current cursor pos, we will use this to position the track windows
		ImVec2 cursorPos = ImGui::GetCursorPos();

		// define consts
		const float trackHeight = mVerticalResolution;
		const float margin = 10.0f;

		int trackCount = 0;
		for (const auto& track : sequence.mTracks)
		{
			// begin inspector
			std::ostringstream inspectorIDStream;
			inspectorIDStream << track->mID << "inspector";
			std::string inspectorID = inspectorIDStream.str();

			// manually set the cursor position before drawing new track window
			cursorPos =
			{
				cursorPos.x , 
				trackHeight + margin + cursorPos.y
			};

			// manually set the cursor position before drawing inspector
			ImVec2 inspectorCursorPos = { cursorPos.x , cursorPos.y };
			ImGui::SetCursorPos(inspectorCursorPos);

			// draw inspector window
			if (ImGui::BeginChild(
				inspectorID.c_str(), // id
				{ inspectorWidth , trackHeight + 5 }, // size
				false, // no border
				ImGuiWindowFlags_NoMove)) // window flags
			{
				// obtain drawlist
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				// store window size and position
				const ImVec2 windowPos = ImGui::GetWindowPos();
				const ImVec2 windowSize = ImGui::GetWindowSize();

				// draw background & box
				drawList->AddRectFilled(
					windowPos,
					{windowPos.x + windowSize.x - 5, windowPos.y + trackHeight},
					guicolors::black);

				drawList->AddRect(
					windowPos,
					{ windowPos.x + windowSize.x - 5, windowPos.y + trackHeight },
					guicolors::white);

				// 
				ImVec2 inspectorCursorPos = ImGui::GetCursorPos();
				inspectorCursorPos.x += 5;
				inspectorCursorPos.y += 5;
				ImGui::SetCursorPos(inspectorCursorPos);

				// scale down everything
				float scale = 0.25f;
				ImGui::GetStyle().ScaleAllSizes(scale);

				// draw the assigned parameter
				ImGui::Text("Assigned Parameter");

				inspectorCursorPos = ImGui::GetCursorPos();
				inspectorCursorPos.x += 5;
				inspectorCursorPos.y += 5;
				ImGui::SetCursorPos(inspectorCursorPos);

				bool assigned = false;
				std::string assignedID;
				std::vector<std::string> parameterIDs;
				int currentItem = 0;
				parameterIDs.emplace_back("none");
				int count = 0;
				const Parameter* assignedParameterPtr = nullptr;
				for(const auto& parameter : sequencePlayer.mParameters)
				{
					count++;

					if (parameter->mID == track->mAssignedParameterID)
					{
						assigned = true;
						assignedID = parameter->mID;
						currentItem = count;
						assignedParameterPtr = parameter.get();
					}

					parameterIDs.emplace_back(parameter->mID);
				}

				ImGui::PushItemWidth(140.0f);
				if (Combo(
					"",
					&currentItem, 
					parameterIDs))
				{
					if(currentItem!=0)
						mController.assignNewParameterID(track->mID, parameterIDs[currentItem]);
					else
						mController.assignNewParameterID(track->mID, "");
					
				}

				//
				ImGui::PopItemWidth();

				// delete track button
				ImGui::Spacing();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				// when we delete a track, we don't immediately call the controller because we are iterating track atm
				if (ImGui::SmallButton("Delete"))
				{
					deleteTrack = true;
					deleteTrackID = track->mID;
				}

				// pop scale
				ImGui::GetStyle().ScaleAllSizes(1.0f / scale);
			}
			ImGui::EndChild();

			const ImVec2 windowCursorPos = { cursorPos.x + inspectorWidth + 5, cursorPos.y };
			ImGui::SetCursorPos(windowCursorPos);

			// begin track
			if (ImGui::BeginChild(
				track->mID.c_str(), // id
				{ timelineWidth + 5 , trackHeight + 5 }, // size
				false, // no border
				ImGuiWindowFlags_NoMove)) // window flags
			{
				// push id
				ImGui::PushID(track->mID.c_str());

				// get child focus
				bool trackHasFocus = ImGui::IsMouseHoveringWindow();

				// get window drawlist
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				// get current imgui cursor position
				ImVec2 cursorPos = ImGui::GetCursorPos();

				// get window position
				ImVec2 windowTopLeft = ImGui::GetWindowPos();

				// calc beginning of timeline graphic
				ImVec2 trackTopLeft = { windowTopLeft.x + cursorPos.x, windowTopLeft.y + cursorPos.y };

				// draw background of track
				drawList->AddRectFilled(
					trackTopLeft, // top left position
					{ trackTopLeft.x + timelineWidth, trackTopLeft.y + trackHeight }, // bottom right position
					guicolors::black); // color 

				// draw border of track
				drawList->AddRect(
					trackTopLeft, // top left position
					{ trackTopLeft.x + timelineWidth, trackTopLeft.y + trackHeight }, // bottom right position
					guicolors::white); // color 

				if (isWindowFocused)
				{
					// handle insertion of segment
					if (mState.currentAction == SequenceGUIMouseActions::NONE)
					{
						if (ImGui::IsMouseHoveringRect(
							trackTopLeft, // top left position
							{ trackTopLeft.x + timelineWidth, trackTopLeft.y + trackHeight }))
						{
							// position of mouse in track
							drawList->AddLine(
							{ mousePos.x, trackTopLeft.y }, // top left
							{ mousePos.x, trackTopLeft.y + trackHeight }, // bottom right
								guicolors::lightGrey, // color
								1.0f); // thickness

									   // right mouse down
							if (ImGui::IsMouseClicked(1))
							{
								double time = (mousePos.x - trackTopLeft.x) / stepSize;

								//
								mState.currentAction = OPEN_INSERT_SEGMENT_POPUP;
								mState.currentActionData = std::make_unique<SequenceGUIInsertSegmentData>(track->mID, time, static_cast<SequenceTrackTypes>(track->mTrackType));
							}
						}
					}

					// draw line in track while in inserting segment popup
					if (mState.currentAction == SequenceGUIMouseActions::OPEN_INSERT_SEGMENT_POPUP || mState.currentAction == INSERTING_SEGMENT)
					{
						const SequenceGUIInsertSegmentData* data = dynamic_cast<SequenceGUIInsertSegmentData*>(mState.currentActionData.get());
						if (data->trackID == track->mID)
						{
							// position of insertion in track
							drawList->AddLine(
							{ trackTopLeft.x + (float)data->time * stepSize, trackTopLeft.y }, // top left
							{ trackTopLeft.x + (float)data->time * stepSize, trackTopLeft.y + trackHeight }, // bottom right
								guicolors::lightGrey, // color
								1.0f); // thickness
						}
					}
				}

				float previousSegmentX = 0.0f;

				SequenceTrackTypes trackType = track->getTrackType();

				int segmentCount = 0;
				for (const auto& segment : track->mSegments)
				{
					float segmentX = (segment->mStartTime + segment->mDuration) * stepSize;
					float segmentWidth = segment->mDuration * stepSize;

					if (trackType == SequenceTrackTypes::NUMERIC)
					{
						drawSegmentContentNumeric(
							isWindowFocused,
							*track.get(),
							*segment.get(),
							trackTopLeft,
							previousSegmentX,
							segmentWidth,
							trackHeight,
							segmentX,
							stepSize,
							drawList,
							mouseDelta,
							(segmentCount == 0));
					}
					else if( trackType == SequenceTrackTypes::VEC3 )
					{
						drawSegmentContentVec<glm::vec3>(
							isWindowFocused,
							*track.get(),
							*segment.get(),
							trackTopLeft,
							previousSegmentX,
							segmentWidth,
							trackHeight,
							segmentX,
							stepSize,
							drawList,
							mouseDelta,
							(segmentCount == 0));
					}
					else if (trackType == SequenceTrackTypes::VEC2)
					{
						drawSegmentContentVec<glm::vec2>(
							isWindowFocused,
							*track.get(),
							*segment.get(),
							trackTopLeft,
							previousSegmentX,
							segmentWidth,
							trackHeight,
							segmentX,
							stepSize,
							drawList,
							mouseDelta,
							(segmentCount == 0));
					}


					// draw segment handlers
					drawSegmentHandler(
						isWindowFocused,
						*track.get(),
						*segment.get(),
						trackTopLeft,
						segmentX,
						segmentWidth,
						trackHeight,
						mouseDelta,
						stepSize,
						drawList);

					//
					previousSegmentX = segmentX;

					//
					segmentCount++;
				}

				// pop id
				ImGui::PopID();

			}

			ImGui::End();

			//
			ImGui::SetCursorPos(cursorPos);

			// increment track count
			trackCount++;
		}

		// delete the track if we did a delete track action
		if (deleteTrack)
		{
			mController.deleteTrack(deleteTrackID);
			mCurveCache.clear();
		}
	}

	void SequenceEditorGUIView::drawControlPointsNumeric(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment_,
		const ImVec2 &trackTopLeft,
		const float segmentX,
		const float segmentWidth,
		const float trackHeight,
		const ImVec2 mouseDelta,
		const int stepSize,
		ImDrawList* drawList)
	{
		const SequenceTrackSegmentNumeric& segment = segment_.getDerivedConst<SequenceTrackSegmentNumeric>();

		// draw first control point handlers IF this is the first segment of the track
		if (track.mSegments[0]->mID == segment.mID)
		{
			const auto& curvePoint = segment.mCurve->mPoints[0];
			std::ostringstream stringStream;
			stringStream << segment.mID << "_point_" << 0;

			ImVec2 circlePoint =
			{ (trackTopLeft.x + segmentX - segmentWidth) + segmentWidth * curvePoint.mPos.mTime,
				trackTopLeft.y + trackHeight * (1.0f - (float) curvePoint.mPos.mValue) };

			drawTanHandlerNum(
				isWindowFocused,
				track,
				segment,
				stringStream,
				segmentWidth,
				curvePoint,
				trackHeight,
				circlePoint,
				0,
				TanPointTypes::IN,
				mouseDelta,
				stepSize,
				drawList);

			drawTanHandlerNum(
				isWindowFocused,
				track,
				segment,
				stringStream,
				segmentWidth,
				curvePoint,
				trackHeight,
				circlePoint,
				0,
				TanPointTypes::OUT,
				mouseDelta,
				stepSize,
				drawList);
		}

		// draw control points of curve
		// we ignore the first and last because they are controlled by the start & end value of the segment
		for (int i = 1; i < segment.mCurve->mPoints.size() - 1; i++)
		{
			// get the curvepoint and generate a unique ID for the control point
			const auto& curvePoint = segment.mCurve->mPoints[i];
			std::ostringstream stringStream;
			stringStream << segment.mID << "_point_" << i;
			std::string pointID = stringStream.str();

			// determine the point at where to draw the control point
			ImVec2 circlePoint =
			{ (trackTopLeft.x + segmentX - segmentWidth) + segmentWidth * curvePoint.mPos.mTime,
				trackTopLeft.y + trackHeight * (1.0f - (float) curvePoint.mPos.mValue) };

			// handle mouse hovering
			bool hovered = false;
			if (isWindowFocused)
			{
				if ((mState.currentAction == NONE ||
					mState.currentAction == HOVERING_CONTROL_POINT_NUM ||
					mState.currentAction == HOVERING_CURVE_NUM)
					&& ImGui::IsMouseHoveringRect(
				{ circlePoint.x - 5, circlePoint.y - 5 },
				{ circlePoint.x + 5, circlePoint.y + 5 }))
				{
					hovered = true;
				}
			}

			if (hovered)
			{
				// if we are hovering this point, store ID
				mState.currentAction = HOVERING_CONTROL_POINT_NUM;
				mState.currentObjectID = pointID;

				// is the mouse held down, then we are dragging
				if (ImGui::IsMouseDown(0))
				{
					mState.currentAction = DRAGGING_CONTROL_POINT_NUM;
					mState.currentActionData = std::make_unique<SequenceGUIDragControlPointDataNum>(track.mID, segment.mID, i);
					mState.currentObjectID = segment.mID;
				}
				// if we clicked right mouse button, delete control point
				else if( ImGui::IsMouseClicked(1))
				{
					mState.currentAction = DELETE_CONTROL_POINT_NUM;
					mState.currentActionData = std::make_unique<SequenceGUIDeleteControlPointDataNum>(track.mID, segment.mID, i);
					mState.currentObjectID = segment.mID;
				}
			}
			else
			{
				// otherwise, if we where hovering but not anymore, stop hovering
				if (mState.currentAction == HOVERING_CONTROL_POINT_NUM &&
					pointID == mState.currentObjectID)
				{
					mState.currentAction = NONE;
				}
			}

			if (isWindowFocused)
			{
				// handle dragging of control point
				if (mState.currentAction == DRAGGING_CONTROL_POINT_NUM &&
					segment.mID == mState.currentObjectID)
				{
					const SequenceGUIDragControlPointDataNum* data
						= dynamic_cast<SequenceGUIDragControlPointDataNum*>(mState.currentActionData.get());

					if (data->controlPointIndex == i)
					{
						float timeAdjust = mouseDelta.x / segmentWidth;
						float valueAdjust = (mouseDelta.y / trackHeight) * -1.0f;

						hovered = true;

						mController.changeCurvePointNumeric(
							data->trackID,
							data->segmentID,
							data->controlPointIndex,
							timeAdjust,
							valueAdjust);
						mCurveCache.clear();

						if (ImGui::IsMouseReleased(0))
						{
							mState.currentAction = NONE;
							mState.currentActionData = nullptr;
						}
					}
				}

				// handle deletion of control point
				if (mState.currentAction == DELETE_CONTROL_POINT_NUM &&
					segment.mID == mState.currentObjectID)
				{
					const SequenceGUIDeleteControlPointDataNum* data
						= dynamic_cast<SequenceGUIDeleteControlPointDataNum*>(mState.currentActionData.get());

					if (data->controlPointIndex == i)
					{
						mController.deleteCurvePointNum(
							data->trackID,
							data->segmentID,
							data->controlPointIndex);
						mCurveCache.clear();

						mState.currentAction = NONE;
						mState.currentActionData = nullptr;

					}
				}
			}
		

			// draw the control point
			drawList->AddCircleFilled(
				circlePoint,
				4.0f,
				hovered ? guicolors::white : guicolors::lightGrey);

			// draw the handlers
			drawTanHandlerNum(
				isWindowFocused,
				track,
				segment,
				stringStream,
				segmentWidth,
				curvePoint,
				trackHeight,
				circlePoint,
				i,
				TanPointTypes::IN,
				mouseDelta,
				stepSize,
				drawList);

			drawTanHandlerNum(
				isWindowFocused,
				track,
				segment,
				stringStream,
				segmentWidth,
				curvePoint,
				trackHeight,
				circlePoint,
				i,
				TanPointTypes::OUT,
				mouseDelta,
				stepSize,
				drawList);
		}

		// handle last control point
		// overlaps with endvalue so only draw tan handlers
		const int controlPointIndex = segment.mCurve->mPoints.size() - 1;
		const auto& curvePoint = segment.mCurve->mPoints[controlPointIndex];

		std::ostringstream stringStream;
		stringStream << segment.mID << "_point_" << controlPointIndex;
		std::string pointID = stringStream.str();

		ImVec2 circlePoint =
		{ (trackTopLeft.x + segmentX - segmentWidth) + segmentWidth * curvePoint.mPos.mTime,
		   trackTopLeft.y + trackHeight * (1.0f - (float)curvePoint.mPos.mValue) };

		drawTanHandlerNum(
			isWindowFocused,
			track,
			segment,
			stringStream,
			segmentWidth,
			curvePoint,
			trackHeight,
			circlePoint,
			controlPointIndex,
			TanPointTypes::IN,
			mouseDelta,
			stepSize,
			drawList);

		drawTanHandlerNum(
			isWindowFocused,
			track,
			segment,
			stringStream,
			segmentWidth,
			curvePoint,
			trackHeight,
			circlePoint,
			controlPointIndex,
			TanPointTypes::OUT,
			mouseDelta,
			stepSize,
			drawList);

		//
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - trackHeight);
	}


	template<typename T>
	void SequenceEditorGUIView::drawControlPointsVec(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment_,
		const ImVec2 &trackTopLeft,
		const float segmentX,
		const float segmentWidth,
		const float trackHeight,
		const ImVec2 mouseDelta,
		const int stepSize,
		ImDrawList* drawList)
	{

		const SequenceTrackSegmentVec<T>& segment 
			= segment_.getDerivedConst<SequenceTrackSegmentVec<T>>();

		// draw first control point(s) handlers IF this is the first segment of the track
		if (track.mSegments[0]->mID == segment.mID)
		{
			for (int v = 0; v < segment.mCurves.size(); v++)
			{
				const auto& curvePoint = segment.mCurves[v]->mPoints[0];
				std::ostringstream stringStream;
				stringStream << segment.mID << "_point_" << 0 << "_curve_" << v;

				ImVec2 circlePoint =
				{ (trackTopLeft.x + segmentX - segmentWidth) + segmentWidth * curvePoint.mPos.mTime,
					trackTopLeft.y + trackHeight * (1.0f - (float)curvePoint.mPos.mValue) };

				drawTanHandlerVec<T>(
					isWindowFocused,
					track,
					segment,
					stringStream,
					segmentWidth,
					curvePoint,
					trackHeight,
					circlePoint,
					0,
					v,
					TanPointTypes::IN,
					mouseDelta,
					stepSize,
					drawList);

				drawTanHandlerVec<T>(
					isWindowFocused,
					track,
					segment,
					stringStream,
					segmentWidth,
					curvePoint,
					trackHeight,
					circlePoint,
					0,
					v,
					TanPointTypes::OUT,
					mouseDelta,
					stepSize,
					drawList);
			}
		}

		// draw control points of curves
		// we ignore the first and last because they are controlled by the start & end value of the segment
		for (int v = 0; v < segment.mCurves.size(); v++)
		{
			for (int i = 1; i < segment.mCurves[v]->mPoints.size() - 1; i++)
			{
				// get the curvepoint and generate a unique ID for the control point
				const auto& curvePoint = segment.mCurves[v]->mPoints[i];
				std::ostringstream stringStream;
				stringStream << segment.mID << "_point_" << i << "_curve_" << v;
				std::string pointID = stringStream.str();

				// determine the point at where to draw the control point
				ImVec2 circlePoint =
				{ (trackTopLeft.x + segmentX - segmentWidth) + segmentWidth * curvePoint.mPos.mTime,
					trackTopLeft.y + trackHeight * (1.0f - (float)curvePoint.mPos.mValue) };

				// handle mouse hovering
				bool hovered = false;
				if (isWindowFocused)
				{
					if ((mState.currentAction == NONE ||
						mState.currentAction == HOVERING_CONTROL_POINT_VEC ||
						mState.currentAction == HOVERING_CURVE_VEC)
						&& ImGui::IsMouseHoveringRect(
					{ circlePoint.x - 5, circlePoint.y - 5 },
					{ circlePoint.x + 5, circlePoint.y + 5 }))
					{
						hovered = true;
					}
				}

				if (hovered)
				{
					// if we are hovering this point, store ID
					mState.currentAction = HOVERING_CONTROL_POINT_VEC;
					mState.currentObjectID = pointID;

					// is the mouse held down, then we are dragging
					if (ImGui::IsMouseDown(0))
					{
						mState.currentAction = DRAGGING_CONTROL_POINT_VEC;
						mState.currentActionData = std::make_unique<SequenceGUIControlPointDataVec>(
							track.mID, 
							segment.mID, 
							i,
							v);
						mState.currentObjectID = segment.mID;
					}
					// if we clicked right mouse button, delete control point
					else if (ImGui::IsMouseClicked(1))
					{
						mState.currentAction = DELETE_CONTROL_POINT_VEC;
						mState.currentActionData = std::make_unique<SequenceGUIDeleteControlPointDataVec>(
							track.mID, 
							segment.mID, 
							i,
							v);
						mState.currentObjectID = segment.mID;
					}
				}
				else
				{
					// otherwise, if we where hovering but not anymore, stop hovering
					if (mState.currentAction == HOVERING_CONTROL_POINT_VEC &&
						pointID == mState.currentObjectID)
					{
						mState.currentAction = NONE;
					}
				}

				if (isWindowFocused)
				{
					// handle dragging of control point
					if (mState.currentAction == DRAGGING_CONTROL_POINT_VEC &&
						segment.mID == mState.currentObjectID)
					{
						const SequenceGUIControlPointDataVec* data
							= dynamic_cast<SequenceGUIControlPointDataVec*>(mState.currentActionData.get());

						if (data->controlPointIndex == i && data->curveIndex == v)
						{
							float timeAdjust = mouseDelta.x / segmentWidth;
							float valueAdjust = (mouseDelta.y / trackHeight) * -1.0f;

							hovered = true;

							mController.changeCurvePointVec<T>(
								data->trackID,
								data->segmentID,
								data->controlPointIndex,
								data->curveIndex,
								timeAdjust,
								valueAdjust);
							mCurveCache.clear();

							if (ImGui::IsMouseReleased(0))
							{
								mState.currentAction = NONE;
								mState.currentActionData = nullptr;
							}
						}
					}

					// handle deletion of control point
					if (mState.currentAction == DELETE_CONTROL_POINT_VEC &&
						segment.mID == mState.currentObjectID)
					{
						const SequenceGUIDeleteControlPointDataVec* data
							= dynamic_cast<SequenceGUIDeleteControlPointDataVec*>(mState.currentActionData.get());

						if (data->controlPointIndex == i && 
							data->curveIndex == v)
						{
							mController.deleteCurvePointVec<T>(
								data->trackID,
								data->segmentID,
								data->controlPointIndex,
								data->curveIndex);
							mCurveCache.clear();

							mState.currentAction = NONE;
							mState.currentActionData = nullptr;
						}
					}
				}


				// draw the control point
				drawList->AddCircleFilled(
					circlePoint,
					4.0f,
					hovered ? guicolors::white : guicolors::lightGrey);

				// draw the handlers
				drawTanHandlerVec<T>(
					isWindowFocused,
					track,
					segment,
					stringStream,
					segmentWidth,
					curvePoint,
					trackHeight,
					circlePoint,
					i,
					v,
					TanPointTypes::IN,
					mouseDelta,
					stepSize,
					drawList);

				drawTanHandlerVec<T>(
					isWindowFocused,
					track,
					segment,
					stringStream,
					segmentWidth,
					curvePoint,
					trackHeight,
					circlePoint,
					i,
					v,
					TanPointTypes::OUT,
					mouseDelta,
					stepSize,
					drawList);
			}
		}

		for (int v = 0; v < segment.mCurves.size(); v++)
		{
			// handle last control point
			// overlaps with endvalue so only draw tan handlers
			const int controlPointIndex = segment.mCurves[v]->mPoints.size() - 1;
			const auto& curvePoint = segment.mCurves[v]->mPoints[controlPointIndex];

			std::ostringstream stringStream;
			stringStream << segment.mID << "_point_" << controlPointIndex << "_curve_" << v;
			std::string pointID = stringStream.str();

			ImVec2 circlePoint =
			{ (trackTopLeft.x + segmentX - segmentWidth) + segmentWidth * curvePoint.mPos.mTime,
				trackTopLeft.y + trackHeight * (1.0f - (float)curvePoint.mPos.mValue) };

			drawTanHandlerVec<T>(
				isWindowFocused,
				track,
				segment,
				stringStream,
				segmentWidth,
				curvePoint,
				trackHeight,
				circlePoint,
				controlPointIndex,
				v,
				TanPointTypes::IN,
				mouseDelta,
				stepSize,
				drawList);

			drawTanHandlerVec<T>(
				isWindowFocused,
				track,
				segment,
				stringStream,
				segmentWidth,
				curvePoint,
				trackHeight,
				circlePoint,
				controlPointIndex,
				v,
				TanPointTypes::OUT,
				mouseDelta,
				stepSize,
				drawList);
		}

		//
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - trackHeight);
	}


	void SequenceEditorGUIView::drawCurveNumeric(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment_,
		const ImVec2 &trackTopLeft,
		const float previousSegmentX,
		const float segmentWidth,
		const float trackHeight,
		const float segmentX,
		const float stepSize,
		ImDrawList* drawList)
	{
		const SequenceTrackSegmentNumeric& segment = segment_.getDerivedConst<SequenceTrackSegmentNumeric>();

		const int resolution = 40;
		bool curveSelected = false;

		// check the cache
		if (mCurveCache.find(segment.mID) == mCurveCache.end())
		{
			std::vector<ImVec2> points;
			points.resize(resolution + 1);
			for (int i = 0; i <= resolution; i++)
			{
				float value = 1.0f - segment.mCurve->evaluate((float)i / resolution);

				points[i] = {
					trackTopLeft.x + previousSegmentX + segmentWidth * ((float)i / resolution),
					trackTopLeft.y + value * trackHeight };
			}

			mCurveCache.emplace(segment.mID, points);
		}

		const std::vector<ImVec2>& points = mCurveCache[segment.mID];

		// because we need to retain the reference to the points in the cache, only clear the cache
		// on the end of this scrope
		bool clearCurveCache = false;
		if (isWindowFocused)
		{
			// determine if mouse is hovering curve
			if ((mState.currentAction == NONE || 
				mState.currentAction == HOVERING_CURVE_NUM)
				&& ImGui::IsMouseHoveringRect(
			{ trackTopLeft.x + segmentX - segmentWidth, trackTopLeft.y }, // top left
			{ trackTopLeft.x + segmentX, trackTopLeft.y + trackHeight }))  // bottom right 
			{
				// translate mouse position to position in curve
				ImVec2 mousePos = ImGui::GetMousePos();
				float xInSegment = ((mousePos.x - (trackTopLeft.x + segmentX - segmentWidth)) / stepSize) / segment.mDuration;
				float yInSegment = 1.0f - ((mousePos.y - trackTopLeft.y) / trackHeight);

				// evaluate curve at x position
				float yInCurve = segment.mCurve->evaluate(xInSegment);

				// insert curve point on click
				const float maxDist = 0.1f;
				if (abs(yInCurve - yInSegment) < maxDist)
				{
					curveSelected = true;
					mState.currentAction = HOVERING_CURVE_NUM;
					mState.currentObjectID = segment.mID;

					if (ImGui::IsMouseClicked(0))
					{
						mController.insertCurvePointNum(track.mID, segment.mID, xInSegment);
						clearCurveCache = true;
					}
				}
				else
				{
					if (mState.currentAction == HOVERING_CURVE_NUM &&
						mState.currentObjectID == segment.mID)
					{
						mState.currentAction = NONE;
					}
				}
			}
			else
			{
				if (mState.currentAction == HOVERING_CURVE_NUM &&
					mState.currentObjectID == segment.mID)
				{
					mState.currentAction = NONE;
				}
			}
		}

		// draw points of curve
		drawList->AddPolyline(
			&*points.begin(), // points array
			points.size(), // size of points array
			guicolors::red, // color
			false, // closed
			curveSelected ? 3.0f : 1.0f, // thickness
			true); // anti-aliased

		//
		if (clearCurveCache)
			mCurveCache.clear();
	}


	void SequenceEditorGUIView::drawSegmentValueNum(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment_,
		const ImVec2 &trackTopLeft,
		const float segmentX,
		const float segmentWidth,
		const float trackHeight,
		const ImVec2 &mouseDelta,
		const float stepSize,
		const SegmentValueTypes segmentType,
		ImDrawList* drawList
	)
	{
		const SequenceTrackSegmentNumeric& segment = segment_.getDerivedConst<SequenceTrackSegmentNumeric>();

		// calculate point of this value in the window
		ImVec2 segmentValuePos =
		{
			trackTopLeft.x + segmentX - (segmentType == BEGIN ? segmentWidth : 0.0f),
			trackTopLeft.y + trackHeight * (1.0f - ((segmentType == BEGIN ? (float) segment.mStartValue : (float) segment.mEndValue) / 1.0f))
		};

		bool hovered = false;

		if (isWindowFocused)
		{
			// check if we are hovering this value
			if ((mState.currentAction == SequenceGUIMouseActions::NONE ||
				mState.currentAction == HOVERING_SEGMENT_VALUE_NUM ||
				mState.currentAction == HOVERING_SEGMENT ||
				mState.currentAction == HOVERING_CURVE_NUM)
				&& ImGui::IsMouseHoveringRect(
			{ segmentValuePos.x - 12, segmentValuePos.y - 12 }, // top left
			{ segmentValuePos.x + 12, segmentValuePos.y + 12 }))  // bottom right 
			{
				hovered = true;
				mState.currentAction = HOVERING_SEGMENT_VALUE_NUM;
				mState.currentActionData = std::make_unique<SequenceGUIDragSegmentData>(
					track.mID,
					segment.mID,
					segmentType);

				if (ImGui::IsMouseDown(0))
				{
					mState.currentAction = DRAGGING_SEGMENT_VALUE_NUM;
					mState.currentObjectID = segment.mID;
				}
			}
			else if (mState.currentAction != DRAGGING_SEGMENT_VALUE_NUM)
			{
				if (mState.currentAction == HOVERING_SEGMENT_VALUE_NUM)
				{
					const SequenceGUIDragSegmentData* data =
						dynamic_cast<SequenceGUIDragSegmentData*>(mState.currentActionData.get());

					if (data->type == segmentType && data->segmentID == segment.mID)
					{
						mState.currentAction = SequenceGUIMouseActions::NONE;
					}
				}
			}

			// handle dragging segment value
			if (mState.currentAction == DRAGGING_SEGMENT_VALUE_NUM &&
				mState.currentObjectID == segment.mID)
			{
				const SequenceGUIDragSegmentData* data =
					dynamic_cast<SequenceGUIDragSegmentData*>(mState.currentActionData.get());

				if (data->type == segmentType)
				{
					hovered = true;

					if (ImGui::IsMouseReleased(0))
					{
						mState.currentAction = NONE;
					}
					else
					{
						float dragAmount = (mouseDelta.y / trackHeight) * -1.0f;
						mController.changeSegmentValueNumeric(
							track.mID,
							segment.mID, 
							dragAmount,
							segmentType);
						mCurveCache.clear();
					}
				}
			}
		}

		if (hovered)
			drawList->AddCircleFilled(segmentValuePos, 5.0f, guicolors::red);
		else
			drawList->AddCircle(segmentValuePos, 5.0f, guicolors::red);
	}

	template<typename T>
	void SequenceEditorGUIView::drawSegmentValueVec(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment_,
		const ImVec2 &trackTopLeft,
		const float segmentX,
		const float segmentWidth,
		const float trackHeight,
		const ImVec2 &mouseDelta,
		const float stepSize,
		const SegmentValueTypes segmentType,
		ImDrawList* drawList
	)
	{
		const SequenceTrackSegmentVec<T>& segment = 
			segment_.getDerivedConst<SequenceTrackSegmentVec<T>>();

		for (int v = 0; v < segment.mCurves.size(); v++)
		{
			// calculate point of this value in the window
			ImVec2 segmentValuePos =
			{
				trackTopLeft.x + segmentX - (segmentType == BEGIN ? segmentWidth : 0.0f),
				trackTopLeft.y + trackHeight * (1.0f - ((segmentType == BEGIN ? (float)segment.mStartValue[v] : (float)segment.mEndValue[v]) / 1.0f))
			};

			bool hovered = false;

			if (isWindowFocused)
			{
				// check if we are hovering this value
				if ((mState.currentAction == SequenceGUIMouseActions::NONE ||
					mState.currentAction == HOVERING_SEGMENT_VALUE_VEC ||
					mState.currentAction == HOVERING_SEGMENT ||
					mState.currentAction == HOVERING_CURVE_VEC)
					&& ImGui::IsMouseHoveringRect(
						{ segmentValuePos.x - 12, segmentValuePos.y - 12 }, // top left
						{ segmentValuePos.x + 12, segmentValuePos.y + 12 }))  // bottom right 
				{
					hovered = true;
					mState.currentAction = HOVERING_SEGMENT_VALUE_VEC;
					mState.currentActionData = std::make_unique<SequenceGUIDragSegmentDataVec>(
						track.mID,
						segment.mID,
						segmentType,
						v);

					if (ImGui::IsMouseDown(0))
					{
						mState.currentAction = DRAGGING_SEGMENT_VALUE_VEC;
						mState.currentObjectID = segment.mID;
					}
				}
				else if (mState.currentAction != DRAGGING_SEGMENT_VALUE_VEC)
				{
					if (mState.currentAction == HOVERING_SEGMENT_VALUE_VEC)
					{
						const SequenceGUIDragSegmentDataVec* data =
							dynamic_cast<SequenceGUIDragSegmentDataVec*>(mState.currentActionData.get());

						if (data->type == segmentType && 
							data->segmentID == segment.mID &&
							data->curveIndex == v)
						{
							mState.currentAction = SequenceGUIMouseActions::NONE;
						}
					}
				}

				// handle dragging segment value
				if (mState.currentAction == DRAGGING_SEGMENT_VALUE_VEC &&
					mState.currentObjectID == segment.mID)
				{
					const SequenceGUIDragSegmentDataVec* data =
						dynamic_cast<SequenceGUIDragSegmentDataVec*>(mState.currentActionData.get());

					if (data->type == segmentType && data->curveIndex == v)
					{
						hovered = true;

						if (ImGui::IsMouseReleased(0))
						{
							mState.currentAction = NONE;
						}
						else
						{
							float dragAmount = (mouseDelta.y / trackHeight) * -1.0f;
							
							mController.changeSegmentValueVec<T>(
								track.mID,
								segment.mID,
								dragAmount,
								v,
								segmentType);
							mCurveCache.clear();
						}
					}
				}
			}

			if (hovered)
				drawList->AddCircleFilled(segmentValuePos, 5.0f, guicolors::red);
			else
				drawList->AddCircle(segmentValuePos, 5.0f, guicolors::red);
		}
	}


	void SequenceEditorGUIView::drawSegmentHandler(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment,
		const ImVec2 &trackTopLeft,
		const float segmentX,
		const float segmentWidth,
		const float trackHeight,
		const ImVec2 &mouseDelta,
		const float stepSize,
		ImDrawList* drawList)
	{
		// segment handler
		if (isWindowFocused &&
			(mState.currentAction == SequenceGUIMouseActions::NONE ||
			(mState.currentAction == HOVERING_SEGMENT && mState.currentObjectID == segment.mID)) &&
			ImGui::IsMouseHoveringRect(
		{ trackTopLeft.x + segmentX - 10, trackTopLeft.y - 10 }, // top left
		{ trackTopLeft.x + segmentX + 10, trackTopLeft.y + trackHeight + 10 }))  // bottom right 
		{
			// draw handler of segment duration
			drawList->AddLine(
			{ trackTopLeft.x + segmentX, trackTopLeft.y }, // top left
			{ trackTopLeft.x + segmentX, trackTopLeft.y + trackHeight }, // bottom right
				guicolors::white, // color
				3.0f); // thickness

						// we are hovering this segment with the mouse
			mState.currentAction = HOVERING_SEGMENT;
			mState.currentObjectID = segment.mID;

			// left mouse is start dragging
			if (ImGui::IsMouseDown(0))
			{
				mState.currentAction = SequenceGUIMouseActions::DRAGGING_SEGMENT;
				mState.currentObjectID = segment.mID;
			}
			// right mouse in deletion popup
			else if (ImGui::IsMouseDown(1))
			{
				std::unique_ptr<SequenceGUIDeleteSegmentData> deleteSegmentData = std::make_unique<SequenceGUIDeleteSegmentData>(track.mID, segment.mID);
				mState.currentAction = SequenceGUIMouseActions::OPEN_DELETE_SEGMENT_POPUP;
				mState.currentObjectID = segment.mID;
				mState.currentActionData = std::move(deleteSegmentData);
			}
		}
		else if (
			mState.currentAction == SequenceGUIMouseActions::DRAGGING_SEGMENT &&
			mState.currentObjectID == segment.mID)
		{
			// draw handler of segment duration
			drawList->AddLine(
			{ trackTopLeft.x + segmentX, trackTopLeft.y }, // top left
			{ trackTopLeft.x + segmentX, trackTopLeft.y + trackHeight }, // bottom right
				guicolors::white, // color
				3.0f); // thickness

			// do we have the mouse still held down ? drag the segment
			if (ImGui::IsMouseDown(0))
			{
				float amount = mouseDelta.x / stepSize;
				mController.segmentDurationChange(segment.mID, amount);
				mCurveCache.clear();
			}
			// otherwise... release!
			else if (ImGui::IsMouseReleased(0))
			{
				mState.currentAction = SequenceGUIMouseActions::NONE;
			}
		}
		else
		{
			// draw handler of segment duration
			drawList->AddLine(
			{ trackTopLeft.x + segmentX, trackTopLeft.y }, // top left
			{ trackTopLeft.x + segmentX, trackTopLeft.y + trackHeight }, // bottom right
				guicolors::white, // color
				1.0f); // thickness

			// release if we are not hovering this segment
			if (mState.currentAction == HOVERING_SEGMENT
				&& mState.currentObjectID == segment.mID)
			{
				mState.currentAction = NONE;
			}
		}
		
	}

	void SequenceEditorGUIView::drawTanHandlerNum(
		const bool isWindowFocused,
		const SequenceTrack &track,
		const SequenceTrackSegment &segment,
		std::ostringstream &stringStream,
		const float segmentWidth,
		const math::FCurvePoint<float, float> &curvePoint,
		const float trackHeight,
		const ImVec2 &circlePoint,
		const int controlPointIndex,
		const TanPointTypes type,
		const ImVec2& mouseDelta,
		const int stepSize,
		ImDrawList* drawList)
	{
		// draw tan handlers
		{
			// create a string stream to create identifier of this object
			std::ostringstream tanStream;
			tanStream << stringStream.str() << (type == TanPointTypes::IN) ? "inTan" : "outTan";

			//
			const math::FComplex<float, float>& tanComplex
				= (type == TanPointTypes::IN) ? curvePoint.mInTan : curvePoint.mOutTan;

			// get the offset from the tan
			ImVec2 offset =
			{ (segmentWidth * tanComplex.mTime) / (float)segment.mDuration,
				(trackHeight *  (float) tanComplex.mValue * -1.0f) };
			ImVec2 tanPoint = { circlePoint.x + offset.x, circlePoint.y + offset.y };

			// set if we are hoverting this point with the mouse
			bool tanPointHovered = false;

			if (isWindowFocused)
			{
				// check if hovered
				if ((mState.currentAction == NONE ||
					mState.currentAction == HOVERING_CURVE_NUM)
					&& ImGui::IsMouseHoveringRect(
				{ tanPoint.x - 5, tanPoint.y - 5 },
				{ tanPoint.x + 5, tanPoint.y + 5 }))
				{
					mState.currentAction = HOVERING_TAN_POINT_NUM;
					mState.currentObjectID = tanStream.str();
					tanPointHovered = true;
				}
				else if (
					mState.currentAction == HOVERING_TAN_POINT_NUM)
				{
					// if we hare already hovering, check if its this point
					if (mState.currentObjectID == tanStream.str())
					{
						if (ImGui::IsMouseHoveringRect(
						{ tanPoint.x - 5, tanPoint.y - 5 },
						{ tanPoint.x + 5, tanPoint.y + 5 }))
						{
							// still hovering
							tanPointHovered = true;

							// start dragging if mouse down
							if (ImGui::IsMouseDown(0) && mState.currentAction == HOVERING_TAN_POINT_NUM)
							{
								mState.currentAction = DRAGGING_TAN_POINT_NUM;

								mState.currentActionData = std::make_unique<SequenceGUIDragTanPointDataNum>(
									track.mID,
									segment.mID,
									controlPointIndex,
									type);
							}
						}
						else
						{
							// otherwise, release!
							mState.currentAction = NONE;
						}
					}
				}

				// handle dragging of tan point
				if (mState.currentAction == DRAGGING_TAN_POINT_NUM)
				{
					const SequenceGUIDragTanPointDataNum* data = dynamic_cast<SequenceGUIDragTanPointDataNum*>(mState.currentActionData.get());
					if (data->segmentID == segment.mID && data->controlPointIndex == controlPointIndex && data->type == type)
					{
						if (ImGui::IsMouseReleased(0))
						{
							mState.currentAction = NONE;
							mState.currentActionData = nullptr;
						}
						else
						{
							tanPointHovered = true;

							float time = mouseDelta.x / stepSize;
							float value = (mouseDelta.y / trackHeight) * -1.0f ;

							mController.changeTanPointNum(
								track.mID,
								segment.mID,
								controlPointIndex,
								type,
								time,
								value);
							mCurveCache.clear();
						}
					}
				}
			}

			// draw line
			drawList->AddLine(circlePoint, tanPoint, tanPointHovered ? guicolors::white : guicolors::darkGrey, 1.0f);

			// draw handler
			drawList->AddCircleFilled(tanPoint, 3.0f, tanPointHovered ? guicolors::white : guicolors::darkGrey);
		}
	}

	template<typename T>
	void SequenceEditorGUIView::drawTanHandlerVec(
		const bool isWindowFocused,
		const SequenceTrack &track,
		const SequenceTrackSegment &segment,
		std::ostringstream &stringStream,
		const float segmentWidth,
		const math::FCurvePoint<float, float> &curvePoint,
		const float trackHeight,
		const ImVec2 &circlePoint,
		const int controlPointIndex,
		const int curveIndex,
		const TanPointTypes type,
		const ImVec2& mouseDelta,
		const int stepSize,
		ImDrawList* drawList)
	{
		// draw tan handlers
		{
			// create a string stream to create identifier of this object
			std::ostringstream tanStream;
			tanStream << stringStream.str() << (type == TanPointTypes::IN) ? "inTan" : "outTan";

			//
			const math::FComplex<float, float>& tanComplex
				= (type == TanPointTypes::IN) ? curvePoint.mInTan : curvePoint.mOutTan;

			// get the offset from the tan
			ImVec2 offset =
			{ (segmentWidth * tanComplex.mTime) / (float)segment.mDuration,
				(trackHeight *  (float)tanComplex.mValue * -1.0f) };
			ImVec2 tanPoint = { circlePoint.x + offset.x, circlePoint.y + offset.y };

			// set if we are hoverting this point with the mouse
			bool tanPointHovered = false;

			if (isWindowFocused)
			{
				// check if hovered
				if ((mState.currentAction == NONE ||
					mState.currentAction == HOVERING_CURVE_VEC)
					&& ImGui::IsMouseHoveringRect(
				{ tanPoint.x - 5, tanPoint.y - 5 },
				{ tanPoint.x + 5, tanPoint.y + 5 }))
				{
					mState.currentAction = HOVERING_TAN_POINT_VEC;
					mState.currentObjectID = tanStream.str();
					tanPointHovered = true;
				}
				else if (
					mState.currentAction == HOVERING_TAN_POINT_VEC)
				{
					// if we hare already hovering, check if its this point
					if (mState.currentObjectID == tanStream.str())
					{
						if (ImGui::IsMouseHoveringRect(
						{ tanPoint.x - 5, tanPoint.y - 5 },
						{ tanPoint.x + 5, tanPoint.y + 5 }))
						{
							// still hovering
							tanPointHovered = true;

							// start dragging if mouse down
							if (ImGui::IsMouseDown(0) && mState.currentAction == HOVERING_TAN_POINT_VEC)
							{
								mState.currentAction = DRAGGING_TAN_POINT_VEC;

								mState.currentActionData = std::make_unique<SequenceGUIDragTanPointDataVec>(
									track.mID,
									segment.mID,
									controlPointIndex,
									curveIndex,
									type);
							}
						}
						else
						{
							// otherwise, release!
							mState.currentAction = NONE;
						}
					}
				}

				// handle dragging of tan point
				if (mState.currentAction == DRAGGING_TAN_POINT_VEC)
				{
					const SequenceGUIDragTanPointDataVec* data =
						dynamic_cast<SequenceGUIDragTanPointDataVec*>(mState.currentActionData.get());
					if (data->segmentID == segment.mID && 
						data->controlPointIndex == controlPointIndex &&
						data->type == type &&
						data->curveIndex == curveIndex)
					{
						if (ImGui::IsMouseReleased(0))
						{
							mState.currentAction = NONE;
							mState.currentActionData = nullptr;
						}
						else
						{
							tanPointHovered = true;

							float time = mouseDelta.x / stepSize;
							float value = (mouseDelta.y / trackHeight) * -1.0f;

							mController.changeTanPointVec<T>(
								track.mID,
								segment.mID,
								controlPointIndex,
								curveIndex,
								type,
								time,
								value);
							mCurveCache.clear();
						}
					}
				}
			}

			// draw line
			drawList->AddLine(circlePoint, tanPoint, tanPointHovered ? guicolors::white : guicolors::darkGrey, 1.0f);

			// draw handler
			drawList->AddCircleFilled(tanPoint, 3.0f, tanPointHovered ? guicolors::white : guicolors::darkGrey);
		}
	}


	void SequenceEditorGUIView::handleInsertSegmentPopup()
	{
		if (mState.currentAction == OPEN_INSERT_SEGMENT_POPUP)
		{
			// invoke insert sequence popup
			ImGui::OpenPopup("Insert Segment");

			mState.currentAction = INSERTING_SEGMENT;
		}

		// handle insert segment popup
		if (mState.currentAction == INSERTING_SEGMENT)
		{
			if (ImGui::BeginPopup("Insert Segment"))
			{
				if (ImGui::Button("Insert"))
				{
					const SequenceGUIInsertSegmentData* data = dynamic_cast<SequenceGUIInsertSegmentData*>(mState.currentActionData.get());
					switch (data->trackType)
					{
					case SequenceTrackTypes::NUMERIC:
						mController.insertSegmentNumeric(data->trackID, data->time);
						break;
					case SequenceTrackTypes::VEC3:
						mController.insertSegmentVec<glm::vec3>(data->trackID, data->time);
						break;
					case SequenceTrackTypes::VEC2:
						mController.insertSegmentVec<glm::vec2>(data->trackID, data->time);
						break;
					}
					
					mCurveCache.clear();

					ImGui::CloseCurrentPopup();
					mState.currentAction = SequenceGUIMouseActions::NONE;
					mState.currentActionData = nullptr;
				}

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
					mState.currentAction = SequenceGUIMouseActions::NONE;
					mState.currentActionData = nullptr;
				}

				ImGui::EndPopup();
			}
			else
			{
				// click outside popup so cancel action
				mState.currentAction = SequenceGUIMouseActions::NONE;
				mState.currentActionData = nullptr;
			}
		}
	}


	void SequenceEditorGUIView::handleDeleteSegmentPopup()
	{
		if (mState.currentAction == OPEN_DELETE_SEGMENT_POPUP)
		{
			// invoke insert sequence popup
			ImGui::OpenPopup("Delete Segment");

			mState.currentAction = DELETING_SEGMENT;
		}

		// handle delete segment popup
		if (mState.currentAction == DELETING_SEGMENT)
		{
			if (ImGui::BeginPopup("Delete Segment"))
			{
				if (ImGui::Button("Delete"))
				{
					const SequenceGUIDeleteSegmentData* data = dynamic_cast<SequenceGUIDeleteSegmentData*>(mState.currentActionData.get());
					mController.deleteSegment(data->trackID, data->segmentID);
					mCurveCache.clear();

					ImGui::CloseCurrentPopup();
					mState.currentAction = SequenceGUIMouseActions::NONE;
					mState.currentActionData = nullptr;
				}

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
					mState.currentAction = SequenceGUIMouseActions::NONE;
					mState.currentActionData = nullptr;
				}

				ImGui::EndPopup();
			}
			else
			{
				// click outside popup so cancel action
				mState.currentAction = SequenceGUIMouseActions::NONE;
				mState.currentActionData = nullptr;
			}
		}
	}


	void SequenceEditorGUIView::drawPlayerController(
		const bool isWindowFocused,
		SequencePlayer& player,
		const float startOffsetX,
		const float timelineWidth, 
		const ImVec2 &mouseDelta)
	{
		const float timelineControllerHeight = 30.0f;

		std::ostringstream stringStream;
		stringStream << mID << "timelinecontroller";
		std::string idString = stringStream.str();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startOffsetX);
		ImGui::PushID(idString.c_str());

		// used for culling ( is stuff inside the parent window ??? )
		ImVec2 parentWindowPos = ImGui::GetWindowPos();
		ImVec2 parentWindowSize = ImGui::GetWindowSize();

		// draw timeline controller
		if (ImGui::BeginChild(
			idString.c_str(), // id
			{ timelineWidth + 5 , timelineControllerHeight }, // size
			false, // no border
			ImGuiWindowFlags_NoMove)) // window flags
		{
			ImVec2 cursorPos = ImGui::GetCursorPos();
			ImVec2 windowTopLeft = ImGui::GetWindowPos();
			ImVec2 startPos =
			{
				windowTopLeft.x + cursorPos.x,
				windowTopLeft.y + cursorPos.y + 15,
			};

			cursorPos.y += 5;

			// get window drawlist
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// draw backgroundbox of controller
			drawList->AddRectFilled(
				startPos,
				{
					startPos.x + timelineWidth,
					startPos.y + timelineControllerHeight - 15
				}, guicolors::black);

			// draw box of controller
			drawList->AddRect(
				startPos,
				{
					startPos.x + timelineWidth,
					startPos.y + timelineControllerHeight - 15
				}, guicolors::white);

			// draw handler of player position
			const double playerTime = player.getPlayerTime();
			const ImVec2 playerTimeRectTopLeft =
			{
				startPos.x + (float)(playerTime / player.getDuration()) * timelineWidth - 5,
				startPos.y
			};
			const ImVec2 playerTimeRectBottomRight =
			{
				startPos.x + (float)(playerTime / player.getDuration()) * timelineWidth + 5,
				startPos.y + timelineControllerHeight,
			};

			drawList->AddRectFilled(
				playerTimeRectTopLeft,
				playerTimeRectBottomRight,
				guicolors::red);

			// draw timestamp text every 100 pixels
			const float timestampInterval = 100.0f;
			int steps = timelineWidth / timestampInterval;
			for (int i = 0; i < steps; i++)
			{
				ImVec2 timestampPos;
				timestampPos.x = i * timestampInterval + startPos.x;
				timestampPos.y = startPos.y - 18;

				if (timestampPos.x < parentWindowSize.x + parentWindowPos.x &&
					timestampPos.x >= parentWindowPos.x)
				{
					if (timestampPos.y >= parentWindowPos.y &&
						timestampPos.y < parentWindowSize.y + parentWindowPos.y)
					{
						double timeInPlayer = mController.getSequence().mDuration * (float)((float)i / steps);
						std::string formattedTimeString = formatTimeString(timeInPlayer);
						drawList->AddText(timestampPos, guicolors::white, formattedTimeString.c_str());

						if (i != 0)
						{
							drawList->AddLine(
							{ timestampPos.x, timestampPos.y + 18 },
							{ timestampPos.x, timestampPos.y + timelineControllerHeight + 2 }, guicolors::darkGrey);
						}
					}
				}
			}

			if (isWindowFocused)
			{
				if (mState.currentAction == NONE || mState.currentAction == HOVERING_PLAYER_TIME)
				{
					if (ImGui::IsMouseHoveringRect(startPos, 
					{
						startPos.x + timelineWidth,
						startPos.y + timelineControllerHeight
					}))
					{
						mState.currentAction = HOVERING_PLAYER_TIME;

						if (ImGui::IsMouseDown(0))
						{
							//
							bool playerWasPlaying = player.getIsPlaying();
							bool playerWasPaused = player.getIsPaused();

							mState.currentAction = DRAGGING_PLAYER_TIME;
							mState.currentActionData = std::make_unique<SequenceGUIDragPlayerData>(
								playerWasPlaying,
								playerWasPaused
								);

							if (playerWasPlaying)
							{
								player.pause();
							}
							
							// snap to mouse position
							double time = ((ImGui::GetMousePos().x - startPos.x) / timelineWidth) * player.getDuration();
							player.setPlayerTime(time);
						}
					}
					else
					{
						
						mState.currentAction = NONE;
					}
				}else if (mState.currentAction == DRAGGING_PLAYER_TIME)
				{
					if (ImGui::IsMouseDown(0))
					{
						double delta = (mouseDelta.x / timelineWidth) * player.getDuration();
						player.setPlayerTime(playerTime + delta);
					}
					else
					{
						if (ImGui::IsMouseReleased(0))
						{
							const SequenceGUIDragPlayerData* data = dynamic_cast<SequenceGUIDragPlayerData*>( mState.currentActionData.get() );
							if (data->playerWasPlaying && !data->playerWasPaused)
							{
								player.play();
							}

							mState.currentAction = NONE;
						}
					}
				}
			}
		}

		ImGui::EndChild();

		ImGui::PopID();
	}


	void SequenceEditorGUIView::drawTimelinePlayerPosition(
		const Sequence& sequence,
		SequencePlayer& player,
		const ImVec2 &timelineControllerWindowPosition, 
		const float trackInspectorWidth,
		const float timelineWidth)
	{
		std::ostringstream stringStream;
		stringStream << mID << "timelineplayerposition";
		std::string idString = stringStream.str();

		// store cursorpos
		ImVec2 cursorPos = ImGui::GetCursorPos();

		ImGui::SetCursorPos(
		{
			timelineControllerWindowPosition.x 
				+ trackInspectorWidth + 5 
				+ timelineWidth * (float)(player.getPlayerTime() / player.getDuration()) - 1,
			timelineControllerWindowPosition.y + 15.0f
		});

		ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, guicolors::red);
		if (ImGui::BeginChild(
			idString.c_str(), // id
			{ 1.0f, sequence.mTracks.size() * ( mVerticalResolution + 10.0f ) + 10.0f }, // size
			false, // no border
			ImGuiWindowFlags_NoMove)) // window flags
		{

			
		}
		ImGui::End();
		ImGui::PopStyleColor();

		// pop cursorpos
		ImGui::SetCursorPos(cursorPos);
	}


	void SequenceEditorGUIView::handleLoadPopup()
	{
		if (mState.currentAction == LOAD)
		{
			//
			if (ImGui::BeginPopupModal(
				"Load",
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize))
			{
				//
				SequenceGUILoadShowData* data = dynamic_cast<SequenceGUILoadShowData*>(mState.currentActionData.get());

				//
				const std::string showDir = "sequences/";

				// Find all files in the preset directory
				std::vector<std::string> files_in_directory;
				utility::listDir(showDir.c_str(), files_in_directory);

				std::vector<std::string> shows;
				std::vector<std::string> showFiles;
				for (const auto& filename : files_in_directory)
				{
					// Ignore directories
					if (utility::dirExists(filename))
						continue;

					if (utility::getFileExtension(filename) == "json")
					{
						shows.emplace_back(utility::getFileName(filename));
						showFiles.emplace_back(filename);
					}
				}

				int index = 0;
				Combo("Sequences",
					&data->selectedShow,
					shows);
					
				utility::ErrorState errorState;
				if (ImGui::Button("Load"))
				{
					if (mController.getSequencePlayer().load(
						showFiles[data->selectedShow], errorState))
					{
						mState.currentAction = NONE;
						mState.currentActionData = nullptr;
						mCurveCache.clear();
						ImGui::CloseCurrentPopup();
					}
					else
					{
						ImGui::OpenPopup("Error");
						data->errorString = errorState.toString();
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					mState.currentAction = NONE;
					mState.currentActionData = nullptr;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text(data->errorString.c_str());
					if (ImGui::Button("OK"))
					{
						mState.currentAction = LOAD;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::EndPopup();
			}
		}
	}


	void SequenceEditorGUIView::handleSaveAsPopup()
	{
		if (mState.currentAction == SAVE_AS)
		{
			// save as popup
			if (ImGui::BeginPopupModal(
				"Save As",
				nullptr,
				ImGuiWindowFlags_AlwaysAutoResize))
			{
				//
				SequenceGUISaveShowData* data =
					dynamic_cast<SequenceGUISaveShowData*>(mState.currentActionData.get());

				const std::string showDir = "sequences";

				// Find all files in the preset directory
				std::vector<std::string> files_in_directory;
				utility::listDir(showDir.c_str(), files_in_directory);

				std::vector<std::string> shows;
				for (const auto& filename : files_in_directory)
				{
					// Ignore directories
					if (utility::dirExists(filename))
						continue;

					if (utility::getFileExtension(filename) == "json")
					{
						shows.push_back(utility::getFileName(filename));
					}
				}
				shows.push_back("<New...>");

				if (Combo("Shows",
					&data->selectedShow,
					shows))
				{
					if (data->selectedShow == shows.size() - 1)
					{
						ImGui::OpenPopup("New");
					}
					else
					{
						ImGui::OpenPopup("Overwrite");
					}
				}

				// new show popup
				std::string newShowFileName;
				bool done = false;
				if (ImGui::BeginPopupModal("New", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					static char name[256] = { 0 };
					ImGui::InputText("Name", name, 256);

					if (ImGui::Button("OK") && strlen(name) != 0)
					{
						newShowFileName = std::string(name, strlen(name));
						newShowFileName += ".json";

						ImGui::CloseCurrentPopup();
						done = true;
					}

					ImGui::SameLine();

					if (ImGui::Button("Cancel"))
						ImGui::CloseCurrentPopup();

					ImGui::EndPopup();
				}
				if (done)
				{
					// Insert before the '<new...>' item
					shows.insert(shows.end() - 1, newShowFileName);

					utility::ErrorState errorState;
					if (mController.getSequencePlayer().save(showDir + "/" + newShowFileName, errorState))
					{
						data->selectedShow = shows.size() - 2;
					}
					else
					{
						data->errorString = errorState.toString();
						ImGui::OpenPopup("Error");
					}
				}

				if (ImGui::BeginPopupModal("Overwrite"))
				{
					utility::ErrorState errorState;
					ImGui::Text(("Are you sure you want to overwrite " + 
						shows[data->selectedShow] + " ?").c_str());
					if (ImGui::Button("OK"))
					{
						if (mController.getSequencePlayer().save(
							shows[data->selectedShow],
							errorState))
						{
						}
						else
						{
							data->errorString = errorState.toString();
							ImGui::OpenPopup("Error");
						}

						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginPopupModal("Error"))
				{
					ImGui::Text(data->errorString.c_str());
					if (ImGui::Button("OK"))
					{
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button("Done"))
				{
					mState.currentAction = NONE;
					mState.currentActionData = nullptr;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
	}


	void SequenceEditorGUIView::drawSegmentContentNumeric(
		const bool isWindowFocused,
		const SequenceTrack &track,
		const SequenceTrackSegment &segment,
		ImVec2 trackTopLeft,
		float previousSegmentX,
		float segmentWidth,
		const float trackHeight,
		float segmentX,
		const float stepSize,
		ImDrawList* drawList,
		const ImVec2 & mouseDelta,
		bool drawStartValue)
	{
		// curve
		drawCurveNumeric(
			isWindowFocused,
			track,
			segment,
			trackTopLeft,
			previousSegmentX,
			segmentWidth,
			trackHeight,
			segmentX,
			stepSize,
			drawList);

		// draw control points
		drawControlPointsNumeric(
			isWindowFocused,
			track,
			segment,
			trackTopLeft,
			segmentX,
			segmentWidth,
			trackHeight,
			mouseDelta,
			stepSize,
			drawList);

		// if this is the first segment of the track
		// also draw a handler for the start value
		if (drawStartValue)
		{
			// draw segment value handler
			drawSegmentValueNum(
				isWindowFocused,
				track,
				segment,
				trackTopLeft,
				segmentX,
				segmentWidth,
				trackHeight,
				mouseDelta,
				stepSize,
				SegmentValueTypes::BEGIN,
				drawList);
		}

		// draw segment value handler
		drawSegmentValueNum(
			isWindowFocused,
			track,
			segment,
			trackTopLeft,
			segmentX,
			segmentWidth,
			trackHeight,
			mouseDelta,
			stepSize,
			SegmentValueTypes::END,
			drawList);
	}

	template<typename T>
	void SequenceEditorGUIView::drawCurveVec(
		const bool isWindowFocused,
		const SequenceTrack& track,
		const SequenceTrackSegment& segment_,
		const ImVec2 &trackTopLeft,
		const float previousSegmentX,
		const float segmentWidth,
		const float trackHeight,
		const float segmentX,
		const float stepSize,
		ImDrawList* drawList)
	{
		const SequenceTrackSegmentVec<T>& segment = segment_.getDerivedConst<SequenceTrackSegmentVec<T>>();

		const int resolution = 40;
		bool curveSelected = false;

		if (mCurveCache.find(segment.mID) == mCurveCache.end())
		{
			std::vector<ImVec2> points;
			points.resize((resolution + 1)*segment.mCurves.size());
			for (int v = 0; v < segment.mCurves.size(); v++)
			{
				for (int i = 0; i <= resolution; i++)
				{
					float value = 1.0f - segment.mCurves[v]->evaluate((float)i / resolution);

					points[i + v * (resolution+1)] =
					{
						trackTopLeft.x + previousSegmentX + segmentWidth * ((float)i / resolution),
						trackTopLeft.y + value * trackHeight
					};
				}
			}
			mCurveCache.emplace(segment.mID, points);
		}

		int selectedCurve = -1;
		if (isWindowFocused)
		{
			// determine if mouse is hovering curve
			if ((mState.currentAction == NONE || 
				 mState.currentAction == HOVERING_CURVE_VEC)
					&& ImGui::IsMouseHoveringRect(
					{ trackTopLeft.x + segmentX - segmentWidth, trackTopLeft.y }, // top left
					{ trackTopLeft.x + segmentX, trackTopLeft.y + trackHeight }))  // bottom right 
			{
				// translate mouse position to position in curve
				ImVec2 mousePos = ImGui::GetMousePos();
				float xInSegment = ((mousePos.x - (trackTopLeft.x + segmentX - segmentWidth)) / stepSize) / segment.mDuration;
				float yInSegment = 1.0f - ((mousePos.y - trackTopLeft.y) / trackHeight);

				for (int i = 0; i < segment.mCurves.size(); i++)
				{
					// evaluate curve at x position
					float yInCurve = segment.mCurves[i]->evaluate(xInSegment);

					// insert curve point on click
					const float maxDist = 0.1f;
					if (abs(yInCurve - yInSegment) < maxDist)
					{
						mState.currentAction = HOVERING_CURVE_VEC;
						mState.currentObjectID = segment.mID;
						mState.currentActionData = std::make_unique<SequenceGUIHoveringCurveVecData>(i);
						if (ImGui::IsMouseClicked(0))
						{
							mController.insertCurvePointVec<T>(
								track.mID,
								segment.mID,
								xInSegment, 
								i);
						}
						selectedCurve = i;
					}
				}
			}
			else
			{
				if (mState.currentAction == HOVERING_CURVE_VEC &&
					mState.currentObjectID == segment.mID)
				{
					mState.currentAction = NONE;
					mState.currentActionData = nullptr;
				}
			}
		}

		for (int i = 0; i < segment.mCurves.size(); i++)
		{
			// draw points of curve
			drawList->AddPolyline(
				&*mCurveCache[segment.mID].begin() + i * ( resolution + 1 ), // points array
				mCurveCache[segment.mID].size() / segment.mCurves.size(), // size of points array
				guicolors::red, // color
				false, // closed
				selectedCurve == i ? 3.0f : 1.0f, // thickness
				true); // anti-aliased
		}
	}

	template<typename T>
	void SequenceEditorGUIView::drawSegmentContentVec(
		const bool isWindowFocused,
		const SequenceTrack &track,
		const SequenceTrackSegment &segment,
		ImVec2 trackTopLeft,
		float previousSegmentX,
		float segmentWidth,
		const float trackHeight,
		float segmentX,
		const float stepSize,
		ImDrawList* drawList,
		const ImVec2 & mouseDelta,
		bool drawStartValue)
	{
		// curve
		drawCurveVec<T>(
			isWindowFocused,
			track,
			segment,
			trackTopLeft,
			previousSegmentX,
			segmentWidth,
			trackHeight,
			segmentX,
			stepSize,
			drawList);
			

		// draw control points
		drawControlPointsVec<T>(
			isWindowFocused,
			track,
			segment,
			trackTopLeft,
			segmentX,
			segmentWidth,
			trackHeight,
			mouseDelta,
			stepSize,
			drawList);

		
		// if this is the first segment of the track
		// also draw a handler for the start value
		if (drawStartValue)
		{
			// draw segment value handler
			drawSegmentValueVec<T>(
				isWindowFocused,
				track,
				segment,
				trackTopLeft,
				segmentX,
				segmentWidth,
				trackHeight,
				mouseDelta,
				stepSize,
				SegmentValueTypes::BEGIN,
				drawList);
		}

		// draw segment value handler
		drawSegmentValueVec<T>(
			isWindowFocused,
			track,
			segment,
			trackTopLeft,
			segmentX,
			segmentWidth,
			trackHeight,
			mouseDelta,
			stepSize,
			SegmentValueTypes::END,
			drawList);
	}


	std::string SequenceEditorGUIView::formatTimeString(double time)
	{
		int hours = time / 3600.0f;
		int minutes = (int)(time / 60.0f) % 60;
		int seconds = (int)time % 60;
		int milliseconds = (int)(time * 100.0f) % 100;

		std::stringstream stringStream;

		stringStream << std::setw(2) << std::setfill('0') << seconds;
		std::string secondsString = stringStream.str();

		stringStream = std::stringstream();
		stringStream << std::setw(2) << std::setfill('0') << minutes;
		std::string minutesString = stringStream.str();

		stringStream = std::stringstream();
		stringStream << std::setw(2) << std::setfill('0') << milliseconds;
		std::string millisecondsStrings = stringStream.str();

		std::string hoursString = "";
		if (hours > 0)
		{
			stringStream = std::stringstream();
			stringStream << std::setw(2) << std::setfill('0') << hours;
			hoursString = stringStream.str() + ":";
		}

		return hoursString + minutesString + ":" + secondsString + ":" + millisecondsStrings;
	}

	static bool vector_getter(void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool SequenceEditorGUIView::Combo(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return ImGui::Combo(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}

	bool SequenceEditorGUIView::ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return ImGui::ListBox(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}


	SequenceEditorView::SequenceEditorView(SequenceEditorController& controller)
		: mController(controller) {}
}