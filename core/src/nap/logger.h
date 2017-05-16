#pragma once

// Local Includes
#include "utility/stringutils.h"
#include "signalslot.h"

// External Includes
#include <memory>
#include <mutex>
#include <string>


// TODO is there a way to avoid/get rid of this macro?
#define NAP_DECLARE_LOG_LEVEL(LEVEL, NAME)														\
	static const LogLevel& NAME##Level()														\
	{																							\
        static LogLevel lvl(#NAME, LEVEL);														\
		return lvl;																				\
    }																							\
    static void NAME(const std::string& msg)													\
	{																							\
		instance().log(LogMessage(NAME##Level(), msg));											\
	}																							\
	\
static void																						\
	NAME(const Object& obj, const std::string& msg)												\
	{																							\
		instance().log(LogMessage(NAME##Level(), msg, &obj));									\
	}																							\
	template <typename... Args>																	\
	static void NAME(const std::string& msg, Args... args)										\
	{																							\
		instance().log(LogMessage(NAME##Level(), utility::stringFormat(msg, args...)));			\
	}																							\
	template <typename... Args>																	\
	static void NAME(const Object& obj, const std::string& msg, Args... args)					\
	{																							\
		instance().log(LogMessage(NAME##Level(), utility::stringFormat(msg, args...), &obj));	\
	}



namespace nap
{
    
    // forward declare
	class Object;

    /**
     * A LogLevel defines a tag for log messages associated to define what messages are output
     * in a certain situation or configuration.
     * Levels are ranked according to their level number.
     * A higher number means it will be output in more situations.
     * Examples are fatal error messages, warnings or debug messages.
     */
	class LogLevel
	{
	public:
        /**
         * Constructor taked the name of the level and a number to indicate where this level is ranked
         * in comparison to other levels
         */
		LogLevel(const std::string& name, int level) : mName(name), mLevel(level) {}
        // returns the level number to indicate the level's ranking
		int level() const { return mLevel; }
		const std::string& name() const { return mName; }
        
        // comparison operators
		bool operator<(const LogLevel& other) const { return this->mLevel < other.mLevel; }
		bool operator==(const LogLevel& other) const { return this->mLevel == other.mLevel; }
		bool operator<=(const LogLevel& other) const
		{
			return this->mLevel < other.mLevel || this->mLevel == other.mLevel;
		}

	private:
		std::string mName;
		int mLevel;
	};


    /**
     * A LogMessage is tagged with a log level.
     * Optionally it stores the object that outputs the message.
     */
	class LogMessage
	{
	public:
		LogMessage(const LogLevel& lvl, const std::string& msg, const Object* obj = nullptr)
			: mLevel(lvl), mMessage(msg), mObj(obj)
		{
		}

        // the level ranking of the message
		const LogLevel& level() const { return mLevel; }
        // returns the actual message
		const std::string& text() const { return mMessage; }
        // optionally returns the object that outputs the message
		const Object* object() const { return mObj; }

	private:
		const LogLevel mLevel;
		const std::string mMessage;
		const Object* mObj;
	};

    
    
    /**
     * The Logger invokes log messages through a global system using a singleton isntance.
     */
	class Logger
	{
	public:
        /** 
         * Sets the current log level.
         * Messages with a level lower than the current level will not be displayed.
         */
		static void setLevel(LogLevel lvl) { instance().mLevel = lvl; }
        // returns the current level.
		static const LogLevel& getLevel() { return instance().mLevel; }

		// Singleton access
		static Logger& instance();

	public:
        // this signal is emitted every time a log message is output.
		Signal<LogMessage> log;

        // all log messages will be displayed
		NAP_DECLARE_LOG_LEVEL(50, fine)
        // messages including debug messages
		NAP_DECLARE_LOG_LEVEL(100, debug)
        // general information about the proceedings of the app
		NAP_DECLARE_LOG_LEVEL(200, info)
        // warnings about unusual situations
		NAP_DECLARE_LOG_LEVEL(300, warn)
        // fatal errors that seriously hinder the functionality of the app
		NAP_DECLARE_LOG_LEVEL(400, fatal)

	private:
		LogLevel mLevel;

		void onLog(LogMessage message);
		Slot<LogMessage> onLogSlot = {[&](LogMessage message) { onLog(message); }};

		Logger() : mLevel(debugLevel()) { log.connect(onLogSlot); }

		Logger(Logger const&) : mLevel(debugLevel()) {}
		void operator=(Logger const&);

		std::mutex outputMutex;
	};
}
