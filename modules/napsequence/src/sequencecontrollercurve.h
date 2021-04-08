/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local Includes
#include "sequencecontroller.h"
#include "sequencecurveenums.h"
#include "sequencetrackcurve.h"
#include "sequenceutils.h"

// External Includes
#include <mathutils.h>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Controller class for curve tracks
	 */
	class NAPAPI SequenceControllerCurve : public SequenceController
	{
	public:
		/**
		 * Constructor
		 * @param player reference to player
		 * @param editor reference to editor
		 */
		SequenceControllerCurve(SequencePlayer & player, SequenceEditor& editor) : SequenceController(player, editor) { }

		/**
		 * @param trackID the id of the track
		 * @param segmentID the id of the segment we need to edit
		 * @param duration the new duration
		 * @return new duration of segment
	 	 */
		double segmentDurationChange(const std::string& trackID, const std::string& segmentID, float duration);

		/**
		 * adds a new curve track of type T ( float, vec2, vec3, vec4 )
		 */
		template<typename T>
		void addNewCurveTrack();

		/**
		 * changes start or end value of segment of type T
		 * @param trackID the track id
		 * @param segmentID id of segment
		 * @param newValue the new value
		 * @param curveIndex the curve index of the value
		 * @param valueType the segment value type ( first or last value )
		 */
		void changeCurveSegmentValue(const std::string& trackID, const std::string& segmentID, float newValue, int curveIndex, SequenceCurveEnums::SegmentValueTypes valueType);

		/**
		 * insert point in curve of segment
		 * @param trackID the track id
		 * @param segmentID the segment id
		 * @param pos the position at which to insert the curvepoint in curve ( range 0-1 )
		 * @param curveIndex the index of the curve
		 */
		void insertCurvePoint(const std::string& trackID, const std::string& segmentID, float pos, int curveIndex);

		/**
		 * deletes point from curve
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param index the point index
		 * @param curveIndex the curveIndex
		 */
		void deleteCurvePoint(const std::string& trackID, const std::string& segmentID, int index, int curveIndex);

		/**
		 * changes a curvepoint value and time / position
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param pointIndex the point index
		 * @param curveIndex the curve index
		 * @param time new time
		 * @param value new value
		 */
		void changeCurvePoint(const std::string& trackID, const std::string& segmentID, int pointIndex, int curveIndex, float time, float value);

		/**
		 * changes tangent of curve point. Tangents are always aligned
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param pointIndex the point index
		 * @param curveIndex the curve index
		 * @param tanType in or out tangent
		 * @param time offset for new time
		 * @param value offset for new value
		 */
		void changeTanPoint(const std::string& trackID, const std::string& segmentID, int pointIndex, int curveIndex, SequenceCurveEnums::ETanPointTypes tanType, float time, float value);

		/**
		 * changes minimum and maximum value of track
		 * @param trackID the trackID
		 * @param minimum new minimum
		 * @param maximum new maximum
		 */
		template<typename T>
		void changeMinMaxCurveTrack(const std::string& trackID, T minimum, T maximum);

		/**
		 * changes curvetype ( linear or bezier )
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param type the new curve type
		 */
		template<typename T>
		void changeCurveType(SequenceTrackSegment& segment, math::ECurveInterp type, int curveIndex);

		/**
		 * overloaded insert segment function
		 * @param trackID the track id
		 * @param time time
		 * @return const pointer to newly created segment
		 */
		const SequenceTrackSegment* insertSegment(const std::string& trackID, double time) override;

		/**
		 * overloaded delete segment function
		 * @param trackID track id
		 * @param segmentID segment id
		 */
		void deleteSegment(const std::string& trackID, const std::string& segmentID) override;

		/**
		 * overloaded insert track function
		 * @param type track type
		 */
		void insertTrack(rttr::type type) override;

		/**
		 * updates curve segments values to be continuous ( segment 1 end value == segment 2 start value etc )
		 * @param trackID the track id of the track that we want to update
		 */
		void updateCurveSegments(const std::string& trackID);

		/**
		 * changes curvetype ( linear or bezier )
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param type the new curve type
		 * @param curveIndex the index of the curve
		 */
		void changeCurveType(const std::string& trackID, const std::string& segmentID, math::ECurveInterp type, int curveIndex);

		/**
		 * register a member function pointer to function responsible for update a segment for a certain track type
		 * @param trackType rttr of track type
		 * @param memFun pointer to member function
		 * @return true on success
		 */
		static bool registerUpdateSegmentFunctionForTrackType(const rttr::type& trackType, void(SequenceControllerCurve::*memFun)(SequenceTrack&));

		/**
		 * register a member function pointer to function responsible for inserting a segment for a certain track type
		 * @param trackType rttr of track type
		 * @param memFun pointer to member function
		 * @return true on success
		 */
		static bool registerInsertSegmentFunctionForTrackType(const rttr::type& trackType, const SequenceTrackSegment*(SequenceControllerCurve::*memFun)(const std::string&, double));

		/**
		 * this method is called during static initialization to register controllers & functions for curved track types
		 * @return true on success
		 */
		static bool registerControllerForTrackTypes();
	protected:
		/**
		 * updates curve segments values to be continuous ( segment 1 end value == segment 2 start value etc )
		 * @param track reference to sequence track
		 */
		template<typename T>
		void updateCurveSegments(SequenceTrack& track);

		/**
		 * insertCurveSegment
		 * inserts a new curvesegment of type T ( vec2, vec3, vec4, float )
		 * @param trackID the track in which to insert the segment
		 * @param time the time at when to insert segment
		 */
		template <typename T>
		const SequenceTrackSegment* insertCurveSegment(const std::string& trackID, double time);

		/**
		 * changes tangent of curve point. Tangents are always aligned
		 * @paramt type of curve track
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param pointIndex the point index
		 * @param curveIndex the curve index
		 * @param tanType in or out tangent
		 * @param time offset for new time
		 * @param value offset for new value
		 */
		template <typename  T>
		void changeTanPoint(SequenceTrackSegment& segment, const std::string& trackID, int pointIndex, int curveIndex, SequenceCurveEnums::ETanPointTypes tanType, float time, float value);

		/**
		 * changes a curvepoint value and time / position
		 * @paramt type of curve track
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param pointIndex the point index
		 * @param curveIndex the curve index
		 * @param time offset for new time
		 * @param value offset for new value
		 */
		template <typename  T>
		void changeCurvePoint(SequenceTrackSegment& segment, int pointIndex, int curveIndex, float time, float value);

		template <typename T>
		void changeLastCurvePoint(SequenceTrackSegment& segment, int curveIndex, float time, float value);

		/**
		 * deletes point from curve
		 * @paramt type of curve track
		 * @param trackID the trackID
		 * @param segmentID the segmentID
		 * @param index the point index
		 * @param curveIndex the curveIndex
		 */
		template<typename T>
		void deleteCurvePoint(SequenceTrackSegment& segment, int index, int curveIndex);

		/**
		 * insert point in curve of segment
		 * @paramt type of curve track
		 * @param trackID the track id
		 * @param segmentID the segment id
		 * @param pos the position at which to insert the curvepoint in curve ( range 0-1 )
		 * @param curveIndex the index of the curve
		 */
		template<typename T>
		void insertCurvePoint(SequenceTrackSegment& segment, float pos, int curveIndex);

		/**
		 * changes start or end value of segment of type T
		 * @param trackID the track id
		 * @param segmentID id of segment
		 * @param amount the amount that the value needs to change
		 * @param curveIndex the curve index of the value
		 * @param valueType the segment value type ( first or last value )
		 */
		template<typename T>
		void changeCurveSegmentValue(SequenceTrack& track, SequenceTrackSegment& segment, float newValue, int curveIndex,
									 SequenceCurveEnums::SegmentValueTypes valueType);

		// map for updating segments
		static std::unordered_map<rttr::type, void(SequenceControllerCurve::*)(SequenceTrack&)>& getUpdateSegmentFunctionMap();

		// map for inserting segments
		static std::unordered_map<rttr::type, const SequenceTrackSegment*(SequenceControllerCurve::*)(const std::string&, double)>& getInsertSegmentFunctionMap();
	};

	//////////////////////////////////////////////////////////////////////////
	// Forward declarations
	//////////////////////////////////////////////////////////////////////////

	template<>
	void NAPAPI SequenceControllerCurve::changeMinMaxCurveTrack<float>(const std::string& trackID, float minimum, float maximum);
}

#include "sequencecontrollercurve_template.h"