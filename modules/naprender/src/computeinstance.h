/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local Includes
#include "renderservice.h"
#include "materialinstance.h"

// External Includes
#include <nap/resourceptr.h>
#include <transformcomponent.h>
#include <rect.h>

namespace nap
{
	/**
	 * Compute Instance
	 * This is a work in progress. To-do's:
	 * - Improve abstraction/interface
	 * - Decide how this instance should be created/used (instance, resource, component?)
	 * - Finish documentation
	 */
	class NAPAPI ComputeInstance final
	{
		friend class RenderService;
	public:
		/**
		 * Default constructor
		 */
		ComputeInstance() = delete;
		ComputeInstance(ComputeMaterialInstanceResource& computeMaterialInstanceResource, RenderService* renderService);

		// Destructor
		~ComputeInstance();

		// Copy constructor
		ComputeInstance(const RenderableMesh& rhs) = delete;

		// Copy assignment operator
		ComputeInstance& operator=(const ComputeInstance& rhs) = delete;

		/**
		 * Initializes the compute instance.
		 * @param errorState contains the error if initialization fails
		 * @return if initialization succeeded.
		 */
		bool init(utility::ErrorState& errorState);

		/**
		 * Executes the compute shader.
		 * @return if the compute work was submitted successfully.
		 */
		bool compute(uint numInvocations, utility::ErrorState& errorState);

		/**
		 * Syncs the graphics queue with this compute shader.
		 * The vertex shader input stage of the current frame will not be executed until the compute shader stage is finished.
		 * Useful when compute shaders modify resources that are read by graphics shaders.
		 * @param numInvocations the number of compute shader invocations
		 * @param graphicsStageFlags the graphics stage flags signifying the current computation depends on.
		 * @return if the compute work was submitted to the compute queue successfully.
		 */
		bool compute(uint numInvocations, VkPipelineStageFlags graphicsStageFlags, utility::ErrorState& errorState);

		/**
		 * @return current material used when drawing the mesh.
		 */
		ComputeMaterialInstance& getComputeMaterialInstance() 			{ return mComputeMaterialInstance; }

		/**
		 * Returns the local workgroup size
		 */
		glm::u32vec3 getLocalWorkGroupSize() const						{ return mComputeMaterialInstance.getComputeMaterial().getShader().getLocalWorkGroupSize(); }

		/**
		 * Signaled before command buffer recording ends
		 * This call can be used to push post-dispatch commands onto the compute command buffer such as vkCmdCopyBuffer.
		 * Additional synchronization (e.g. vkCmdPipelineBarrier(srcStageMask:COMPUTE, dstStageMask:TRANSFER ...)) must be handled by the user.
		 */
		nap::Signal<const DescriptorSet&> mPostDispatch;

	private:
		bool computeInternal(uint numInvocations, utility::ErrorState& errorState);

		ComputeMaterialInstanceResource*		mComputeMaterialInstanceResource = nullptr;
		ComputeMaterialInstance					mComputeMaterialInstance;

		RenderService*							mRenderService = nullptr;
		std::vector<VkSemaphore>				mSemaphores;								///< GPU synchronization primitive
	};
}
