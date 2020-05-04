#pragma once

// Local Includes
#include "vulkan/vulkan_core.h"
#include "utility/dllexport.h"
#include "vk_mem_alloc.h"

namespace nap
{
	/**
	 * Defines a buffer object on the GPU and acts as a base class for 
	 * all Vulkan derived buffer types. This object creation / destruction 
	 * as well as the internal buffer type.
	 */
	class NAPAPI GPUBuffer
	{
	public:
		GPUBuffer(VmaAllocator vmaAllocator);

		/**
		 * Default destructor
		 */
		virtual ~GPUBuffer();

		GPUBuffer(const GPUBuffer& other) = delete;
		GPUBuffer& operator=(const GPUBuffer& other) = delete;

		VkBuffer getBuffer() const { return mBuffer; }

	protected:
		void setDataInternal(VkPhysicalDevice physicalDevice, VkDevice device, void* data, int elementSize, size_t numVertices, size_t reservedNumVertices, VkBufferUsageFlagBits usage);

	private:
		VmaAllocator		mVmaAllocator = VK_NULL_HANDLE;
		VmaAllocation		mAllocation = VK_NULL_HANDLE;
		VkBuffer			mBuffer = VK_NULL_HANDLE;
		size_t				mCurCapacity = 0;			// Amount of memory reserved
		size_t				mCurSize = 0;				// defines the number of points in the buffer
	};

} // opengl
