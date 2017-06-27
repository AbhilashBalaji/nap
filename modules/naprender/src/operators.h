#pragma once

// External Includes
#include <nap.h>

namespace nap
{
	/**
	 * THIS IS A TEST OPERATOR
	 */
	class ExecuteDrawOperator : public Operator
	{
		RTTI_ENABLE(Operator)
	public:
		ExecuteDrawOperator();

		OutputTriggerPlug drawOutputPlug  = { this, "draw" };

	private:
		void init();
	};
}
