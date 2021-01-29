#include "calendaritem.h"

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::CalendarItem::Time)
	RTTI_CONSTRUCTOR(int, int)
	RTTI_PROPERTY("Hour",	&nap::CalendarItem::Time::mHour,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Minute", &nap::CalendarItem::Time::mMinute,	nap::rtti::EPropertyMetaData::Required)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::CalendarItem)
	RTTI_PROPERTY("Time",			&nap::CalendarItem::mTime,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Duration",		&nap::CalendarItem::mDuration,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Title",			&nap::CalendarItem::mTitle,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Description",	&nap::CalendarItem::mDescription,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::MonthlyCalendarItem)
	RTTI_PROPERTY("Day",			&nap::MonthlyCalendarItem::mDay,			nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::WeeklyCalendarItem)
	RTTI_PROPERTY("Day",			&nap::WeeklyCalendarItem::mDay,				nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::DailyCalendarItem)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::UniqueCalendarItem)
	RTTI_PROPERTY("Date", &nap::UniqueCalendarItem::mDate, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

namespace nap
{

	CalendarItem::Time::Time(int hour, int minute) : mHour(hour), mMinute(minute)
	{ }


	nap::Minutes nap::CalendarItem::Time::toMinutes() const
	{
		return nap::Minutes((mHour * 60) + mMinute);
	}


	bool CalendarItem::init(utility::ErrorState& errorState)
	{
		if (!errorState.check(
			mTime.mHour >= 0 && mTime.mHour < 24 && 
			mTime.mMinute >= 0 && mTime.mMinute < 60, 
			"%s, invalid time", mID.c_str()))
			return false;

		if (!errorState.check(mDuration.mHour >= 0 && mDuration.mMinute >= 0,
			"%s, negative duration", mID.c_str()))
			return false;

		return true;
	}


	bool WeeklyCalendarItem::init(utility::ErrorState& errorState)
	{
		if (!CalendarItem::init(errorState))
			return false;

		// Ensure day is not specified as unknown
		if (!errorState.check(mDay != EDay::Unknown, "%s: day not specified", mID.c_str()))
			return false;

		return CalendarItem::init(errorState);
	}


	bool WeeklyCalendarItem::active(SystemTimeStamp timeStamp)
	{
		// Compute start / end in minutes
		Minutes sample_mins(static_cast<int>(mDay) * 24 * 60);
		Minutes sta_time = sample_mins + mTime.toMinutes();
		Minutes end_time = sta_time + mDuration.toMinutes();

		// Get current minutes in week
		DateTime cur_dt(timeStamp);
		Minutes cur_day_mins(static_cast<int>(cur_dt.getDay()) * 24 * 60);
		Minutes cur_hou_mins((cur_dt.getHour() * 60) + cur_dt.getMinute());
		Minutes cur_mins(cur_day_mins + cur_hou_mins);

		// See if it's in bounds
		if (cur_mins >= sta_time && cur_mins < end_time)
			return true;

		// Wrap to start if end_time exceeds total minutes in week
		constexpr Minutes minutesWeek(7 * 24 * 60);
		if (end_time > minutesWeek)
		{
			Minutes wrapped(end_time.count() % minutesWeek.count());
			return cur_mins < wrapped;
		}
		return false;
	}


	bool DailyCalendarItem::init(utility::ErrorState& errorState)
	{
		return CalendarItem::init(errorState);
	}


	bool DailyCalendarItem::active(SystemTimeStamp timeStamp)
	{
		// Get minute bounds for day
		Minutes sta_time(mTime.toMinutes());
		Minutes end_time = sta_time + mDuration.toMinutes();

		// Get current minute in day
		DateTime cur_dt(timeStamp);
		Minutes cur_mins((cur_dt.getHour() * 60) + cur_dt.getMinute());

		// See if it's in bounds
		if (cur_mins >= sta_time && cur_mins < end_time)
			return true;

		// Wrap to start if end_time exceeds total minutes in day
		constexpr Minutes minutesDay(Hours(24));
		if (end_time > minutesDay)
		{
			Minutes wrapped(end_time.count() % minutesDay.count());
			return cur_mins < wrapped;
		}
		return false;
	}


	bool UniqueCalendarItem::init(utility::ErrorState& errorState)
	{
		// Base class
		if (!CalendarItem::init(errorState))
			return false;

		// Ensure date exists
		if(!errorState.check(mDate.valid(), "%s: invalid date", mID.c_str()))
			return false;

		return true;
	}


	bool UniqueCalendarItem::active(SystemTimeStamp timeStamp)
	{
		SystemTimeStamp sta_time = mDate.toSystemTime() + mTime.toMinutes();
		SystemTimeStamp end_time = sta_time + mDuration.toMinutes();
		return (timeStamp >= sta_time && timeStamp < end_time);
	}


	bool MonthlyCalendarItem::init(utility::ErrorState& errorState)
	{
		if (!CalendarItem::init(errorState))
			return false;

		// Check if day is at least within bounds
		if (!errorState.check(mDay >= 1 && mDay <= 31, "%s: invalid day", mID.c_str()))
			return false;

		return true;
	}


	bool MonthlyCalendarItem::active(SystemTimeStamp timeStamp)
	{
		// Get sample (lookup) date
		DateTime cur_dt(timeStamp);
		int cur_year = cur_dt.getYear();
		int cur_mont = static_cast<int>(cur_dt.getMonth());
		
		// If the day exists in this month, we can try to sample ahead.
		if (Date::exists(cur_dt.getYear(), cur_dt.getMonth(), mDay))
		{
			// If current time is ahead of lookup, see if it's in range
			SystemTimeStamp lookup_time = createTimestamp(cur_year, cur_mont, mDay, mTime.mHour, mTime.mMinute);
			if (timeStamp >= lookup_time)
			{
				SystemTimeStamp end_time = lookup_time + mDuration.toMinutes();
				return timeStamp < end_time;
			}
		}

		// Otherwise we need to check if the current time falls within the window of the previous months range.
		// For example: an event triggered at the end of a month can be active in the beginning of a new month.
		cur_mont -= 1;
		if (cur_mont == 0)
		{
			cur_mont = 12;
			cur_year -= 1;
		}

		// Make sure the day exists in the last month, if not the event could not have been triggered
		if (!Date::exists(cur_dt.getYear(), static_cast<EMonth>(cur_mont), mDay))
			return false;

		// Previous month sample date
		SystemTimeStamp lookup_time = createTimestamp(cur_year, cur_mont, mDay, mTime.mHour, mTime.mMinute);

		// Check if it falls in the previous month window
		SystemTimeStamp end_time = lookup_time + mDuration.toMinutes();
		if (timeStamp >= lookup_time && timeStamp < end_time)
			return true;

		return false;
	}
}
