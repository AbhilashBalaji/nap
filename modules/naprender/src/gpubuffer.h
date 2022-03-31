/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local Includes
#include "fillpolicy.h"
#include "formatutils.h"
#include "renderutils.h"

// External Includes
#include <nap/resourceptr.h>
#include <glm/glm.hpp>
#include <nap/logger.h>
#include <vulkan/vulkan_core.h>
#include <utility/dllexport.h>
#include <nap/signalslot.h>
#include <vector>
#include <utility/errorstate.h>

namespace nap
{
	class RenderService;
	class Core;

	/**
	 * Flag that determines how GPU data, typically nap::GPUBuffer, is used at runtime. Regards the type of data access
	 * between host (CPU) and device (GPU).
	 *
	 * For instance, a 'Static' buffer (i.e. containing a mesh) is uploaded from the CPU to the GPU exactly once.
	 * This allows the system to remove unused buffers after the upload is complete.
	 * If there is the need to update a buffer more frequently, even once after upload,
	 * it is required the usage is set to 'DynamicWrite'.
	 *
	 * Note that static data is often placed in a different cache on the GPU, not accessible by the CPU, which
	 * allows for faster drawing times. 'DynamicWrite' buffers are uploaded into shared CPU / GPU memory
	 * and are therefore slower to access. Keep this in mind when selecting the appropriate memory usage.
	 */
	enum class EMemoryUsage : uint
	{
		Static,				///< Buffer data is uploaded only once from the CPU to the GPU
		DynamicRead,		///< Buffer data is uploaded only once from the CPU to the GPU, and frequently read from GPU to CPU
		DynamicWrite		///< Buffer data is updated more than once from the CPU to the GPU
	};

	//////////////////////////////////////////////////////////////////////////
	// GPU Buffer
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Defines a Vulkan buffer object on the GPU.
	 *
	 * A static buffer is updated (uploaded to) only once, a dynamic buffer can be updated more frequently but
	 * requires more resources and is 'generally' slower to draw, depending of the memory layout of the underlying hardware.
	 * It is not allowed to update a static buffer after the initial upload!
	 *
	 * Note that static data is often placed in a different cache on the GPU, not accessible by the CPU, which
	 * allows for faster drawing times. 'DynamicWrite' buffers are uploaded into shared CPU / GPU memory
	 * and are therefore slower to access. Keep this in mind when selecting the appropriate memory usage.
	 */
	class NAPAPI BaseGPUBuffer : public Resource
	{
		friend class RenderService;
		RTTI_ENABLE(Resource)
	public:
		/**
		 * Every buffer needs to have access to the render engine.
		 * The given 'usage' controls if a buffer can be updated more than once
		 * and in which memory space it is placed.
		 * @param core the nap core
		 */
		BaseGPUBuffer(Core& core);

		/**
		 * Every buffer needs to have access to the render engine.
		 * The given 'usage' controls if a buffer can be updated more than once
		 * and in which memory space it is placed.
		 * @param core the nap core
		 * @param usage how the buffer is used at runtime.
		 */
		BaseGPUBuffer(Core& core, EMemoryUsage usage);

		virtual ~BaseGPUBuffer();

		// Copy construction not allowed
		BaseGPUBuffer(const BaseGPUBuffer& other) = delete;

		// Copy assignment not allowed
		BaseGPUBuffer& operator=(const BaseGPUBuffer& other) = delete;

		/**
		 * @return handle to the Vulkan buffer.
		 */
		virtual VkBuffer getBuffer() const;

		/**
		 * @return handle to the buffer data.
		 */
		virtual const BufferData& getBufferData() const;

		/**
		 * Initialize the buffer
		 */
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * @return the number of buffer elements
		 */
		virtual uint getCount() const = 0;

		/**
		 * @return the size of the buffer in bytes
		 */
		virtual size_t getSize() const = 0;

		/**
		 * @return the element size in bytes
		 */
		virtual uint getElementSize() const = 0;

		/**
		 * @return the buffer usage flags.
		 */
		virtual VkBufferUsageFlags getBufferUsageFlags() const { return mUsageFlags; }

		/**
		 * Ensures the given buffer usage flags are applied when allocating and creating the buffer,
		 * next to the flags derived from the 'Usage' property. Call this function before allocation.
		 * @param usage buffer usage flags required on allocation
		 */
		void ensureUsage(VkBufferUsageFlags usage) { mUsageFlags |= usage; }

		/**
		 * Implemented by derived classes
		 * @return whether this buffer is initialized
		 */
		virtual bool isInitialized() const = 0;

		/**
		 * Starts a transfer of buffer data from GPU to CPU. Use this overload to pass your own copy function. This is a non blocking call.
		 * @param copyFunction the copy function to call when the buffer data is available for download.
		 */
		void asyncGetData(std::function<void(const void*, size_t)> copyFunction);

		/**
		 * Called right after the buffer on the GPU has been updated.
		 */
		nap::Signal<> bufferChanged;

		EMemoryUsage			mMemoryUsage = EMemoryUsage::Static;			///< Property 'Usage' How the buffer is used: initialized once (Static), updated frequently from CPU to GPU (DynamicWrite) or read from GPU to CPU (DynamicRead).
		bool					mClear = false;									///< Property 'Clear' If no fill policy is set, performs an initial clear-to-zero transfer operation on the device buffer on init()

	protected:

		/**
		 * Allocates buffers, called by derived classes
		 * @param size size in bytes of the buffer to allocate
		 * @param deviceUsage how the data is used at runtime on the device (e.g. VERTEX, INDEX, UNIFORM, STORAGE)
		 * @param errorState contains error when data could not be set.
		 * @return if the data was set
		 */
		bool allocateInternal(size_t size, utility::ErrorState& errorState);

		/**
		 * Allocates and updates GPU buffer content, called by derived classes.
		 * @param data pointer to the data to upload.
		 * @param size size in bytes of the data to upload
		 * @param reservedSize allows the buffer to allocate more memory than required, needs to be >= size
		 * @param deviceUsage how the data is used at runtime on the device (e.g. VERTEX, INDEX, UNIFORM, STORAGE)
		 * @param errorState contains error when data could not be set.
		 * @return if the data was set
		 */
		bool setDataInternal(const void* data, size_t size, size_t reservedSize, utility::ErrorState& errorState);

		/**
		 * Helper function that calls requestBufferClear() in RenderService for derived classes
		 */
		void requestClear();

		RenderService* mRenderService = nullptr;					///< Handle to the render service
		std::vector<BufferData>		mRenderBuffers;								///< Render accessible buffers
		std::vector<BufferData>		mStagingBuffers;							///< Staging buffers, used when uploading or downloading data
		uint32						mSize = 0;									///< Current used buffer size in bytes

		int							mCurrentRenderBufferIndex = 0;				///< Current render buffer index
		int							mCurrentStagingBufferIndex = 0;				///< Current staging buffer index
		std::vector<int>			mDownloadStagingBufferIndices;				///< Staging buffer indices associated with a frameindex

	private:
		using BufferReadCallback = std::function<void(void* data, size_t sizeInBytes)>;

		// Called when usage = static
		bool setDataInternalStatic(const void* data, size_t size, utility::ErrorState& errorState);

		// Called when usage = dynamic write
		bool setDataInternalDynamic(const void* data, size_t size, size_t reservedSize, VkBufferUsageFlags deviceUsage, utility::ErrorState& errorState);

		// Uploads data from the staging buffer into GPU buffer. Automatically called by the render service at the appropriate time.
		// Only occurs when 'usage' = 'static'. Dynamic data shares GPU / CPU memory and is updated immediately.
		void upload(VkCommandBuffer commandBuffer);

		// Downloads data from GPU buffer to staging buffer
		void download(VkCommandBuffer commandBuffer);

		// Clears the buffer to zero
		void clear(VkCommandBuffer commandBuffer);

		// Called by the render service when download is ready
		void notifyDownloadReady(int frameIndex);

		// Clears queued texture downloads
		void clearDownloads();

		std::vector<BufferReadCallback>	mReadCallbacks;			///< Number of callbacks based on number of frames in flight
		VkBufferUsageFlags mUsageFlags = 0;						///< Buffer usage flags that are shared over host (staging) and device (gpu) buffers
	};


	//////////////////////////////////////////////////////////////////////////
	// GPUBufferNumeric
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Base class for all types of one dimensional GPU buffers.
	 * Supported values for child classes such as GPUBufferNumeric<T> must be primitives that can be mapped to 
	 * VkFormat. This is enforced by the requirement to implement getFormat().
	 */
	class NAPAPI GPUBuffer : public BaseGPUBuffer
	{
		RTTI_ENABLE(BaseGPUBuffer)
	public:
		GPUBuffer(Core& core) :
			BaseGPUBuffer(core)
		{ }

		GPUBuffer(Core& core, EMemoryUsage usage) :
			BaseGPUBuffer(core, usage)
		{ }

		/**
		 * Uploads data to the GPU based on the settings provided.
		 * This function automatically allocates GPU memory if required.
		 * @param data pointer to the block of data that needs to be uploaded.
		 * @param elementCount the number of elements
		 * @param reservedElementCount the number of elements to reserve
		 * @param errorState contains the error if upload operation failed
		 * @return if upload succeeded
		 */
		virtual bool setData(const void* data, size_t elementCount, size_t reservedElementCount, utility::ErrorState& errorState) = 0;

		/**
		 * @return the buffer format
		 */
		virtual VkFormat getFormat() const = 0;

		uint32 mCount = 0;				///< Property: 'Count' The number of  elements to initialize/allocate the buffer with.
	};


	//////////////////////////////////////////////////////////////////////////
	// Numeric GPU Buffer
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Typed class for GPU numeric buffers.
	 *
	 * Allocates all required host (staging) and device buffers based on the specified properties.
	 * If a 'FillPolicy' is available, the buffer will also be uploaded to immediately. Alternatively, 'Clear' sets all
	 * of the buffer values to zero on init(). 'FillPolicy' and 'Clear' are mutually exclusive and the former has
	 * priority over the latter.
	 * 
	 * Supported types are primitive types that can be mapped to VkFormat.
	 * @tparam T primitive value data type
	 */
	template<typename T>
	class GPUBufferNumeric : public GPUBuffer
	{
		RTTI_ENABLE(GPUBuffer)
	public:
		/**
		 * Every numeric buffer needs to have access to the render engine.
		 * @param renderService the render engine
		 */
		GPUBufferNumeric(Core& core) :
			GPUBuffer(core)
		{ }

		/**
		 * Every numeric buffer needs to have access to the render engine.
		 * The given 'usage' controls if a buffer can be updated more than once, and in which memory space it is placed.
		 * The format defines the vertex element size in bytes.
		 * @param renderService the render engine
		 * @param usage how the buffer is used at runtime.
		 */
		GPUBufferNumeric(Core& core, EMemoryUsage usage) :
			GPUBuffer(core, usage)
		{ }

		/**
		 * Initialize this buffer. This will allocate all required staging and device buffers based on the specified properties.
		 * If a 'FillPolicy' is available, the buffer will also be uploaded to immediately. Alternatively, 'Clear' sets all
		 * of the buffer values to zero on init(). 'FillPolicy' and 'Clear' are mutually exclusive and the former has priority
		 * over the latter.
		 */
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * Uploads data to the GPU based on the settings provided.
		 * This function automatically allocates GPU memory if required.
		 * @param data pointer to the block of data that needs to be uploaded.
		 * @param elementCount the number of elements
		 * @param reservedElementCount the number of elements to reserve
		 * @param errorState contains the error if upload operation failed
		 * @return if upload succeeded
		 */
		virtual bool setData(const void* data, size_t elementCount, size_t reservedElementCount, utility::ErrorState& errorState) override;

		/**
		 * Uploads data to the GPU based on the settings provided.
		 * This function automatically allocates GPU memory if required.
		 * @param data vector containing the data to upload
		 * @param errorState contains the error if upload operation failed
		 * @return if upload succeeded
		 */
		bool setData(const std::vector<T>& data, utility::ErrorState& errorState);

		/**
		 * @return the number of buffer values
		 */
		virtual uint getCount() const override							{ return mCount; }

		/**
		 * @return the size of the buffer in bytes
		 */
		virtual size_t getSize() const override							{ return mCount * sizeof(T); };

		/**
		 * @return the size of a single vertex element
		 */
		virtual uint getElementSize() const override					{ return sizeof(T); };

		/**
		 * @return the buffer format
		 */
		virtual VkFormat getFormat() const override						{ return getVulkanFormat<T>(); }

		/**
		 * @return whether this buffer is initialized
		 */
		virtual bool isInitialized() const override						{ return mInitialized; };

		ResourcePtr<FillPolicy<T>>						mFillPolicy = nullptr;	///< Property 'FillPolicy' Optional fill policy to fill the buffer with on initialization

	protected:
		// Whether the buffer was successfully initialized
		bool											mInitialized = false;
	};


	//////////////////////////////////////////////////////////////////////////
	// Vertex Buffers
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Vertex GPU buffers.
	 *
	 * Allocates all required host (staging) and device buffers based on the specified properties.
	 * If a 'FillPolicy' is available, the buffer will also be uploaded to immediately. Alternatively, 'Clear' sets all
	 * of the buffer values to zero on init(). 'FillPolicy' and 'Clear' are mutually exclusive and the former has
	 * priority over the latter.
	 *
	 * In addition to nap::GPUBufferNumeric, this class distinguishes vertex buffers from general purpose numeric buffers.
	 * Internally, some flags are stored that help the driver identify and optimize buffers that have a specific purpose in a rendering operation.
	 * They also play a role in synchronization of compute and graphics operations.
	 *
	 * Supported types are primitive types that can be mapped to VkFormat.
	 * @tparam T primitive value data type
	 */
	template<typename T>
	class VertexBuffer final : public GPUBufferNumeric<T>
	{
		RTTI_ENABLE(GPUBufferNumeric<T>)
	public:
		/**
		 * Every vertex buffer needs to have access to the render engine.
		 * @param renderService the render engine
		 */
		VertexBuffer(Core& core) :
			GPUBufferNumeric<T>(core)
		{ }

		/**
		 * Every vertex buffer needs to have access to the render engine.
		 * The given 'usage' controls if a buffer can be updated more than once, and in which memory space it is placed.
		 * The format defines the vertex element size in bytes.
		 * @param renderService the render engine
		 * @param usage how the buffer is used at runtime.
		 * @param if the buffer is allowed to be bound to a shader as a storage buffer using a descriptor
		 */
		VertexBuffer(Core& core, EMemoryUsage usage, bool storage) :
			GPUBufferNumeric<T>(core, usage), mStorage(storage)
		{ }

		/**
		 * Initialize this buffer. This will allocate all required staging and device buffers based on the buffer properties.
		 * If a fill policy is available, the buffer will also be uploaded to immediately.
		 */
		virtual bool init(utility::ErrorState& errorState) override;

	private:
		bool mStorage = true;			///< Property: Allows the buffer to be bound to a shader as a storage buffer using a descriptor, allowing it be read and set from a shader program.
	};


	//////////////////////////////////////////////////////////////////////////
	// Index Buffer
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Index GPU buffer.
	 *
	 * Allocates all required host (staging) and device buffers based on the specified properties.
	 * If a 'FillPolicy' is available, the buffer will also be uploaded to immediately. Alternatively, 'Clear' sets all
	 * of the buffer values to zero on init(). 'FillPolicy' and 'Clear' are mutually exclusive and the former has
	 * priority over the latter.
	 *
	 * In addition to nap::GPUBufferNumeric, this class distinguishes index buffers from general purpose numeric buffers.
	 * Internally, some flags are stored that help the driver identify and optimize buffers that have a specific purpose in a rendering operation.
	 *
	 * Supported types are primitive types that can be mapped to VkFormat.
	 * @tparam T primitive value data type
	 */
	class NAPAPI IndexBuffer final : public GPUBufferNumeric<uint>
	{
		RTTI_ENABLE(GPUBufferNumeric<uint>)
	public:
		/**
		 * Every index buffer needs to have access to the render engine.
		 * @param renderService the render engine
		 */
		IndexBuffer(Core & core) : GPUBufferNumeric<uint>(core)			{ }

		/**
		 * Every index buffer needs to have access to the render engine.
		 * The given 'usage' controls if a buffer can be updated more than once, and in which memory space it is placed.
		 * The format defines the vertex element size in bytes.
		 * @param renderService the render engine
		 * @param usage how the buffer is used at runtime.
		 * @param storage if the buffer is allowed to be bound to a shader as a storage buffer using a descriptor
		 */
		IndexBuffer(Core & core, EMemoryUsage usage, bool storage) :
			GPUBufferNumeric<uint>(core, usage), mStorage(storage)		{ }

		/**
		 * Initialize this buffer. This will allocate all required staging and device buffers based on the buffer properties.
		 * If a fill policy is available, the buffer will also be uploaded to immediately.
		 */
		virtual bool init(utility::ErrorState & errorState) override;

	private:
		bool mStorage = true;			///< Allows the buffer to be bound to a shader as a storage buffer using a descriptor, allowing it be read and set from a shader program.
	};


	//////////////////////////////////////////////////////////////////////////
	// GPU numeric buffer type definitions
	//////////////////////////////////////////////////////////////////////////

	// General purpose GPU buffers
	using GPUBufferUInt			= GPUBufferNumeric<uint>;
	using GPUBufferInt			= GPUBufferNumeric<int>;
	using GPUBufferFloat		= GPUBufferNumeric<float>;
	using GPUBufferVec2			= GPUBufferNumeric<glm::vec2>;
	using GPUBufferVec3			= GPUBufferNumeric<glm::vec3>;
	using GPUBufferVec4			= GPUBufferNumeric<glm::vec4>;
	using GPUBufferMat4			= GPUBufferNumeric<glm::mat4>;

	// Vertex GPU buffers
	using VertexBufferUInt		= VertexBuffer<uint>;
	using VertexBufferInt		= VertexBuffer<int>;
	using VertexBufferFloat		= VertexBuffer<float>;
	using VertexBufferVec2		= VertexBuffer<glm::vec2>;
	using VertexBufferVec3		= VertexBuffer<glm::vec3>;
	using VertexBufferVec4		= VertexBuffer<glm::vec4>;


	//////////////////////////////////////////////////////////////////////////
	// Template Definitions
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	bool GPUBufferNumeric<T>::init(utility::ErrorState& errorState)
	{
		if (!GPUBuffer::init(errorState))
			return false;

		if (!errorState.check(mMemoryUsage != EMemoryUsage::DynamicWrite || mCount >= 0,
			"Cannot allocate a non-DynamicWrite buffer with zero elements."))
			return false;

		// Calculate buffer size
		uint32 buffer_size = mCount * sizeof(T);

		// Allocate buffer memory
		if (!allocateInternal(buffer_size, errorState))
			return false;

		// Upload data when a buffer fill policy is available
		if (mFillPolicy != nullptr)
		{
			if (mMemoryUsage != EMemoryUsage::DynamicRead)
			{
				// Create a staging buffer to upload
				auto staging_buffer = std::make_unique<T[]>(mCount);
				mFillPolicy->fill(mCount, staging_buffer.get());

				// Prepare staging buffer upload
				if (!setDataInternal(staging_buffer.get(), buffer_size, buffer_size, errorState))
					return false;
			}
			else
			{
				// Warn user that buffers cannot be filled when their usage is set to DynamicRead
				nap::Logger::warn(utility::stringFormat("%s: The configured fill policy was ignored as the buffer usage is DynamicRead", mID.c_str()).c_str());
			}
		}

		// Optionally clear - does not count as an upload
		else if (mClear)
			BaseGPUBuffer::requestClear();

		mInitialized = true;
		return true;
	}

	template<typename T>
	bool GPUBufferNumeric<T>::setData(const void* data, size_t elementCount, size_t reservedElementCount, utility::ErrorState& errorState)
	{
		if (!setDataInternal(data, sizeof(T) * elementCount, sizeof(T) * reservedElementCount, errorState))
			return false;

		// Update count
		mCount = elementCount;
		return true;
	}

	template<typename T>
	bool GPUBufferNumeric<T>::setData(const std::vector<T>& data, utility::ErrorState& errorState)
	{
		if (!setDataInternal(data.data(), data.size() * sizeof(T), data.capacity() * sizeof(T), errorState))
			return false;

		// Update count
		mCount = data.size();
		return true;
	}

	template<typename T>
	bool VertexBuffer<T>::init(utility::ErrorState& errorState)
	{
		VkBufferUsageFlags req_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		req_usage |= mStorage ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
		GPUBufferNumeric<T>::ensureUsage(req_usage);
		return GPUBufferNumeric<T>::init(errorState);
	}
}
