#pragma once

// External Includes
#include <nap/resource.h>
#include <glm/glm.hpp>
#include <nap/datetime.h>
#include <nap/numeric.h>

namespace nap
{
	/**
	 * Base class of all calendar items.
	 * Every item has a time and duration. The title and description are optional.
	 * Derived classes must override active(). That method defines if the item
	 * currently 'occurs' based on the given timestamp.
	 */
	class NAPAPI CalendarItem : public Resource
	{
		RTTI_ENABLE(Resource)
	public:	

		/**
		 * Serializable calendar time structure.
		 * Can be copied and moved.
		 */
		struct NAPAPI Time
		{
			Time() = default;
			Time(int hour, int minute);
			uint mHour	= 0;					///< Property: 'Hour' (0-23)
			uint mMinute	= 0;				///< Property: 'Minute' (0-59)
			nap::Minutes toMinutes() const;		///< Convert into minutes
		};

		/**
		 * Serializable calendar point in time structure.
		 * Represents point in time together with duration
		 */
		struct NAPAPI Point
		{
			Point() = default;
			Point(Time time, Time duration);
			Time mTime;							///< Property: 'Time' time of the event: hours (0-23) & minutes (0-59)
			Time mDuration;						///< Property: 'Duration' length of event: hours (0-23) & minutes (0-59). Duration of 0 = never
			bool valid() const;  				///< Returns if time is valid
		};

		// Default Constructor
		CalendarItem() = default;

		// Item constructor
		CalendarItem(const Point& point, const std::string& title) : 
			mPoint(point), mTitle(title) { }

		/**
		 * Initializes the calendar item, always call this in derived classes.
		 * Ensures the given time is valid.
		 * @return If initialization succeeded.
		 */
		bool init(utility::ErrorState& errorState) override;

		/**
		 * Returns if the current item is active based  on the specified 
		 * 'Time', 'Duration' and other properties of this item.
		 * Must be implemented in derived classes.
		 * @param timeStamp time to validate, for example the current system time
		 * @return if the current item is active.
		 */
		virtual bool active(SystemTimeStamp timeStamp) = 0;

		/**
		 * Updates item title
		 * @param title new item title
		 */
		void setTitle(const std::string& title);

		/**
		 * @return item title
		 */
		const std::string& getTitle() const;

		/**
		 * Updates item description
		 * @param description new item description
		 */
		void setDescription(const std::string& description);
	
		/**
		 * @return item description
		 */
		const std::string& getDescription() const;

		/**
		 * Updates item time and duration and ensures new settings are valid.
		 * @param point new time and duration
		 * @return if time and duration have been updated
		 */
		bool setPoint(const Point& point);

		/**
		 * @return item time and duration
		 */
		const Point& getPoint() const;

		/**
		 * Updates time and ensures it is valid.
		 * @param time new time
		 * @return if the time is updated
		 */
		bool setTime(const Time& time);

		/**
		 * @return item time
		 */
		const Time& getTime() const;

		/**
		 * Updates item duration.
		 * @param duration new duration
		 * @return if the duration is updated
		 */
		void setDuration(const Time& duration);

		/**
		 * @return item duration
		 */
		const Time& getDuration() const;

		std::string mTitle = "";			///< Property: 'Title' item title
		Point		mPoint;					///< Property; 'Point' point in time together with duration
		std::string	mDescription = "";		///< Property: 'Description' item description
	};


	/**
	 * Monthly recurring calendar item
	 */
	class NAPAPI MonthlyCalendarItem : public CalendarItem
	{
		RTTI_ENABLE(CalendarItem)
	public:

		// Default constructor
		MonthlyCalendarItem() = default;

		// Argument constructor
		MonthlyCalendarItem(const CalendarItem::Point& point, const std::string& title, int day) :
			CalendarItem(point, title), mDay(day) {}

		/**
		 * @return if the day and time are valid
		 */
		bool init(utility::ErrorState& errorState) override;
		
		/**
		 * Sets the day of the month, ensures the day is in range (1-31).
		 * @param day the new day of the month (1-31)
		 * @return if the day has been updated
		 */
		bool setDay(int day);

		/**
		 * @return day of the month
		 */
		int getDay() const { return mDay; }

		/**
		 * @param timeStamp time to validate
		 * @return if the monthly calender item currently occurs.
		 */
		virtual bool active(SystemTimeStamp timeStamp) override;
		
		int mDay = 1;	///< Property: 'Day' day of the month (1-31)
	};


	/**
	 * Weekly recurring calendar item
	 */
	class NAPAPI WeeklyCalendarItem : public CalendarItem
	{
		RTTI_ENABLE(CalendarItem)
	public:

		// Default constructor
		WeeklyCalendarItem() = default;

		// Argument constructor
		WeeklyCalendarItem(const CalendarItem::Point& point, const std::string& title, EDay day) :
			CalendarItem(point, title), mDay(day) {}

		/**
		 * Initializes the weekly calendar item. 
		 * Checks if the day and time are valid.
		 * @return if the day and time are valid
		 */
		bool init(utility::ErrorState& errorState) override;

		/**
		 * Updates the day of the week
		 * @param day the new day of the week
		 * @return if the day of the week is updated
		 */
		bool setDay(EDay day);

		/**
		 * @return the day of the week
		 */
		EDay getDay() const;

		/**
		 * @param timeStamp time to validate
		 * @return if the weekly calender item currently occurs.
		 */
		virtual bool active(SystemTimeStamp timeStamp) override;

		EDay mDay = EDay::Monday;	///< Property: 'Day' day of the week
	};


	/**
	 * Daily recurring calendar item
	 */
	class NAPAPI DailyCalendarItem : public CalendarItem
	{
		RTTI_ENABLE(CalendarItem)
	public: 
		/**
		 * Initializes the daily calendar item.
		 * Checks if the time is valid.
		 * @return if the time is valid
		 */
		bool init(utility::ErrorState& errorState) override;

		/**
		 * @param timeStamp time to validate
		 * @return if the daily calender item currently occurs.
		 */
		virtual bool active(SystemTimeStamp timeStamp) override;
	};


	/**
	 * Unique calendar item
	 */
	class NAPAPI UniqueCalendarItem : public CalendarItem
	{
		RTTI_ENABLE(CalendarItem)
	public:

		// Default constructor
		UniqueCalendarItem();

		// Argument constructor
		UniqueCalendarItem(const CalendarItem::Point& point, const std::string& title, const Date& date) :
			CalendarItem(point, title), mDate(date) {}

		/**
		 * Initializes the unique calendar item.
		 * Checks if the date and time are valid.
		 * @return if the date and time are valid
		 */
		bool init(utility::ErrorState& errorState) override;
		
		/**
		 * Updates the calendar date, ensures the new date is valid
		 * @param date the new date
		 * @return if the date is updated
		 */
		bool setDate(const nap::Date& date);

		/**
		* @return the calendar date
		*/
		nap::Date getDate() const;

		/**
		 * @param timeStamp time to validate
		 * @return if the unique calender item is active.
		 */
		virtual bool active(SystemTimeStamp timeStamp) override;

		nap::Date mDate;	///< Property: 'Date' calendar date
	};
}
