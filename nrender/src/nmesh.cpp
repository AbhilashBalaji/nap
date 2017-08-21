#include "nmesh.h"
#include <assert.h>

namespace opengl
{
	const Mesh::VertexAttributeID Mesh::VertexAttributeIDs::PositionVertexAttr("Position");
	const Mesh::VertexAttributeID Mesh::VertexAttributeIDs::NormalVertexAttr("Normal");
	const Mesh::VertexAttributeID Mesh::VertexAttributeIDs::UVVertexAttr("UV");
	const Mesh::VertexAttributeID Mesh::VertexAttributeIDs::ColorVertexAttr("Color");

	const opengl::Mesh::VertexAttributeID Mesh::VertexAttributeIDs::GetUVVertexAttr(int uvChannel)
	{
		std::ostringstream stream;
		stream << UVVertexAttr << uvChannel;
		return stream.str();
	}


	const opengl::Mesh::VertexAttributeID Mesh::VertexAttributeIDs::GetColorVertexAttr(int colorChannel)
	{
		std::ostringstream stream;
		stream << ColorVertexAttr << colorChannel;
		return stream.str();
	}


	// Constructor initializes object draw mode
	Mesh::Mesh(int numVertices, EDrawMode drawMode) :
		mNumVertices(numVertices),
		mDrawMode(drawMode)
	{	}


	void Mesh::addVertexAttribute(const VertexAttributeID& id, unsigned int components, const float* data)
	{
		VertexAttribute attribute;
		attribute.mID = id;
		attribute.mData = std::make_unique<FloatVertexContainer>();

		// Copy our data
		bool success = attribute.mData->copyData(components, mNumVertices, data);
		assert(success);

		// Synchronize on success (data in CPU memory will be uploaded to GPU)
		attribute.mData->sync();

		mAttributes.emplace_back(std::move(attribute));
	}


	const VertexAttributeBuffer* Mesh::findVertexAttributeBuffer(const VertexAttributeID& id) const
	{
		for (const VertexAttribute& attribute : mAttributes)
        {
            if (attribute.mID == id)
                return attribute.mData->getVertexBuffer();
        }

        opengl::printMessage(opengl::MessageType::ERROR, "Unable to find vertex attribute buffer associated with attribute: %s. The shader compiler might have stripped it away", id.c_str());
		return nullptr;
	}


	void Mesh::setIndices(unsigned int count, const unsigned int* data)
	{
		assert(count > 0);

		if (mIndices == nullptr)
		{
			mIndices = std::make_unique<IndexContainer>();
		}

		// Copy our data
		mIndices->copyData(count, data);

		// Synchronize on success (data in CPU memory will be uploaded to GPU)
		mIndices->sync();
	}
}
