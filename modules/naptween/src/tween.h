/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// internal includes
#include "tweeneasing.h"
#include "tweenmode.h"

// external includes
#include <mathutils.h>
#include <nap/signalslot.h>
#include <nap/logger.h>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	// forward declares
	class TweenService;

	/**
	 * Base class of every tween
	 */
	class NAPAPI TweenBase
	{
		// because the killed signal is dispatched from the TweenHandle upon destruction we want the handle to access the TweenBase private members in this case
		friend class TweenHandleBase;
	public:
		/**
		 * Constructor
		 */
		TweenBase() = default;

		/**
		 * Default deconstructor
		 */
		virtual ~TweenBase() = default;

		/**
		 * Update function called by tween service
		 * @param deltaTime
		 */
		virtual void update(double deltaTime) = 0;
	public:
		// signals

		/**
		 * Killed signal will be dispatched when the handle of the Tween is deconstructed
		 */
		Signal<> KilledSignal;
	protected:
		// killed boolean
		bool 	mKilled 	= false;

		// complete boolean
		bool 	mComplete 	= false;
	};

	/**
	 * A Tween is responsible for interpolating between two values over the period of a certain time using an easing method ( see : https://github.com/jesusgollonet/ofpennereasing )
	 * A Tween is updated by the TweenService, which holds all unique pointers to Tweens and thus retains ownership over all Tweens created
	 * This is the reason the only way to create a working tween is by calling createTween on the TweenService
	 * Because a Tween is always created and updated by the TweenService, it means all update calls will occur on the main thread
	 * You can tween any type that supports arithmetic operators
	 * Please note that a tween can ONLY be accessible outside the TweenService by using the TweenHandle
	 * @tparam T the type of value that you would like to tween
	 */
	template<typename T>
	class Tween : public TweenBase
	{
	public:
		/**
		 * Constructor taking the initial start & end value of the tween, plus duration
		 * @param start start value of the tween
		 * @param end end value of the tween
		 * @param duration duration of the tween
		 */
		Tween(T start, T end, float duration);

		/**
		 * update function called by the TweenService
		 * @param deltaTime
		 */
		void update(double deltaTime) override;

		/**
		 * set easing method used for tweening
		 * @param easing the easing method
		 */
		void setEase(ETweenEasing easing);

		/**
		 * sets the tween mode for this tween, see ETweenMode enum
		 * @param mode
		 */
		void setMode(ETweenMode mode);

		/**
		 * restart the tween
		 */
		void restart();

		/**
		 * @return current tween mode
		 */
		const ETweenMode getMode() const { return mMode; }

		/**
		 * @return current ease type
		 */
		const ETweenEasing getEase() const{ return mEasing; }

		/**
		 * @return current time in time
		 */
		const float getTime() const { return mTime; }

		/**
		 * @return duration of tween
		 */
		const float getDuration() const { return mDuration; }

		/**
		 * @return current tweened value
		 */
		const T& getCurrentValue() const { return mCurrentValue; }

		/**
		 * @return start value
		 */
		const T& getStartValue() const { return mStart; }

		/**
		 * @return end value
		 */
		const T& getEndValue() const { return mEnd; }
	public:
		// Signals

		/**
		 * Update signal dispatched on value update
		 * Occurs on main thread
		 */
		Signal<const T&> UpdateSignal;

		/**
		 * Complete signal dispatched when tween is finished
		 * Always dispatched on main thread
		 */
		Signal<const T&> CompleteSignal;
	private:

		/**
		 * unique ptr to current easing method
		 */
		std::unique_ptr<TweenEaseBase<T>> mEase = nullptr;

		/**
		 * holds update function, update function is dependent on current tween mode
		 */
		std::function<void(double)> mUpdateFunc;

		// current time
		float 			mTime 			= 0.0f;

		// start value
		T 				mStart;

		// end value
		T				mEnd;

		// current value
		T 				mCurrentValue;

		// duration
		float 			mDuration;

		// tween mode
		ETweenMode 		mMode 			= ETweenMode::NORMAL;

		// ease type
		ETweenEasing 	mEasing 		= ETweenEasing::LINEAR;
	};

	//////////////////////////////////////////////////////////////////////////
	// shortcuts
	//////////////////////////////////////////////////////////////////////////
	using TweenFloat 	= Tween<float>;
	using TweenDouble 	= Tween<double>;
	using TweenVec2 	= Tween<glm::vec2>;
	using TweenVec3 	= Tween<glm::vec3>;

	//////////////////////////////////////////////////////////////////////////
	// explicit MSVC template specialization exports
	//////////////////////////////////////////////////////////////////////////
	template class NAPAPI Tween<float>;
	template class NAPAPI Tween<double>;
	template class NAPAPI Tween<glm::vec2>;
	template class NAPAPI Tween<glm::vec3>;

	//////////////////////////////////////////////////////////////////////////
	// template definition
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	Tween<T>::Tween(T start, T end, float duration)
		: TweenBase(), mStart(start), mEnd(end), mCurrentValue(start), mDuration(duration)
	{
		setEase(ETweenEasing::LINEAR);
		setMode(ETweenMode::NORMAL);
	}

	template<typename T>
	void Tween<T>::update(double deltaTime)
	{
		if(mKilled)
			return;

		mUpdateFunc(deltaTime);
	}

	template<typename T>
	void Tween<T>::setMode(ETweenMode mode)
	{
		mMode = mode;

		switch (mode)
		{
		default:
			nap::Logger::warn("Unknown tween mode, choosing default mode");
		case NORMAL:
		{
			mUpdateFunc = [this](double deltaTime)
			{
			  if(!mComplete)
			  {
				  mTime += deltaTime;

				  if( mTime >= mDuration )
				  {
					  mTime = mDuration;
					  mComplete = true;

					  mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);

					  UpdateSignal.trigger(mCurrentValue);
					  CompleteSignal.trigger(mCurrentValue);
				  }else
				  {
					  mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);
					  UpdateSignal.trigger(mCurrentValue);
				  }
			  }
			};
		}
			break;
		case PING_PONG:
		{
			float direction = 1.0f;
			mUpdateFunc = [this, direction](double deltaTime) mutable
			{
				mTime += deltaTime * direction;

				if( mTime > mDuration )
				{
					mTime = mDuration - ( mTime - mDuration );
					direction = -1.0f;

					mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);
					UpdateSignal.trigger(mCurrentValue);
				}else if( mTime < 0.0f )
				{
					mTime = -mTime;
					direction = 1.0f;

					mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);
					UpdateSignal.trigger(mCurrentValue);
				}else
				{
					mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);
					UpdateSignal.trigger(mCurrentValue);
				}
			};
		}
			break;
		case LOOP:
		{
			mUpdateFunc = [this](double deltaTime)
			{
				if(!mComplete)
				{
					mTime += deltaTime;

					if( mTime > mDuration )
					{
						mTime = mDuration - mTime;

						mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);
						UpdateSignal.trigger(mCurrentValue);
					}else
					{
						mCurrentValue = mEase->evaluate(mStart, mEnd, mTime / mDuration);
						UpdateSignal.trigger(mCurrentValue);
					}
				}
			};
		}
			break;
		case REVERSE:
		{
			mUpdateFunc = [this](double deltaTime)
			{
				if(!mComplete)
				{
					mTime += deltaTime;

					if( mTime > mDuration )
					{
						mComplete = true;
					  	mTime = mDuration;

						mCurrentValue = mEase->evaluate(mStart, mEnd, 1.0f - ( mTime / mDuration ));

						UpdateSignal.trigger(mCurrentValue);
					  	CompleteSignal.trigger(mCurrentValue);
					}else
					{
						mCurrentValue = mEase->evaluate(mStart, mEnd, 1.0f - ( mTime / mDuration ));
					  	UpdateSignal.trigger(mCurrentValue);
					}
				}
			};
		}
			break;
		}
	}

	template<typename T>
	void Tween<T>::restart()
	{
		mTime	 		= 0.0f;
		mComplete 		= false;
		mKilled 		= false;
		mCurrentValue 	= mStart;
	}


	template<typename T>
	void Tween<T>::setEase(ETweenEasing easing)
	{
		mEasing = easing;

		switch (easing)
		{
		default:
			nap::Logger::warn("Unknown easing mode, choosing default linear easing");
			mEasing = ETweenEasing::LINEAR;
		case ETweenEasing::LINEAR:
			mEase = std::make_unique<TweenEaseLinear<T>>();
			break;
		case ETweenEasing::CUBIC_INOUT:
			mEase = std::make_unique<TweenEaseInOutCubic<T>>();
			break;
		case ETweenEasing::CUBIC_OUT:
			mEase = std::make_unique<TweenEaseOutCubic<T>>();
			break;
		case ETweenEasing::CUBIC_IN:
			mEase = std::make_unique<TweenEaseInCubic<T>>();
			break;
		case ETweenEasing::BACK_OUT:
			mEase = std::make_unique<TweenEaseOutBack<T>>();
			break;
		case ETweenEasing::BACK_INOUT:
			mEase = std::make_unique<TweenEaseInOutBack<T>>();
			break;
		case ETweenEasing::BACK_IN:
			mEase = std::make_unique<TweenEaseInBack<T>>();
			break;
		case ETweenEasing::BOUNCE_OUT:
			mEase = std::make_unique<TweenEaseOutBounce<T>>();
			break;
		case ETweenEasing::BOUNCE_INOUT:
			mEase = std::make_unique<TweenEaseInOutBounce<T>>();
			break;
		case ETweenEasing::BOUNCE_IN:
			mEase = std::make_unique<TweenEaseInBounce<T>>();
			break;
		case ETweenEasing::CIRC_OUT:
			mEase = std::make_unique<TweenEaseOutCirc<T>>();
			break;
		case ETweenEasing::CIRC_INOUT:
			mEase = std::make_unique<TweenEaseInOutCirc<T>>();
			break;
		case ETweenEasing::CIRC_IN:
			mEase = std::make_unique<TweenEaseInCirc<T>>();
			break;
		case ETweenEasing::ELASTIC_OUT:
			mEase = std::make_unique<TweenEaseOutElastic<T>>();
			break;
		case ETweenEasing::ELASTIC_INOUT:
			mEase = std::make_unique<TweenEaseInOutElastic<T>>();
			break;
		case ETweenEasing::ELASTIC_IN:
			mEase = std::make_unique<TweenEaseInElastic<T>>();
			break;
		case ETweenEasing::EXPO_OUT:
			mEase = std::make_unique<TweenEaseOutExpo<T>>();
			break;
		case ETweenEasing::EXPO_INOUT:
			mEase = std::make_unique<TweenEaseInOutExpo<T>>();
			break;
		case ETweenEasing::EXPO_IN:
			mEase = std::make_unique<TweenEaseInExpo<T>>();
			break;
		case ETweenEasing::QUAD_OUT:
			mEase = std::make_unique<TweenEaseOutQuad<T>>();
			break;
		case ETweenEasing::QUAD_INOUT:
			mEase = std::make_unique<TweenEaseInOutQuad<T>>();
			break;
		case ETweenEasing::QUAD_IN:
			mEase = std::make_unique<TweenEaseInQuad<T>>();
			break;
		case ETweenEasing::QUART_OUT:
			mEase = std::make_unique<TweenEaseOutQuart<T>>();
			break;
		case ETweenEasing::QUART_INOUT:
			mEase = std::make_unique<TweenEaseInOutQuart<T>>();
			break;
		case ETweenEasing::QUART_IN:
			mEase = std::make_unique<TweenEaseInQuart<T>>();
			break;
		case ETweenEasing::QUINT_OUT:
			mEase = std::make_unique<TweenEaseOutQuint<T>>();
			break;
		case ETweenEasing::QUINT_INOUT:
			mEase = std::make_unique<TweenEaseInOutQuint<T>>();
			break;
		case ETweenEasing::QUINT_IN:
			mEase = std::make_unique<TweenEaseInQuint<T>>();
			break;
		case ETweenEasing::SINE_OUT:
			mEase = std::make_unique<TweenEaseOutSine<T>>();
			break;
		case ETweenEasing::SINE_INOUT:
			mEase = std::make_unique<TweenEaseInOutSine<T>>();
			break;
		case ETweenEasing::SINE_IN:
			mEase = std::make_unique<TweenEaseInSine<T>>();
			break;
		}
	}
}

