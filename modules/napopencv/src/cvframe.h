#pragma once

#include <rtti/rtti.h>
#include <vector>
#include <opencv2/core/mat.hpp>

namespace nap
{
	/**
	 * Single OpenCV frame that contains one or multiple OpenCV matrices.
	 * Together the matrices define the content of a frame. A frame can be copied and moved. 
	 * Note that a frame copy (assignment or construction) does not copy the actual content of the matrices, it only increases the ref count.
	 * To create a deep copy of a frame, including all the matrices, call CVFrame::deepCopyTo().
	 */
	class NAPAPI CVFrame final
	{
		RTTI_ENABLE()
	public:
		// Default Constructor
		CVFrame() = default;
		
		// Default Destructor
		~CVFrame() = default;
		
		// Default copy constructor
		CVFrame(const CVFrame& other) = default;
		
		// Default copy assignment operator
		CVFrame& operator=(const CVFrame& other) = default;
		
		// Move constructor
		CVFrame(CVFrame&& other);
		
		// Move assignment operator
		CVFrame& operator=(CVFrame&& other);

		/**
		 * Constructs a new frame with the given number of matrices.
		 * @param count the number of matrices to create.
		 */
		CVFrame(int count);

		/**
		 * Array subscript overload. Does not perform a bounds check!
		 * @return the matrix at the given index
		 */
		cv::UMat& operator[](std::size_t index)					{ return mMatrices[index]; }

		/**
		 * Array subscript overload. Does not perform a bounds check!
		 * @return the matrix at the given index
		 */
		const cv::UMat& operator[](std::size_t index) const		{ return mMatrices[index]; }

		/**
		 * Performs a deep copy of this frame.
		 * The default copy operation does not copy the actual content of the matrices, only increases the ref count.
		 * The content of the given frame is cleared.
		 * @param outFrame the frame to copy the data of this frame to.
		 */
		void deepCopyTo(CVFrame& outFrame) const;

		/**
		 * Clones this frame including all of it's content.
		 * The default copy operation does not copy the actual content of the matrices, only increases the ref count.
		 * @return a clone of this frame.
		 */
		CVFrame clone() const;

		/**
		 * @return the number of matrices this frame contains.
		 */
		int getCount() const									{ return static_cast<int>(mMatrices.size()); }

		/**
		 * Adds a new empty matrix to this frame.
		 * @return the newly created matrix.
		 */
		cv::UMat& addNew();

		/**
		 * Adds the given matrix to this frame.
		 * @param matrix new OpenCV matrix to add to this frame.
		 */
		void add(const cv::UMat& matrix);

		/**
		 * Gives a matrix to this frame. Ownership is transferred.
		 * @param matrix new OpenCV matrix to add to this frame.
		 */
		void add(cv::UMat&& matrix);

		/**
		 * Clears all frame data.
		 */
		void clear();

	private:
		std::vector<cv::UMat> mMatrices;	///< All OpenCV matrices associated with the frame
	};
}