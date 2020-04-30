// local includes
#include "sequenceeditor.h"
#include "sequencetrack.h"
#include "sequencetracksegment.h"
#include "sequenceutils.h"

// external includes
#include <nap/logger.h>
#include <fcurve.h>
#include <functional>

RTTI_BEGIN_CLASS(nap::SequenceEditor)
RTTI_PROPERTY("Sequence Player", &nap::SequenceEditor::mSequencePlayer, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

using namespace nap::SequenceEditorTypes;

namespace nap
{
	std::unordered_map<rttr::type, std::function<void(nap::SequenceEditorController&,nap::SequenceTrack&)>> SequenceEditorController::sUpdateSegmentsMap
		{
			{ RTTI_OF(SequenceTrackCurveFloat),  [](nap::SequenceEditorController& controller, nap::SequenceTrack& track){
				 controller.updateCurveSegments<float>(track); } },
			{ RTTI_OF(SequenceTrackCurveVec2),  [](nap::SequenceEditorController& controller, nap::SequenceTrack& track){
				 controller.updateCurveSegments<glm::vec2>(track); } },
			{ RTTI_OF(SequenceTrackCurveVec3),  [](nap::SequenceEditorController& controller, nap::SequenceTrack& track){
				 controller.updateCurveSegments<glm::vec3>(track); } },
			{ RTTI_OF(SequenceTrackCurveVec4),  [](nap::SequenceEditorController& controller, nap::SequenceTrack& track){
				 controller.updateCurveSegments<glm::vec4>(track); } },
			{ RTTI_OF(SequenceTrackEvent),  [](nap::SequenceEditorController& controller, nap::SequenceTrack& track){ } }
		};

	bool SequenceEditor::init(utility::ErrorState& errorState)
	{
		if (!Resource::init(errorState))
		{
			return false;
		}

		mController = std::make_unique<SequenceEditorController>(
			*mSequencePlayer.get());

		return true;
	}


	void SequenceEditorController::segmentDurationChange(
		const std::string& trackID,
		const std::string& segmentID,
		float amount)
	{
		// lock
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		//
		Sequence& sequence = mSequencePlayer.getSequence();

		//
		SequenceTrack* track = findTrack(trackID);
		assert(track != nullptr);

		ResourcePtr<SequenceTrackSegment> previousSegment = nullptr;
		for (auto trackSegment : track->mSegments)
		{
			if (trackSegment->mID == segmentID)
			{
				// check if new duration is valid
				bool valid = true;
				double newDuration = trackSegment->mDuration + amount;

				if (newDuration > 0.0)
				{
					if (previousSegment != nullptr)
					{
						if (trackSegment->mStartTime + newDuration < previousSegment->mStartTime + previousSegment->mDuration)
						{
							valid = false;
						}
					}
				}
				else
				{
					valid = false;
				}

				if (valid)
				{
					trackSegment->mDuration += amount;

					sUpdateSegmentsMap[track->get_type()](*this, *track);

					updateTracks();
				}
				break;
			}

			previousSegment = trackSegment;
		}
	}

	void SequenceEditorController::segmentEventStartTimeChange(
		const std::string& trackID,
		const std::string& segmentID,
		float amount)
	{
		// pause player thread
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		auto* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);
		assert(segment->get_type().is_derived_from(RTTI_OF(SequenceTrackSegmentEvent)));

		auto& segmentEvent = static_cast<SequenceTrackSegmentEvent&>(*segment);
		segmentEvent.mStartTime += amount;

		updateTracks();
	}


	const Sequence& SequenceEditor::getSequence() const
	{
		return mSequencePlayer->getSequence();
	}


	SequenceEditorController& SequenceEditor::getController()
	{
		return *mController.get();
	}



	void SequenceEditorController::save()
	{
		// save
		utility::ErrorState errorState;
		if (!errorState.check(mSequencePlayer.save(mSequencePlayer.mDefaultSequence, errorState), "Error saving show!"))
		{
			nap::Logger::error(errorState.toString());
		}
	}


	void SequenceEditorController::deleteSegment(const std::string& trackID, const std::string& segmentID)
	{
		// pause player thread
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		//
		Sequence& sequence = mSequencePlayer.getSequence();

		for (auto& track : sequence.mTracks)
		{
			if (track->mID == trackID)
			{
				int segmentIndex = 0;
				for (auto& segment : track->mSegments)
				{
					if (segment->mID == segmentID)
					{
						// store the duration of the segment that we are deleting
						double duration = segment->mDuration;

						// erase it from the list
						track->mSegments.erase(track->mSegments.begin() + segmentIndex);
						
						// get the segment that is now at the previous deleted segments position
						if (track->mSegments.begin() + segmentIndex != track->mSegments.end())
						{
							// add the duration
							track->mSegments[segmentIndex]->mDuration += duration;
						}

						deleteObjectFromSequencePlayer(segmentID);

						// update segments
						sUpdateSegmentsMap[track->get_type()](*this, *track);

						break;
					}
					updateTracks();
					segmentIndex++;
				}

				break;
			}
		}
	}


	void SequenceEditorController::updateTracks()
	{
		double longestTrackDuration = 0.0;
		for (auto& track : mSequencePlayer.mSequence->mTracks)
		{
			double trackDuration = 0.0;
			double highestSegment = 0.0;

			for(const auto& segment : track->mSegments)
			{
				if (segment->mStartTime + segment->mDuration > highestSegment)
				{
					highestSegment = segment->mStartTime + segment->mDuration;
					trackDuration = highestSegment;
				}
			}

			if (trackDuration > longestTrackDuration)
			{
				longestTrackDuration = trackDuration;
			}
		}

		for (auto& track : mSequencePlayer.mSequence->mTracks)
		{
			mSequencePlayer.mSequence->mDuration = longestTrackDuration;
		}
	}

	SequenceTrackSegment* SequenceEditorController::findSegment(const std::string& trackID, const std::string& segmentID)
	{
		//
		Sequence& sequence = mSequencePlayer.getSequence();

		for (auto& track : sequence.mTracks)
		{
			if (track->mID == trackID)
			{
				for (auto& segment : track->mSegments)
				{
					if (segment->mID == segmentID)
					{
						return segment.get();
					}
				}
			}
		}

		return nullptr;
	}

	const SequenceTrackSegment* SequenceEditorController::getSegment(const std::string& trackID, const std::string& segmentID) const
	{
		//
		const Sequence& sequence = mSequencePlayer.getSequenceConst();

		for (const auto& track : sequence.mTracks)
		{
			if (track->mID == trackID)
			{
				for (auto& segment : track->mSegments)
				{
					if (segment->mID == segmentID)
					{
						return segment.get();
					}
				}
			}
		}

		return nullptr;
	}


	SequenceTrack* SequenceEditorController::findTrack(const std::string& trackID)
	{
		//
		Sequence& sequence = mSequencePlayer.getSequence();

		for (auto& track : sequence.mTracks)
		{
			if (track->mID == trackID)
			{
				return track.get();
			}
		}

		return nullptr;
	}


	void SequenceEditorController::assignNewObjectID(
		const std::string& trackID,
		const std::string& objectID)
	{
		SequenceTrack* track = findTrack(trackID);
		assert(track != nullptr);

		std::unique_lock<std::mutex> l = mSequencePlayer.lock();

		if (mSequencePlayer.createAdapter(objectID, trackID, l))
		{
			track->mAssignedObjectIDs = objectID;
		}
	}


	void SequenceEditorController::deleteTrack(const std::string& deleteTrackID)
	{
		auto lock = mSequencePlayer.lock();

		//
		Sequence& sequence = mSequencePlayer.getSequence();

		int index = 0;
		for (const auto& track : sequence.mTracks)
		{
			if (track->mID == deleteTrackID)
			{
				if (mSequencePlayer.mAdapters.find(track->mID) != mSequencePlayer.mAdapters.end())
				{
					mSequencePlayer.mAdapters.erase(track->mID);
				}

				sequence.mTracks.erase(sequence.mTracks.begin() + index);

				deleteObjectFromSequencePlayer(deleteTrackID);

				break;
			}
			index++;
		}
	}


	SequencePlayer& SequenceEditorController::getSequencePlayer() const
	{
		return mSequencePlayer;
	}


	const Sequence& SequenceEditorController::getSequence() const
	{
		return *mSequencePlayer.mSequence;
	}


	void SequenceEditorController::deleteObjectFromSequencePlayer(const std::string& id)
	{
		if (mSequencePlayer.mReadObjectIDs.find(id) != mSequencePlayer.mReadObjectIDs.end())
		{
			mSequencePlayer.mReadObjectIDs.erase(id);
		}

		for (int i = 0; i < mSequencePlayer.mReadObjects.size(); i++)
		{
			if (mSequencePlayer.mReadObjects[i]->mID == id)
			{
				mSequencePlayer.mReadObjects.erase(mSequencePlayer.mReadObjects.begin() + i);
				break;
			}
		}
	}


	template<typename T>
	void SequenceEditorController::addNewCurveTrack()
	{
		std::unique_lock<std::mutex> l = mSequencePlayer.lock();

		Sequence& sequence = mSequencePlayer.getSequence();

		SequenceTrack* newTrack = sequenceutils::createSequenceCurveTrack<T>(
			mSequencePlayer.mReadObjects,
			mSequencePlayer.mReadObjectIDs);
		sequence.mTracks.emplace_back(ResourcePtr<SequenceTrack>(newTrack));

		updateCurveSegments<T>(*newTrack);
	}

	void SequenceEditorController::addNewEventTrack()
	{
		std::unique_lock<std::mutex> l = mSequencePlayer.lock();

		Sequence& sequence = mSequencePlayer.getSequence();

		SequenceTrack* newTrack = sequenceutils::createSequenceEventTrack(
			mSequencePlayer.mReadObjects,
			mSequencePlayer.mReadObjectIDs);

		sequence.mTracks.emplace_back(ResourcePtr<SequenceTrack>(newTrack));
	}

	/**
	*
	*/
	template<typename T>
	void SequenceEditorController::insertCurveSegment(
		const std::string& trackID,
		double time)
	{
		auto l = mSequencePlayer.lock();

		int curveCount = 0;
		if (RTTI_OF(T) == RTTI_OF(glm::vec4))
		{
			curveCount = 4;
		}
		else if (RTTI_OF(T) == RTTI_OF(glm::vec3))
		{
			curveCount = 3;
		}
		else if (RTTI_OF(T) == RTTI_OF(glm::vec2))
		{
			curveCount = 2;
		}
		else if (RTTI_OF(T) == RTTI_OF(float))
		{
			curveCount = 1;
		}

		//
		assert(curveCount > 0);

		//
		Sequence& sequence = mSequencePlayer.getSequence();

		// find the right track
		for (auto& track : sequence.mTracks)
		{
			if (track->mID == trackID)
			{
				// track found

				// find the segment the new segment in inserted after
				int segmentCount = 1;
				for (auto& segment : track->mSegments)
				{
					if (segment->mStartTime < time &&
						segment->mStartTime + segment->mDuration > time)
					{
						// segment found

						// create new segment & set parameters
						std::unique_ptr<SequenceTrackSegmentCurve<T>> newSegment = std::make_unique<SequenceTrackSegmentCurve<T>>();
						newSegment->mStartTime = time;
						newSegment->mDuration = segment->mStartTime + segment->mDuration - time;
						newSegment->mCurves.resize(curveCount);
						for (int i = 0; i < curveCount; i++)
						{
							std::unique_ptr<math::FCurve<float, float>> segmentCurve = std::make_unique<math::FCurve<float, float>>();
							segmentCurve->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);

							// assign curve
							newSegment->mCurves[i] = nap::ResourcePtr<math::FCurve<float, float>>(segmentCurve.get());

							// move ownership
							mSequencePlayer.mReadObjects.emplace_back(std::move(segmentCurve));
						}

						//
						SequenceTrackSegmentCurve<T>& segmentVec = static_cast<SequenceTrackSegmentCurve<T>&>(*segment.get());

						// set the value by evaluation curve
						newSegment->setStartValue(
							segmentVec.getValue(
							(segment->mStartTime + segment->mDuration - time) / segment->mDuration)
						);

						// check if there is a next segment
						if (segmentCount < track->mSegments.size())
						{
							// if there is a next segment, the new segments end value is the start value of the next segment ...
							SequenceTrackSegmentCurve<T>& nextSegmentFloat = static_cast<SequenceTrackSegmentCurve<T>&>(*track->mSegments[segmentCount].get());
							
							newSegment->setEndValue(nextSegmentFloat.getEndValue());
						}
						else
						{
							// ... otherwise it just gets this segments end value
							newSegment->setEndValue(segmentVec.getEndValue());
						}

						// the segment's end value gets the start value the newly inserted segment 
						segmentVec.setEndValue(newSegment->getStartValue());

						// change duration of segment before inserted segment
						segment->mDuration = newSegment->mStartTime - segment->mStartTime;

						// generate unique id
						newSegment->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);

						// wrap it in a resource ptr and insert it into the track
						ResourcePtr<SequenceTrackSegment> newSegmentResourcePtr(newSegment.get());
						track->mSegments.insert(track->mSegments.begin() + segmentCount, newSegmentResourcePtr);

						// move ownership to sequence player
						mSequencePlayer.mReadObjects.emplace_back(std::move(newSegment));

						//
						updateCurveSegments<T>(*(track.get()));

						break;
					}
					else if (segmentCount == track->mSegments.size())
					{
						// insert segment at the end of the list

						// create new segment & set parameters
						std::unique_ptr<SequenceTrackSegmentCurve<T>> newSegment =
							std::make_unique<SequenceTrackSegmentCurve<T>>();
						newSegment->mStartTime = segment->mStartTime + segment->mDuration;
						newSegment->mDuration = time - newSegment->mStartTime;
						newSegment->mCurves.resize(curveCount);
						for (int v = 0; v < curveCount; v++)
						{
							std::unique_ptr<math::FCurve<float, float>> newCurve = std::make_unique<math::FCurve<float, float>>();
							newCurve->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);
							newSegment->mCurves[v] = ResourcePtr<math::FCurve<float, float>>(newCurve.get());
							mSequencePlayer.mReadObjects.emplace_back(std::move(newCurve));
						}

						// generate unique id
						newSegment->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);

						// wrap it in a resource ptr and insert it into the track
						ResourcePtr<SequenceTrackSegment> newSegmentResourcePtr(newSegment.get());
						track->mSegments.emplace_back(newSegmentResourcePtr);

						// move ownership to sequence player
						mSequencePlayer.mReadObjects.emplace_back(std::move(newSegment));

						//
						updateCurveSegments<T>(*(track.get()));

						break;
					}


					segmentCount++;
				}

				//
				if (track->mSegments.size() == 0)
				{
					// create new segment & set parameters
					std::unique_ptr<SequenceTrackSegmentCurve<T>> newSegment = std::make_unique<SequenceTrackSegmentCurve<T>>();
					newSegment->mStartTime = 0.0;
					newSegment->mDuration = time - newSegment->mStartTime;

					// make new curve of segment
					newSegment->mCurves.resize(curveCount);
					for (int v = 0; v < curveCount; v++)
					{
						std::unique_ptr<math::FCurve<float, float>> newCurve = std::make_unique<math::FCurve<float, float>>();
						newCurve->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);
						newSegment->mCurves[v] = ResourcePtr<math::FCurve<float, float>>(newCurve.get());
						mSequencePlayer.mReadObjects.emplace_back(std::move(newCurve));
					}

					// generate unique id
					newSegment->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);

					// wrap it in a resource ptr and insert it into the track
					ResourcePtr<SequenceTrackSegment> newSegmentResourcePtr(newSegment.get());
					track->mSegments.emplace_back(newSegmentResourcePtr);

					// move ownership to sequence player
					mSequencePlayer.mReadObjects.emplace_back(std::move(newSegment));

					//
					updateCurveSegments<T>(*(track.get()));
				}
				break;
			}
		}
	}

	template<typename T>
	void SequenceEditorController::changeCurveType(
		const std::string& trackID,
		const std::string& segmentID,
		math::ECurveInterp type)
	{
		auto l = mSequencePlayer.lock();

		auto* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		auto* segmentCurve = dynamic_cast<SequenceTrackSegmentCurve<T>*>(segment);
		assert(segmentCurve != nullptr);
		
		segmentCurve->mCurveType = type;
		for (int i = 0; i < segmentCurve->mCurves.size(); i++)
		{
			for (int j = 0; j < segmentCurve->mCurves[i]->mPoints.size(); j++)
			{
				segmentCurve->mCurves[i]->mPoints[j].mInterp = type;
			}
		}
	}


	void SequenceEditorController::insertEventSegment(
		const std::string& trackID,
		double time,
		const std::string& eventMessage)
	{
		auto l = mSequencePlayer.lock();

		// create new segment & set parameters
		std::unique_ptr<SequenceTrackSegmentEvent> newSegment = std::make_unique<SequenceTrackSegmentEvent>();
		newSegment->mStartTime = time;
		newSegment->mMessage = eventMessage;
		newSegment->mID = sequenceutils::generateUniqueID(mSequencePlayer.mReadObjectIDs);
		newSegment->mDuration = 0.0;

		//
		SequenceTrack* track = findTrack(trackID);
		assert(track != nullptr);

		track->mSegments.emplace_back(ResourcePtr<SequenceTrackSegmentEvent>(newSegment.get()));

		mSequencePlayer.mReadObjects.emplace_back(std::move(newSegment));
	}

	void SequenceEditorController::editEventSegment(
		const std::string& trackID,
		const std::string& segmentID,
		const std::string& eventMessage)
	{
		auto l = mSequencePlayer.lock();

		SequenceTrack* track = findTrack(trackID);
		assert(track != nullptr);

		SequenceTrackSegment* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		SequenceTrackSegmentEvent* eventSegment = dynamic_cast<SequenceTrackSegmentEvent*>(segment);
		assert(eventSegment != nullptr);

		eventSegment->mMessage = eventMessage;
	}

	/**
	*
	*/
	template<typename T>
	void SequenceEditorController::changeCurveSegmentValue(
		const std::string& trackID,
		const std::string& segmentID,
		float amount,
		int curveIndex,
		SegmentValueTypes valueType)
	{
		//
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		SequenceTrack* track = findTrack(trackID);
		assert(track != nullptr);

		SequenceTrackSegment* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		SequenceTrackSegmentCurve<T>& segmentVec = static_cast<SequenceTrackSegmentCurve<T>&>(*segment);
		assert(curveIndex < segmentVec.mCurves.size());

		if (segment != nullptr)
		{
			switch (valueType)
			{
			case BEGIN:
			{
				segmentVec.mCurves[curveIndex]->mPoints[0].mPos.mValue += amount;
				segmentVec.mCurves[curveIndex]->mPoints[0].mPos.mValue = math::clamp<float>(segmentVec.mCurves[curveIndex]->mPoints[0].mPos.mValue, 0.0f, 1.0f);
			}
			break;
			case END:
			{
				int lastPoint = segmentVec.mCurves[curveIndex]->mPoints.size() - 1;
				segmentVec.mCurves[curveIndex]->mPoints[lastPoint].mPos.mValue += amount;
				segmentVec.mCurves[curveIndex]->mPoints[lastPoint].mPos.mValue = math::clamp<float>(segmentVec.mCurves[curveIndex]->mPoints[lastPoint].mPos.mValue, 0.0f, 1.0f);
			}
			break;
			}
		}

		//
		updateCurveSegments<T>(*track);
	}
	
	template<typename T>
	void SequenceEditorController::updateCurveSegments(SequenceTrack& track)
	{
		// update start time and duration of all segments
		ResourcePtr<SequenceTrackSegmentCurve<T>> prevSeg = nullptr;
		for (auto trackSeg : track.mSegments)
		{
			if (prevSeg == nullptr)
			{
				trackSeg->mStartTime = 0.0;
			}
			else
			{
				trackSeg->mStartTime = prevSeg->mStartTime + prevSeg->mDuration;
				prevSeg->mDuration = trackSeg->mStartTime - prevSeg->mStartTime;
			}
			prevSeg = static_cast<ResourcePtr<SequenceTrackSegmentCurve<T>>>(trackSeg);
		}

		// 
		 prevSeg = nullptr;
		for (auto trackSeg : track.mSegments)
		{
			auto& trackSegVec = static_cast<SequenceTrackSegmentCurve<T>&>(*trackSeg.get());

			if (prevSeg == nullptr)
			{
			}
			else
			{
				// if we have a previous segment, the curve gets the value of the start value of the current segment
				trackSegVec.setStartValue(prevSeg->getEndValue());
			}
			prevSeg = &trackSegVec;
		}
	}

	/**
	*
	*/
	template<typename T>
	void SequenceEditorController::insertCurvePoint(
		const std::string& trackID,
		const std::string& segmentID,
		float pos,
		int curveIndex)
	{
		//
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		// find segment
		SequenceTrackSegment* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		//
		auto& trackSegNum = static_cast<SequenceTrackSegmentCurve<T>&>(*segment);
		assert(curveIndex < trackSegNum.mCurves.size());

		// iterate trough points of curve
		for (int i = 0; i < trackSegNum.mCurves[curveIndex]->mPoints.size() - 1; i++)
		{
			// find the point the new point needs to get inserted after
			if (trackSegNum.mCurves[curveIndex]->mPoints[i].mPos.mTime <= pos
				&& trackSegNum.mCurves[curveIndex]->mPoints[i + 1].mPos.mTime > pos)
			{
				// create point
				math::FCurvePoint<float, float> p;
				p.mPos.mTime = pos;
				p.mPos.mValue = trackSegNum.mCurves[curveIndex]->evaluate(pos);
				p.mInTan.mTime = -0.1f;
				p.mOutTan.mTime = 0.1f;
				p.mInTan.mValue = 0.0f;
				p.mOutTan.mValue = 0.0f;
				p.mTangentsAligned = true;
				p.mInterp = trackSegNum.mCurveType;

				// insert point
				trackSegNum.mCurves[curveIndex]->mPoints.insert(trackSegNum.mCurves[curveIndex]->mPoints.begin() + i + 1, p);
				trackSegNum.mCurves[curveIndex]->invalidate();
				break;
			}
		}

	}

	template<typename T>
	void SequenceEditorController::deleteCurvePoint(
		const std::string& trackID,
		const std::string& segmentID,
		const int index,
		int curveIndex)
	{
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		// find segment
		SequenceTrackSegment* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		//
		auto& trackSegVec = static_cast<SequenceTrackSegmentCurve<T>&>(*segment);
		assert(curveIndex < trackSegVec.mCurves.size());

		if (index < trackSegVec.mCurves[curveIndex]->mPoints.size())
		{
			//
			trackSegVec.mCurves[curveIndex]->mPoints.erase(trackSegVec.mCurves[curveIndex]->mPoints.begin() + index);
			trackSegVec.mCurves[curveIndex]->invalidate();
		}
	}

	template<typename T>
	void SequenceEditorController::changeCurvePoint(
		const std::string& trackID,
		const std::string& segmentID,
		const int pointIndex,
		const int curveIndex,
		float time,
		float value)
	{
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		// find segment
		SequenceTrackSegment* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		//
		auto& trackSegVec = static_cast<SequenceTrackSegmentCurve<T>&>(*segment);;
		assert(curveIndex < trackSegVec.mCurves.size());
		assert(pointIndex < trackSegVec.mCurves[curveIndex]->mPoints.size());

		//
		math::FCurvePoint<float, float>& curvePoint
			= trackSegVec.mCurves[curveIndex]->mPoints[pointIndex];
		curvePoint.mPos.mTime += time;
		curvePoint.mPos.mValue += value;
		curvePoint.mPos.mValue = math::clamp<float>(curvePoint.mPos.mValue, 0.0f, 1.0f);
		trackSegVec.mCurves[curveIndex]->invalidate();
	}

	template<typename T>
	void SequenceEditorController::changeTanPoint(
		const std::string& trackID,
		const std::string& segmentID,
		const int pointIndex,
		const int curveIndex,
		TanPointTypes tanType,
		float time,
		float value)
	{
		//
		std::unique_lock<std::mutex> lock = mSequencePlayer.lock();

		// find segment
		SequenceTrackSegment* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr);

		//
		auto& trackSegVec = static_cast<SequenceTrackSegmentCurve<T>&>(*segment);;
		assert(curveIndex < trackSegVec.mCurves.size());
		assert(pointIndex < trackSegVec.mCurves[curveIndex]->mPoints.size());

		//
		auto& curvePoint = trackSegVec.mCurves[curveIndex]->mPoints[pointIndex];
		switch (tanType)
		{
		case IN:
		{
			if (curvePoint.mInTan.mTime + time < curvePoint.mOutTan.mTime)
			{
				curvePoint.mInTan.mTime += time;
				curvePoint.mInTan.mValue += value;

				if (curvePoint.mTangentsAligned)
				{
					curvePoint.mOutTan.mTime = -curvePoint.mInTan.mTime;
					curvePoint.mOutTan.mValue = -curvePoint.mInTan.mValue;
				}
			}
		}
		break;
		case OUT:
		{
			if (curvePoint.mOutTan.mTime + time > curvePoint.mInTan.mTime)
			{
				curvePoint.mOutTan.mTime += time;
				curvePoint.mOutTan.mValue += value;

				if (curvePoint.mTangentsAligned)
				{
					curvePoint.mInTan.mTime = -curvePoint.mOutTan.mTime;
					curvePoint.mInTan.mValue = -curvePoint.mOutTan.mValue;
				}
			}

		}
		break;
		}

		// is this the last control point ?
		// then also change the first control point of the next segment accordinly
		if (pointIndex == trackSegVec.mCurves[curveIndex]->mPoints.size() - 1)
		{
			SequenceTrack* track = findTrack(trackID);
			assert(track != nullptr);

			for (int i = 0; i < track->mSegments.size(); i++)
			{
				if (track->mSegments[i].get() == segment &&
					i + 1 < track->mSegments.size())
				{
					auto& nextSegmentCurvePoint =
						static_cast<SequenceTrackSegmentCurve<T>&>(*track->mSegments[i + 1]).mCurves[curveIndex]->mPoints[0];

					nextSegmentCurvePoint.mInTan.mTime = curvePoint.mInTan.mTime;
					nextSegmentCurvePoint.mInTan.mValue = curvePoint.mInTan.mValue;
					nextSegmentCurvePoint.mOutTan.mTime = curvePoint.mOutTan.mTime;
					nextSegmentCurvePoint.mOutTan.mValue = curvePoint.mOutTan.mValue;

				}
			}
		}

		trackSegVec.mCurves[curveIndex]->invalidate();
	}

	template<typename T>
	void SequenceEditorController::changeMinMaxCurveTrack(const std::string& trackID, T minimum, T maximum)
	{
		auto l = mSequencePlayer.lock();

		SequenceTrack* track = findTrack(trackID);
		assert(track != nullptr);
		
		SequenceTrackCurve<T>* trackCurve = static_cast<SequenceTrackCurve<T>*>(track);
		trackCurve->mMinimum = minimum;
		trackCurve->mMaximum = maximum;
	}

	// explicit template declarations
	template NAPAPI void nap::SequenceEditorController::addNewCurveTrack<float>();
	template NAPAPI void nap::SequenceEditorController::addNewCurveTrack<glm::vec2>();
	template NAPAPI void nap::SequenceEditorController::addNewCurveTrack<glm::vec3>();
	template NAPAPI void nap::SequenceEditorController::addNewCurveTrack<glm::vec4>();

	template NAPAPI void nap::SequenceEditorController::insertCurveSegment<float>(const std::string& trackID, double time);
	template NAPAPI void nap::SequenceEditorController::insertCurveSegment<glm::vec2>(const std::string& trackID, double time);
	template NAPAPI void nap::SequenceEditorController::insertCurveSegment<glm::vec3>(const std::string& trackID, double time);
	template NAPAPI void nap::SequenceEditorController::insertCurveSegment<glm::vec4>(const std::string& trackID, double time);

	template NAPAPI void nap::SequenceEditorController::insertCurvePoint<float>(const std::string& trackID, const std::string& segmentID, float pos, int curveIndex);
	template NAPAPI void nap::SequenceEditorController::insertCurvePoint<glm::vec2>(const std::string& trackID, const std::string& segmentID, float pos, int curveIndex);
	template NAPAPI void nap::SequenceEditorController::insertCurvePoint<glm::vec3>(const std::string& trackID, const std::string& segmentID, float pos, int curveIndex);
	template NAPAPI void nap::SequenceEditorController::insertCurvePoint<glm::vec4>(const std::string& trackID, const std::string& segmentID, float pos, int curveIndex);

	template NAPAPI void nap::SequenceEditorController::changeCurvePoint<float>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, float time, float value);
	template NAPAPI void nap::SequenceEditorController::changeCurvePoint<glm::vec2>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, float time, float value);
	template NAPAPI void nap::SequenceEditorController::changeCurvePoint<glm::vec3>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, float time, float value);
	template NAPAPI void nap::SequenceEditorController::changeCurvePoint<glm::vec4>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, float time, float value);

	template NAPAPI void nap::SequenceEditorController::changeCurveSegmentValue<float>(const std::string& trackID, const std::string& segmentID, float amount, int curveIndex, SequenceEditorTypes::SegmentValueTypes valueType);
	template NAPAPI void nap::SequenceEditorController::changeCurveSegmentValue<glm::vec2>(const std::string& trackID, const std::string& segmentID, float amount, int curveIndex, SequenceEditorTypes::SegmentValueTypes valueType);
	template NAPAPI void nap::SequenceEditorController::changeCurveSegmentValue<glm::vec3>(const std::string& trackID, const std::string& segmentID, float amount, int curveIndex, SequenceEditorTypes::SegmentValueTypes valueType);
	template NAPAPI void nap::SequenceEditorController::changeCurveSegmentValue<glm::vec4>(const std::string& trackID, const std::string& segmentID, float amount, int curveIndex, SequenceEditorTypes::SegmentValueTypes valueType);

	template NAPAPI void nap::SequenceEditorController::changeTanPoint<float>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, SequenceEditorTypes::TanPointTypes tanType, float time, float value);
	template NAPAPI void nap::SequenceEditorController::changeTanPoint<glm::vec2>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, SequenceEditorTypes::TanPointTypes tanType, float time, float value);
	template NAPAPI void nap::SequenceEditorController::changeTanPoint<glm::vec3>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, SequenceEditorTypes::TanPointTypes tanType, float time, float value);
	template NAPAPI void nap::SequenceEditorController::changeTanPoint<glm::vec4>(const std::string& trackID, const std::string& segmentID, const int pointIndex, const int curveIndex, SequenceEditorTypes::TanPointTypes tanType, float time, float value);

	template NAPAPI void nap::SequenceEditorController::deleteCurvePoint<float>(const std::string& trackID, const std::string& segmentID, const int index, int curveIndex);
	template NAPAPI void nap::SequenceEditorController::deleteCurvePoint<glm::vec2>(const std::string& trackID, const std::string& segmentID, const int index, int curveIndex);
	template NAPAPI void nap::SequenceEditorController::deleteCurvePoint<glm::vec3>(const std::string& trackID, const std::string& segmentID, const int index, int curveIndex);
	template NAPAPI void nap::SequenceEditorController::deleteCurvePoint<glm::vec4>(const std::string& trackID, const std::string& segmentID, const int index, int curveIndex);

	template NAPAPI void SequenceEditorController::changeMinMaxCurveTrack<float>(const std::string& trackID, float minimum, float maximum);
	template NAPAPI void SequenceEditorController::changeMinMaxCurveTrack<glm::vec2>(const std::string& trackID, glm::vec2 minimum, glm::vec2 maximum);
	template NAPAPI void SequenceEditorController::changeMinMaxCurveTrack<glm::vec3>(const std::string& trackID, glm::vec3 minimum, glm::vec3 maximum);
	template NAPAPI void SequenceEditorController::changeMinMaxCurveTrack<glm::vec4>(const std::string& trackID, glm::vec4 minimum, glm::vec4 maximum);

	template NAPAPI void SequenceEditorController::changeCurveType<float>(const std::string& trackID, const std::string& segmentID, math::ECurveInterp type);
	template NAPAPI void SequenceEditorController::changeCurveType<glm::vec2>(const std::string& trackID, const std::string& segmentID, math::ECurveInterp type);
	template NAPAPI void SequenceEditorController::changeCurveType<glm::vec3>(const std::string& trackID, const std::string& segmentID, math::ECurveInterp type);
	template NAPAPI void SequenceEditorController::changeCurveType<glm::vec4>(const std::string& trackID, const std::string& segmentID, math::ECurveInterp type);
}
