#pragma once

// Local Includes
#include "uniformdeclarations.h"
#include "gpuvaluebuffer.h"

// External Includes
#include <rtti/objectptr.h>
#include <glm/glm.hpp>
#include <utility/dllexport.h>
#include <nap/resource.h>

namespace nap
{
	// Forward Declares
	class Texture2D;
	class UniformInstance;

	using UniformCreatedCallback = std::function<void()>;


	/**
	 * Access key and usage mode of a uniform struct. Dictates the usage of all child uniforms.
	 * Uniform usage directly maps to the descriptor set binding index in the shader
	 */
	enum class EUniformSetKey : int
	{
		DynamicWrite = 0,	// DynamicWrite: Update uniform data each frame
		Static = 1,			// Static: Update uniform data once on descriptorset update
		Handle = 2,			// Handle: Bring Your Own Resources. Use for pre-initialized data such as GPU buffers
		None = -1			// None: Invalid usage
	};


	/**
	 * Sets outSet to the corresponding Uniform set enum type if supported
	 * @param set the input uniform set index
	 * @return the output uniform set enum type
	 */
	static nap::EUniformSetKey getUniformSetKey(int set)
	{
		if (set == static_cast<int>(nap::EUniformSetKey::DynamicWrite))
			return nap::EUniformSetKey::DynamicWrite;

		else if (set == static_cast<int>(nap::EUniformSetKey::Static))
			return nap::EUniformSetKey::Static;

		else if (set == static_cast<int>(nap::EUniformSetKey::Handle))
			return nap::EUniformSetKey::Handle;

		// If this assert is triggered, an unsupported uniform set index was given
		assert(false);

		// Return none
		return nap::EUniformSetKey::None;
	}


	/**
	 * Shader uniform resource base class.
	 */
	class NAPAPI Uniform : public Resource
	{
		RTTI_ENABLE(Resource)
	public:
		std::string mName;		///< Name of uniform in shader
	};


	/**
	 * Contains other uniforms, including: values, structs and arrays.
	 * myshader.frag example:
	 * 
	 * ~~~~~{.cpp}
	 *	struct PointLight
	 *	{
	 *		vec3		mPosition;
	 *		vec3 		mIntensity;
	 *	};
	 * ~~~~~
	 */
	class NAPAPI UniformStruct : public Uniform
	{
		RTTI_ENABLE(Uniform)
	public:

		/**
		 * Adds a uniform.
		 * @param uniform the uniform to add
		 */
		void addUniform(Uniform& uniform);

		/**
		 * @param name the name of the uniform to find.
		 * @return a uniform with the given name, nullptr if not found
		 */
		Uniform* findUniform(const std::string& name);

	public:
		std::vector<rtti::ObjectPtr<Uniform>> mUniforms;
		EUniformSetKey mSet;	///< Usage of the uniform
	};
	

	/**
	 * A list of uniform structs. 
	 * The list must be declared inside a uniform buffer object (block).
	 * myshader.frag example:
	 *
	 * ~~~~~{.cpp}
	 *	struct PointLight						///< Point light structure
	 *	{
	 *		vec3		mPosition;
	 *		vec3 		mIntensity;
	 *	};
	 * 
	 *	uniform UBO								///< Uniform buffer object
	 *	{
	 *		uniform PointLight lights[10];		///< 10 lights
	 *	} ubo;
	 * ~~~~~
	 */
	class NAPAPI UniformStructArray : public Uniform
	{
		RTTI_ENABLE(Uniform)
	public:
		/**
		 * Inserts a structure at the given index.
		 * @param index index to insert struct.
		 * @param uniformStruct the struct to insert
		 */
		void insertStruct(int index, UniformStruct& uniformStruct);

	public:
		std::vector<rtti::ObjectPtr<UniformStruct>> mStructs;
	};


	/**
	 * Uniform value base class.
	 */
	class NAPAPI UniformValue : public Uniform
	{
		RTTI_ENABLE(Uniform)
	};


	/**
	 * Specific type of uniform value, for example: float, vec2, vec3, int etc.
	 * All values must be declared inside a uniform buffer object (block).
	 * myshader.frag example:
	 *
	 * ~~~~~{.cpp}
	 *	uniform UBO								///< Uniform buffer object
	 *	{
	 *		vec3 color;							///< object color uniform
	 *		float length;						///< hair length uniform
	 *	} ubo;
	 * ~~~~~
	 */
	template<typename T>
	class TypedUniformValue : public UniformValue
	{
		RTTI_ENABLE(UniformValue)
	public:
		T mValue = T();
	};


	/**
	 * Base class for list of uniform values.
	 * The list must be declared inside a uniform buffer object (block).
	 * myshader.frag example:
	 *
	 * ~~~~~{.cpp}
	 *	uniform UBO								///< Uniform buffer object
	 *	{
	 *		uniform vec3 positions[10];			///< List of object positions
	 *	} ubo;
	 * ~~~~~
	 */
	class NAPAPI UniformValueArray : public UniformValue
	{
		RTTI_ENABLE(UniformValue)

	public:
		/**
		 * @return The number of elements in this array
		 */
		virtual int getCount() const = 0;
	};


	/**
	 * List of uniform values, for example: float[3], vec3[10] etc.
	 * The list must be declared inside a uniform buffer object (block).
	 * myshader.frag example:
	 * 
	 * ~~~~~{.cpp}
	 *	uniform UBO								///< Uniform buffer object
	 *	{
	 *		uniform vec3 positions[10];			///< List of object position
	 *	} ubo;
	 * ~~~~~
	 */
	template<typename T>
	class TypedUniformValueArray : public UniformValueArray
	{
		RTTI_ENABLE(UniformValueArray)
	public:
		/**
		 * @return total number of elements.
		 */
		virtual int getCount() const override { return mValues.size(); }
		std::vector<T> mValues;
	};


	/**
	 * Base class for list of uniform values.
	 * The list must be declared inside a uniform buffer object (block).
	 * myshader.frag example:
	 *
	 * ~~~~~{.cpp}
	 *	uniform UBO								///< Uniform buffer object
	 *	{
	 *		uniform vec3 positions[10];			///< List of object positions
	 *	} ubo;
	 * ~~~~~
	 */
	class NAPAPI UniformValueBuffer : public UniformValue
	{
		RTTI_ENABLE(UniformValue)

	public:
		/**
		 * @return The number of elements in this array
		 */
		virtual int getCount() const = 0;

		/**
		 * @return Whether a buffer is set
		 */
		virtual bool hasBuffer() const = 0;
	};


	/**
	 * List of uniform values, for example: float[3], vec3[10] etc.
	 * The list must be declared inside a uniform buffer object (block).
	 * myshader.frag example:
	 *
	 * ~~~~~{.cpp}
	 *	uniform UBO								///< Uniform buffer object
	 *	{
	 *		uniform vec3 positions[10];			///< List of object position
	 *	} ubo;
	 * ~~~~~
	 */
	template<typename T>
	class TypedUniformValueBuffer : public UniformValueBuffer
	{
		RTTI_ENABLE(UniformValueBuffer)
	public:
		/**
		 * @return total number of elements.
		 */
		virtual int getCount() const override { return hasBuffer() ? mBuffer->mCount : 0; }

		virtual bool hasBuffer() const override { return mBuffer != nullptr; }

		rtti::ObjectPtr<TypedGPUValueBuffer<T>> mBuffer;	/// Property 'Buffer'
	};


	/**
	 * Find a shader uniform based on the given shader uniform declaration.
	 * @param members uniforms of type nap::Uniform to search through.
	 * @param declaration uniform declaration to match
	 * @return uniform that matches with the given shader declaration, nullptr if not found.
	 */
	template<class T>
	const Uniform* findUniformStructMember(const std::vector<T>& members, const UniformDeclaration& declaration)
	{
		for (auto& member : members)
			if (member->mName == declaration.mName)
				return member.get();
		return nullptr;
	}


	//////////////////////////////////////////////////////////////////////////
	// Uniform value type definitions
	//////////////////////////////////////////////////////////////////////////
	
	using UniformInt = TypedUniformValue<int>;
	using UniformFloat = TypedUniformValue<float>;
	using UniformVec2 = TypedUniformValue<glm::vec2>;
	using UniformVec3 = TypedUniformValue<glm::vec3>;
	using UniformVec4 = TypedUniformValue<glm::vec4>;
	using UniformMat4 = TypedUniformValue<glm::mat4>;


	//////////////////////////////////////////////////////////////////////////
	// Uniform value array type definitions
	//////////////////////////////////////////////////////////////////////////

	using UniformIntArray = TypedUniformValueArray<int>;
	using UniformFloatArray = TypedUniformValueArray<float>;
	using UniformVec2Array = TypedUniformValueArray<glm::vec2>;
	using UniformVec3Array = TypedUniformValueArray<glm::vec3>;
	using UniformVec4Array = TypedUniformValueArray<glm::vec4>;
	using UniformMat4Array = TypedUniformValueArray<glm::mat4>;


	//////////////////////////////////////////////////////////////////////////
	// Uniform value buffer type definitions
	//////////////////////////////////////////////////////////////////////////

	using UniformIntBuffer = TypedUniformValueBuffer<int>;
	using UniformFloatBuffer = TypedUniformValueBuffer<float>;
	using UniformVec2Buffer = TypedUniformValueBuffer<glm::vec2>;
	using UniformVec3Buffer = TypedUniformValueBuffer<glm::vec3>;
	using UniformVec4Buffer = TypedUniformValueBuffer<glm::vec4>;
	using UniformMat4Buffer = TypedUniformValueBuffer<glm::mat4>;
}
