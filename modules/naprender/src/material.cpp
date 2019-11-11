// Local Includes
#include "material.h"
#include "nshader.h"
#include "mesh.h"

// External includes
#include <nap/logger.h>
#include <GL/glew.h>
#include "rtti/rttiutilities.h"
#include "renderservice.h"
#include "nshaderutils.h"

RTTI_BEGIN_ENUM(nap::EBlendMode)
	RTTI_ENUM_VALUE(nap::EBlendMode::NotSet,				"NotSet"),
	RTTI_ENUM_VALUE(nap::EBlendMode::Opaque,				"Opaque"),
	RTTI_ENUM_VALUE(nap::EBlendMode::AlphaBlend,			"AlphaBlend"),
	RTTI_ENUM_VALUE(nap::EBlendMode::Additive,				"Additive")
RTTI_END_ENUM

RTTI_BEGIN_ENUM(nap::EDepthMode)
	RTTI_ENUM_VALUE(nap::EDepthMode::NotSet,				"NotSet"),
	RTTI_ENUM_VALUE(nap::EDepthMode::InheritFromBlendMode,	"InheritFromBlendMode"),
	RTTI_ENUM_VALUE(nap::EDepthMode::ReadWrite,				"ReadWrite"),
	RTTI_ENUM_VALUE(nap::EDepthMode::ReadOnly,				"ReadOnly"),
	RTTI_ENUM_VALUE(nap::EDepthMode::WriteOnly,				"WriteOnly"),
	RTTI_ENUM_VALUE(nap::EDepthMode::NoReadWrite,			"NoReadWrite")
RTTI_END_ENUM

RTTI_BEGIN_STRUCT(nap::Material::VertexAttributeBinding)
	RTTI_VALUE_CONSTRUCTOR(const std::string&, const std::string&)
	RTTI_PROPERTY("MeshAttributeID",			&nap::Material::VertexAttributeBinding::mMeshAttributeID, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("ShaderAttributeID",			&nap::Material::VertexAttributeBinding::mShaderAttributeID, nap::rtti::EPropertyMetaData::Required)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::MaterialInstanceResource)
	RTTI_PROPERTY("Material",					&nap::MaterialInstanceResource::mMaterial,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Uniforms",					&nap::MaterialInstanceResource::mUniforms,	nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("BlendMode",					&nap::MaterialInstanceResource::mBlendMode,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("DepthMode",					&nap::MaterialInstanceResource::mDepthMode,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::MaterialInstance)
	RTTI_FUNCTION("getOrCreateUniform",			(nap::Uniform* (nap::MaterialInstance::*)(const std::string&)) &nap::MaterialInstance::getOrCreateUniform);
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::Material)
	RTTI_PROPERTY("Uniforms",					&nap::Material::mUniforms,					nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Shader",						&nap::Material::mShader,					nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("VertexAttributeBindings",	&nap::Material::mVertexAttributeBindings,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("BlendMode",					&nap::Material::mBlendMode,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("DepthMode",					&nap::Material::mDepthMode,					nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
	std::unique_ptr<UniformSampler> createUniformFromDeclaration(VkDevice device, const opengl::UniformSamplerDeclaration& declaration)
	{
		return std::make_unique<UniformSampler2D>(device, declaration);
	}

	/**
	* Creates Uniform objects based on a declaration.
	*/
	std::unique_ptr<UniformValue> createUniformFromDeclaration(const opengl::UniformValueDeclaration& declaration)
	{
		std::unique_ptr<UniformValue> result;

		switch (declaration.mType)
		{
		case opengl::EGLSLType::Int:
			{
				if (declaration.isArray())
					result = std::make_unique<UniformIntArray>(declaration.mSize);
				else
					result = std::make_unique<UniformInt>();
				
				break;
			}
		case opengl::EGLSLType::Float:
			{
				if (declaration.isArray())
					result = std::make_unique<UniformFloatArray>(declaration.mSize);
				else
					result = std::make_unique<UniformFloat>();

				break;
			}
		case opengl::EGLSLType::Vec4:
			{
				if (declaration.isArray())
					result = std::make_unique<UniformVec4Array>(declaration.mSize);
				else
					result = std::make_unique<UniformVec4>();

				break;
			}
		case opengl::EGLSLType::Mat4:
			{
				if (declaration.isArray())
					result = std::make_unique<UniformMat4Array>(declaration.mSize);
				else
					result = std::make_unique<UniformMat4>();

				break;
			}
		case opengl::EGLSLType::Vec3:
			{
				if (declaration.isArray())
					result = std::make_unique<UniformVec3Array>(declaration.mSize);
				else
					result = std::make_unique<UniformVec3>();

				break;
			}
		}
		assert(result);
		result->mName = declaration.mName;
		result->setDeclaration(declaration);
		return result;
	}

	/**
	 * This is a convenience function to get the number of elements in the specified uniform; it internally does the check to see if it's an 
	 * array of values or array of textures.
	 *
	 * Note that this function is also called for non-array Uniforms; the return value can then be used by the client code to test if the uniform was an 
	 * array or not. Otherwise, the client would have to do the array of values/array of textures check themselves before calling this function,
	 * which would defeat the point.
	 *
	 * @return The number of array elements in the uniform; -1 if the uniform is not an array uniform.
	 */
	static int getNumArrayElements(const Uniform& uniform)
	{
		const UniformValueArray* value_array = rtti_cast<const UniformValueArray>(&uniform);
		if (value_array != nullptr)
			return value_array->getCount();

		const UniformSamplerArray* texture_array = rtti_cast<const UniformSamplerArray>(&uniform);
		if (texture_array != nullptr)
			return texture_array->getNumElements();

		return -1;
	}

	/**
	 * Helper function to verify that the specified uniform matches with the uniform declaration in the shader.
	 * It verifies that the type of uniform (array/non-array) matches with the shader; if it's an array, it also verifies that the lengths match
	 */
	static bool verifyUniform(const UniformValue& uniform, const opengl::UniformValueDeclaration& uniformDeclaration, const std::string& shaderID, utility::ErrorState& errorState)
	{
		// If the declaration is an array, verify that the uniform is also an array and that the sizes match
		if (uniformDeclaration.isArray())
		{
			int numElements = getNumArrayElements(uniform);
			if (!errorState.check(numElements != -1, "Uniform %s is not an array uniform but the variable type in shader %s is.", uniform.mName.c_str(), shaderID.c_str()))
				return false;

			if (!errorState.check(numElements == uniformDeclaration.mSize, "Amount of elements (%d) in uniform %s does not match the amount of elements (%d) declared in shader %s.", numElements, uniform.mName.c_str(), uniformDeclaration.mSize, shaderID.c_str()))
				return false;
		}

		// Verify the uniform types match
		if (!errorState.check(uniform.getGLSLType() == uniformDeclaration.mType, "Uniform %s does not match the variable type in the shader %s", uniform.mName.c_str(), shaderID.c_str()))
			return false;

		return true;
	}

	/**
	 * Helper function to verify that the two uniforms match in size. Note that this is also called for non-array uniforms, in which case it always matches.
	 */
	static bool verifyArrayUniforms(const std::string& fullUniformPath, const Uniform& sourceUniform, const Uniform& destUniform, const std::string& shaderID, utility::ErrorState& errorState)
	{
		int source_size = getNumArrayElements(sourceUniform);
		int dest_size = getNumArrayElements(destUniform);

		if (!errorState.check(source_size <= dest_size, "The number of elements (%d) in uniform %s is higher than the number of elements (%d) declared in shader %s", source_size, fullUniformPath.c_str(), dest_size, shaderID.c_str()))
			return false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// MaterialInstance
	//////////////////////////////////////////////////////////////////////////

	Uniform* MaterialInstance::getOrCreateUniform(const std::string& name)
	{
		Uniform* existing = findUniform(name);
		if (existing != nullptr)
			return existing;

		return &createUniform(name);
	}


	void MaterialInstance::pushUniforms(int frameIndex)
	{
		VkDevice device = getMaterial()->getRenderer().getDevice();

		for (UniformBufferObject& ubo : mUniformBufferObjects)
		{
			void* mapped_memory;
			vkMapMemory(device, ubo.mBuffersMemory[frameIndex], 0, ubo.mDeclaration->mSize, 0, &mapped_memory);
			
			for (auto& kvp : ubo.mUniforms)
			{
				const Uniform* uniform = kvp.second;
				
				const nap::UniformValue* uniform_val = rtti_cast<const nap::UniformValue>(uniform);
				if (uniform_val != nullptr)
				{
					uniform_val->push((uint8_t*)mapped_memory);
				}
			}

			vkUnmapMemory(device, ubo.mBuffersMemory[frameIndex]);
		}

// 		// Keep track of which uniforms were set (i.e. overridden) by the material instance
// 		std::unordered_set<std::string> instance_bindings;
// 		int texture_unit = 0;
// 
// 		// Push all texture uniforms that are set (i.e. overridden) in the instance
// 		const UniformTextureBindings& instance_texture_bindings = getTextureBindings();
// 		for (auto& kvp : instance_texture_bindings)
// 		{
// 			nap::UniformTexture* uniform_tex = rtti_cast<nap::UniformTexture>(kvp.second.mUniform.get());
// 			assert(uniform_tex != nullptr);
// 			texture_unit += uniform_tex->push(*kvp.second.mDeclaration, texture_unit);
// 			instance_bindings.insert(kvp.first);
// 		}
// 
// 		// Push all value uniforms that are set (i.e. overridden) in the instance
// 		const UniformValueBindings& instance_value_bindings = getValueBindings();
// 		for (auto& kvp : instance_value_bindings)
// 		{
// 			nap::UniformValue* uniform_val = rtti_cast<nap::UniformValue>(kvp.second.mUniform.get());
// 			assert(uniform_val != nullptr);
// 			uniform_val->push(*kvp.second.mDeclaration);
// 			instance_bindings.insert(kvp.first);
// 		}
// 
// 		// Push all uniform textures in the material that weren't overridden by the instance
// 		// Note that the material contains mappings for all the possible uniforms in the shader
// 		Material* material = getMaterial();
// 		for (auto& kvp : material->getTextureBindings())
// 		{
// 			if (instance_bindings.find(kvp.first) == instance_bindings.end())
// 			{
// 				nap::UniformTexture* uniform_tex = rtti_cast<nap::UniformTexture>(kvp.second.mUniform.get());
// 				assert(uniform_tex != nullptr);
// 				texture_unit += uniform_tex->push(*kvp.second.mDeclaration, texture_unit);
// 			}
// 		}
// 
// 		// Push all uniform values in the material that weren't overridden by the instance
// 		for (auto& kvp : material->getValueBindings())
// 		{
// 			if (instance_bindings.find(kvp.first) == instance_bindings.end())
// 			{
// 				nap::UniformValue* uniform_val = rtti_cast<nap::UniformValue>(kvp.second.mUniform.get());
// 				assert(uniform_val != nullptr);
// 				uniform_val->push(*kvp.second.mDeclaration);
// 			}
// 		}
// 
// 		glActiveTexture(GL_TEXTURE0);
	}


	void MaterialInstance::pushBlendMode()
	{
		EDepthMode depth_mode = getDepthMode();
		
		// Global
		glDepthFunc(GL_LEQUAL);
		glBlendEquation(GL_FUNC_ADD);

		// Switch based on blend mode
		switch (getBlendMode())
		{
			case EBlendMode::Opaque:
			{
				glDisable(GL_BLEND);
				if (depth_mode == EDepthMode::InheritFromBlendMode)
				{
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_TRUE);
				}
				break;
			}
			case EBlendMode::AlphaBlend:
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				if (depth_mode == EDepthMode::InheritFromBlendMode)
				{
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);
				}
				break;
			}
			case EBlendMode::Additive:
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				if (depth_mode == EDepthMode::InheritFromBlendMode)
				{
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);
				}
				break;
			}
		}

		// If the depth mode is not inherited (based on blend mode) set it.
		if (depth_mode != EDepthMode::InheritFromBlendMode)
		{
			switch (depth_mode)
			{
			case EDepthMode::ReadWrite:
			{
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				break;
			}
			case EDepthMode::ReadOnly:
			{
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				break;
			}
			case EDepthMode::WriteOnly:
			{
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				break;
			}
			case EDepthMode::NoReadWrite:
			{
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				break;
			}
			default:
				assert(false);
			}
		}
	}


	int MaterialInstance::getTextureUnit(nap::UniformSampler& uniform)
	{
// 		int texture_unit = 0;
// 		std::unordered_set<std::string> instance_bindings;
// 
// 		// Iterate over all material instance texture bindings
// 		// If the texture uniform matches the requested uniform it is considered to be at that location
// 		// If not the location is incremented until a match is found
// 		const UniformSamplers& instance_texture_bindings = getUniformSamplers();
// 		for (auto& kvp : instance_texture_bindings)
// 		{
// 			nap::Uniform* uniform_tex = kvp.second.get();
// 			if (uniform_tex == &uniform)
// 				return texture_unit;
// 			texture_unit++;
// 			instance_bindings.insert(kvp.first);
// 		}
// 
// 		// Iterate over all source material bindings
// 		// If the texture uniform matches the requested uniform it is considered to be at that location
// 		// If not, there is no valid texture binding associated with the given uniform
// 		Material* material = getMaterial();
// 		for (auto& kvp : material->getTextureBindings())
// 		{
// 			if (instance_bindings.find(kvp.first) == instance_bindings.end())
// 			{
// 				nap::Uniform* uniform_tex = kvp.second.mUniform.get();
// 				if (uniform_tex == &uniform)
// 					return texture_unit;
// 				texture_unit++;
// 			}
// 		}

		// No texture binding associated with the given uniform
		return -1;
	}

	
	Uniform& MaterialInstance::createUniform(const std::string& name)
	{
		const opengl::UniformValueDeclarations& uniform_declarations = mResource->mMaterial->getShader()->getShader().getUniformValueDeclarations();
		const opengl::UniformSamplerDeclarations& sampler_declarations = mResource->mMaterial->getShader()->getShader().getUniformSamplerDeclarations();

		opengl::UniformValueDeclarations::const_iterator value_pos = uniform_declarations.find(name);
		if (value_pos != uniform_declarations.end())
		{
			const opengl::UniformValueDeclaration* declaration = value_pos->second;
			std::unique_ptr<UniformValue> uniform = createUniformFromDeclaration(*declaration);
			return addUniformValue(std::move(uniform));
		}
		else
		{
			opengl::UniformSamplerDeclarations::const_iterator sampler_pos = sampler_declarations.find(name);
			assert(sampler_pos != sampler_declarations.end());

			const opengl::UniformSamplerDeclaration& declaration = sampler_pos->second;
			std::unique_ptr<UniformSampler> uniform = createUniformFromDeclaration(mDevice, declaration);
			return addUniformSampler(std::move(uniform));
		}
	}

	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		return -1;
	}

	bool createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, utility::ErrorState& errorState) 
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (!errorState.check(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) == VK_SUCCESS, "could not create buffer"))
			return false;

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (!errorState.check(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS, "Could not allocate memory for buffer"))
			return false;

		if (!errorState.check(vkBindBufferMemory(device, buffer, bufferMemory, 0) == VK_SUCCESS, "Could not bind buffer memory"))
			return false;

		return true;
	}

	bool MaterialInstance::init(Renderer& renderer, MaterialInstanceResource& resource, utility::ErrorState& errorState)
	{
		mResource = &resource;
		mDevice = renderer.getDevice();

		Material& material = *resource.mMaterial;
		const opengl::Shader& shader = material.getShader()->getShader();

		const opengl::UniformValueDeclarations& uniform_values = shader.getUniformValueDeclarations();
		const opengl::UniformSamplerDeclarations& uniform_samplers = shader.getUniformSamplerDeclarations();

		// Create new uniforms for all the uniforms in mUniforms
		for (ResourcePtr<Uniform>& uniform : resource.mUniforms)
		{
			if (uniform->get_type().is_derived_from<UniformValue>())
			{
				opengl::UniformValueDeclarations::const_iterator declaration = uniform_values.find(uniform->mName);
				if (declaration == uniform_values.end())
					continue;

				if (!verifyUniform(*rtti_cast<UniformValue>(uniform.get()), *declaration->second, material.getShader()->mID, errorState))
					return false;

				std::unique_ptr<UniformValue> new_uniform = createUniformFromDeclaration(*declaration->second);
				nap::rtti::copyObject(*uniform, *new_uniform.get());

				addUniformValue(std::move(new_uniform));
			}
			else if (uniform->get_type().is_derived_from<UniformSampler>())
			{
				opengl::UniformSamplerDeclarations::const_iterator declaration = uniform_samplers.find(uniform->mName);
				if (declaration == uniform_samplers.end())
					continue;

				std::unique_ptr<UniformSampler> new_uniform = createUniformFromDeclaration(renderer.getDevice(), declaration->second);
				nap::rtti::copyObject(*uniform, *new_uniform.get());

				addUniformSampler(std::move(new_uniform));
			}
		}

		const std::vector<opengl::UniformBufferObjectDeclaration>& ubo_declarations = shader.getUniformBufferObjectDeclarations();

		for (int index = 0; index < ubo_declarations.size(); ++index)
		{
			const opengl::UniformBufferObjectDeclaration& ubo_declaration = ubo_declarations[index];
			
			UniformBufferObject ubo(ubo_declaration);
			for (auto& uniform_declaration : ubo_declaration.mDeclarations)
			{
				const Uniform* uniform = findUniform(uniform_declaration->mName);
				if (uniform == nullptr)
					uniform = material.findUniform(uniform_declaration->mName);

				if (!errorState.check(uniform != nullptr, "Unable to find uniform for declaration"))
					return false;

				ubo.mUniforms.insert(std::make_pair(uniform_declaration->mName, uniform));
			}

			mUniformBufferObjects.emplace_back(std::move(ubo));
		}

		int num_frames = 2;

		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = num_frames * ubo_declarations.size();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = num_frames * uniform_samplers.size() ;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount	= poolSizes.size();
		poolInfo.pPoolSizes		= poolSizes.data();
		poolInfo.maxSets		= num_frames;

		if (!errorState.check(vkCreateDescriptorPool(renderer.getDevice(), &poolInfo, nullptr, &mDescriptorPool) == VK_SUCCESS, "Failed to create descriptor pool"))
			return false;

		std::vector<VkDescriptorSetLayout> layouts(num_frames, getMaterial()->getDescriptorSetLayout());
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool		= mDescriptorPool;
		allocInfo.descriptorSetCount	= num_frames;
		allocInfo.pSetLayouts			= layouts.data();

		mDescriptorSets.resize(num_frames);
		if (!errorState.check(vkAllocateDescriptorSets(renderer.getDevice(), &allocInfo, mDescriptorSets.data()) == VK_SUCCESS, "Failed to create descriptor set"))
			return false;

		int num_descriptors = (num_frames * ubo_declarations.size()) + (num_frames * uniform_samplers.size());
		std::vector<VkWriteDescriptorSet> descriptor_writes;
		descriptor_writes.resize(num_descriptors);

		std::vector<VkDescriptorBufferInfo> descriptor_buffers(num_descriptors);
		descriptor_buffers.resize(num_descriptors);

		int descriptor_index = 0;
		for (size_t frame_index = 0; frame_index < num_frames; frame_index++)
		{
			for (int buffer_index = 0; buffer_index != ubo_declarations.size(); ++buffer_index)
			{
				VkBuffer vkBuffer;
				VkDeviceMemory vkBufferMemory;

				if (!createBuffer(renderer.getDevice(), renderer.getPhysicalDevice(), ubo_declarations[buffer_index].mSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkBuffer, vkBufferMemory, errorState))
					return false;

				UniformBufferObject& ubo = mUniformBufferObjects[buffer_index];
				ubo.mBuffers.push_back(vkBuffer);
				ubo.mBuffersMemory.push_back(vkBufferMemory);

				VkDescriptorBufferInfo& bufferInfo = descriptor_buffers[descriptor_index];
				bufferInfo.buffer = ubo.mBuffers.back();
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;

				VkWriteDescriptorSet& write_descriptor_set = descriptor_writes[descriptor_index];
				write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write_descriptor_set.dstSet = mDescriptorSets[frame_index];
				write_descriptor_set.dstBinding = ubo_declarations[buffer_index].mBinding;
				write_descriptor_set.dstArrayElement = 0;
				write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write_descriptor_set.descriptorCount = 1;
				write_descriptor_set.pBufferInfo = &bufferInfo;

				descriptor_index++;
			}

			for (auto& sampler_declaration : uniform_samplers)
			{
				const UniformSampler2D* uniform = findUniform<UniformSampler2D>(sampler_declaration.second.mName);
				if (uniform == nullptr)
					uniform = material.findUniform<UniformSampler2D>(sampler_declaration.second.mName);

				if (!errorState.check(uniform != nullptr, "Unable to find uniform for declaration"))
					return false;

				if (!errorState.check(uniform->mTexture != nullptr, "No texture set for uniform"))
					return false;

				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView		= uniform->mTexture->getImageView();
				imageInfo.sampler		= uniform->getSampler();

				VkWriteDescriptorSet& write_descriptor_set = descriptor_writes[descriptor_index];
				write_descriptor_set.sType	= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write_descriptor_set.dstSet = mDescriptorSets[frame_index];
				write_descriptor_set.dstBinding = sampler_declaration.second.mBinding;
				write_descriptor_set.dstArrayElement = 0;
				write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				write_descriptor_set.descriptorCount = 1;
				write_descriptor_set.pImageInfo = &imageInfo;

				descriptor_index++;
			}
		}

		vkUpdateDescriptorSets(renderer.getDevice(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

		return true;
	}


	Material* MaterialInstance::getMaterial() 
	{ 
		return mResource->mMaterial.get(); 
	}


	EBlendMode MaterialInstance::getBlendMode() const
	{
		if (mResource->mBlendMode != EBlendMode::NotSet)
			return mResource->mBlendMode;

		return mResource->mMaterial->getBlendMode();
	}


	void MaterialInstance::setBlendMode(EBlendMode blendMode)
	{
		mResource->mBlendMode = blendMode;
	}

	
	void MaterialInstance::setDepthMode(EDepthMode depthMode)
	{
		mResource->mDepthMode = depthMode;
	}


	EDepthMode MaterialInstance::getDepthMode() const
	{
		if (mResource->mDepthMode != EDepthMode::NotSet)
			return mResource->mDepthMode;

		return mResource->mMaterial->getDepthMode();
	}


	//////////////////////////////////////////////////////////////////////////
	// Material
	//////////////////////////////////////////////////////////////////////////

	Material::Material(RenderService& renderService) :
		mRenderer(&renderService.getRenderer())
	{
	}

	UniformStruct& Material::getOrCreateUniformStruct(const std::string& globalName, const std::string& localName, bool& created)
	{
		auto pos = mOwnedStructUniforms.find(globalName);
		if (pos == mOwnedStructUniforms.end())
		{
			auto inserted = mOwnedStructUniforms.insert(std::make_pair(globalName, std::make_unique<UniformStruct>()));
			inserted.first->second->mName = localName;
			created = true;
			return *inserted.first->second;
		}

		created = false;
		return *pos->second;
	}


	UniformStructArray& Material::getOrCreateUniformStructArray(const std::string& globalName, const std::string& localName, bool& created)
	{
		auto pos = mOwnedStructArrayUniforms.find(globalName);
		if (pos == mOwnedStructArrayUniforms.end())
		{
			auto inserted = mOwnedStructArrayUniforms.insert(std::make_pair(globalName, std::make_unique<UniformStructArray>()));
			inserted.first->second->mName = localName;
			created = true;
			return *inserted.first->second;
		}

		created = false;
		return *pos->second;
	}


	/**
	 * This function recursively creates uniform for the specified declaration. It correctly deals with regular uniforms, arrays of uniforms, structures and all combinations.
	 * It works by inspecting the 'path' to a particular uniform. For example, consider the following shader code:
	 *
	 * struct Point
	 * {
	 *     float x;
	 *     float y;
	 * };
	 *
	 * struct Light
	 * {
	 *     Point		mPoint;
	 *     float 		mTest[2];
	 *     sampler2D 	mTestSampler[2];
	 * };
	 *
     * uniform Light mLight[2];
	 *
	 * This will result in a number of shader declarations that looks like the following (assuming all uniforms are actually used by the shader):
	 *
	 * mLight[0].mPoint.x
	 * mLight[0].mPoint.y
	 * mLight[0].mTest[0]
	 * mLight[1].mPoint.x
	 * mLight[1].mPoint.y
	 * mLight[1].mTest[0]
	 *
	 * Note a few things:
	 * - There are only declarations for the 'leaf' elements, i.e. the actual uniforms. There are no separate declarations for intermediate structs or arrays that are on the 'path'
	 * - Leaf arrays (mTest in the above example) only have a single declaration, not one per element. The size of the declaration will be set to the number of elements in the array.
	 * - Non-leaf arrays (mLight in the above example) have declaration for each used element
	 *
	 * While recursing into each element, we'll create the appropriate type of element (Uniform, UniformStructArray or UniformStruct) to represent each 'part'. 
	 * The name given to each uniform will be the name of the current part we're in, without the path and without the brackets.  In the above example, 
	 * the uniform 'mLight[0].mTest[0]' will be given the name 'mTest'. 
	 *
	 * This naming scheme was chosen to ensure that the user doesn't need to specify the full path when declaring these uniforms in the material; the full path can always 
	 * be deduced (during the 'apply') phase as we recursively go through the uniforms.
	 */
	Uniform* Material::addUniformRecursive(const opengl::UniformValueDeclaration& declaration, const std::string& path, const std::vector<std::string>& parts, int partIndex, bool& didCreateUniform)
	{
		assert(partIndex < parts.size());

		const std::string& part = parts[partIndex];

		// Determine the type of the uniform. The cases we're interested in are:
		// - There is a bracket present in the current part. This must mean that we're currently in an array
		// - The path consists of multiple elements and we're not at the last element yet. This must mean that we're currently in a struct, otherwise there wouldn't be multiple parts in the first place.
		//
		// Note that it is possible to both be in an array and in a struct (i.e. array of structures).
		size_t bracketPos = part.find_first_of('[');
		bool isArray = bracketPos != std::string::npos;
		bool isStruct = partIndex < parts.size() - 1;

		// Build up the full path as we go. This allows us to give each uniform we construct along the way a unique 'name'
		std::string currentPath = path;
		if (!currentPath.empty())
			currentPath += ".";

		currentPath += part;

		// If the part is both an array and a struct, we need to create multiple elements:
		// - An element to represent the array (UniformStructArray)
		// - An element to represent the struct inside the array (UniformStruct)
		if (isArray && isStruct)
		{
			// The local name for a uniform is the part without the brackets and without the full path (i.e. mLight in the above example)
			std::string local_name = part.substr(0, bracketPos);

			// Determine the index of the array we're currently at. We do this by looking for the last opening bracket and assuming it's followed by a number.
			size_t arrayStartPos = currentPath.find_last_of('[');
			assert(arrayStartPos != std::string::npos);

			int array_index = (int)strtol(currentPath.data() + arrayStartPos + 1, nullptr, 10);			

			// Ensure there's an element to represent the array
			UniformStructArray& array = getOrCreateUniformStructArray(currentPath.substr(0, arrayStartPos), local_name, didCreateUniform);			

			// Note that since non-leaf arrays can be present in multiple declarations, we can

			// Ensure there's an element to represent the struct at this index
			bool did_create_child_struct = false;
			UniformStruct& child_struct = getOrCreateUniformStruct(currentPath, part, did_create_child_struct);

			// Since the array / struct can be present in multiple declarations (see the x, y members of the point struct in the above example),
			// we only want to add the struct into the array if this is the first time we're seeing it (i.e. we just created it). Otherwise, we would end up with duplicates.
			if (did_create_child_struct)
				array.insertStruct(array_index, child_struct);

			// Recurse into struct to add the uniform
			bool did_create_child = false;
			Uniform* child_uniform = addUniformRecursive(declaration, currentPath, parts, partIndex + 1, did_create_child);
			assert(child_uniform != nullptr);

			// If we created a new uniform, add it to the struct. Note that we need to deal with duplicates here again, in cases of nested structs.
			if (did_create_child)
				child_struct.addUniform(*child_uniform);

			return &array;
		}
		else if (isStruct)
		{
			// The part is just a struct; we can recurse directly into it
			UniformStruct& uniform_struct = getOrCreateUniformStruct(currentPath, part, didCreateUniform);

			bool did_create_child = false;
			Uniform* child_uniform = addUniformRecursive(declaration, currentPath, parts, partIndex + 1, did_create_child);
			assert(child_uniform != nullptr);

			// If we created a new uniform, add it to the struct. Note that we need to deal with duplicates here again, in cases of nested structs.
			if (did_create_child)
				uniform_struct.addUniform(*child_uniform);

			return &uniform_struct;
		}
		else
		{
			// Create a new uniform from the declaration and set its name
			std::unique_ptr<UniformValue> new_uniform = createUniformFromDeclaration(declaration);
			new_uniform->mName = part;

			// Add the container
			Uniform* result = new_uniform.get();
			addUniformValue(std::move(new_uniform));

			didCreateUniform = true;
			return result;
		}
	}


	/**
	 * Recursively apply uniforms in the source struct onto the destination struct
	 */
	bool applyUniformsRecursive(const std::string& parentUniformPath, const std::string& shaderID, UniformStruct& sourceStruct, UniformStruct& destStruct, utility::ErrorState& errorState)
	{
		for (auto& source_uniform : sourceStruct.mUniforms)
		{
			std::string current_uniform_path = utility::stringFormat("%s.%s", parentUniformPath.c_str(), source_uniform->mName.c_str());

			// All uniforms in the source (i.e. defined in the material) should also be present in the destination (i.e. defined in shader)
			Uniform* dest_uniform = destStruct.findUniform(source_uniform->mName);
			if (!errorState.check(dest_uniform != nullptr, "Uniform '%s' could not be matched with an uniform in shader %s. Perhaps the uniform is not used in the shader or the name is incorrect.", current_uniform_path.c_str(), shaderID.c_str()))
			{
				nap::Logger::warn(errorState.toString());
				continue;
			}

			// Types must match
			if (!errorState.check(source_uniform->get_type() == dest_uniform->get_type(), "Mismatch between types for uniform '%s' (source type = %s, target type = %s) in shader %s",
								  current_uniform_path.c_str(), source_uniform.get_type().get_name().data(), dest_uniform->get_type().get_name().data(), shaderID.c_str()))
			{
				return false;
			}

			// If the uniform is an array of structures, recursively apply the nested uniforms
			UniformStructArray* source_struct_array = rtti_cast<UniformStructArray>(source_uniform.get());
			if (source_struct_array != nullptr)
			{
				UniformStructArray* dest_struct_array = (UniformStructArray*)dest_uniform;

				// Size of the source must be <= the size in the dest (shader)
				size_t source_size = source_struct_array->mStructs.size();
				size_t dest_size = dest_struct_array->mStructs.size();
				if (!errorState.check(source_size <= dest_size, "The number of elements (%d) in uniform %s exceeds the number of elements (%d) declared in shader %s", source_size, current_uniform_path.c_str(), dest_size, shaderID.c_str()))
					return false;

				// Recurse
				for (int index = 0; index < std::min(source_size, dest_size); ++index)
				{
					std::string array_uniform_path = utility::stringFormat("%s[%d]", current_uniform_path.c_str(), index);

					UniformStruct* source_uniform_struct = rtti_cast<UniformStruct>(source_struct_array->mStructs[index].get());
					UniformStruct* dest_uniform_struct = rtti_cast<UniformStruct>(dest_struct_array->mStructs[index].get());

					if (!applyUniformsRecursive(array_uniform_path, shaderID, *source_uniform_struct, *dest_uniform_struct, errorState))
						return false;
				}
			}
			else
			{
				// If the uniform is a struct, just recurse
				UniformStruct* source_struct = rtti_cast<UniformStruct>(source_uniform.get());
				if (source_struct != nullptr)
				{
					UniformStruct* dest_struct = (UniformStruct*)dest_uniform;
					if (!applyUniformsRecursive(current_uniform_path, shaderID, *source_struct, *dest_struct, errorState))
						return false;
				}
				else
				{
					// Regular uniform; verify array lengths match if it's an array uniform
					if (!verifyArrayUniforms(current_uniform_path, *source_uniform, *dest_uniform, shaderID, errorState))
						return false;

					// Copy the properties of the uniform as defined in the material over the default-constructed uniform
					nap::rtti::copyObject(*source_uniform, *dest_uniform);
				}
			}
		}

		return true;
	}


	/**
	 * The Material init will initialize all uniforms that can be used with the bound shader. The shader contains the authoritative set of Uniforms that can be set;
	 * the Uniforms defined in the material must match the Uniforms declared by the shader. If the shader declares a Uniform that is not present in the Material, a 
	 * default Uniform will be used. 
	 
	 * To prevent us from having to check everywhere whether a uniform is present in the material or not, we create Uniforms for *all* uniforms
	 * declared by the shader, even if they're present in the material. Then, if the Uniform is also present in the material, we simply copy the existing uniform over the
	 * newly-constructed uniform. Furthermore, it is important to note why we always create new uniforms and do not touch the input data:
	 *
	 * - Because we create default uniforms with default values, the ownership of those newly created objects
	 *   lies within the Material. It is very inconvenient if half of the Uniforms is owned by the system and
	 *   half of the Uniforms by Material. Instead, Material owns all of them.
	 * - It is important to maintain a separation between the input data (mUniforms) and the runtime data. Json objects
	 *   should always be considered constant.
	 * - The separation makes it easy to build faster mappings for textures and values, and to provide a map interface
	 *   instead of a vector interface (which is what is supported at the moment for serialization).
	 *
	 * Thus, the Material init happens in two passes:
	 * - First, we create uniforms for all uniforms declared in the shader. This is a recursive process, due to the presence of arrays/struct uniforms
	 * - Then, we apply all uniforms that are present in the material (mUniforms) onto the newly-constructed uniforms. This is also a recursive process.
	 *
	 * Note that the first pass creates a 'tree' of uniforms (arrays can contain structs, which can contains uniforms, etc); the tree of uniforms defined in the material 
	 * must match the tree generated in the first pass.
	 */
	bool Material::init(utility::ErrorState& errorState)
	{
		if (!errorState.check(mShader != nullptr, "Shader not set in material %s", mID.c_str()))
			return false;

		// First pass: recursively create uniforms for all declarations present in the shader.
		const opengl::UniformValueDeclarations& uniform_declarations = mShader->getShader().getUniformValueDeclarations();
		for (auto& kvp : uniform_declarations)
		{
			const opengl::UniformValueDeclaration& declaration = *kvp.second;

			std::vector<std::string> parts = utility::splitString(declaration.mName, '.');

			bool did_create_uniform = false;
			addUniformRecursive(declaration, "", parts, 0, did_create_uniform);
		}

		const opengl::UniformSamplerDeclarations& sampler_declarations = mShader->getShader().getUniformSamplerDeclarations();
		for (auto& kvp : sampler_declarations)
		{
			const opengl::UniformSamplerDeclaration& declaration = kvp.second;

			// Create a new uniform from the declaration and set its name
			std::unique_ptr<UniformSampler> new_uniform = createUniformFromDeclaration(mRenderer->getDevice(), declaration);

			// Add the container
			Uniform* result = new_uniform.get();
			addUniformSampler(std::move(new_uniform));
		}

		// Second pass: recursively apply uniforms present in the material on top of the uniforms we created in the first pass
		// Note that the list of uniforms in mUniforms only contains the 'root' uniforms; any uniforms within a struct or array will be contained within 
		// the corresponding UniformStructArray or UniformStruct.
		for (ResourcePtr<Uniform>& uniform : mUniforms)
		{
			// If the uniform is a an array of structures, we need to go through all elements (structures) in the array and recursively apply the uniforms inside each element
			UniformStructArray* source_uniform_struct_array = rtti_cast<UniformStructArray>(uniform.get());
			if (source_uniform_struct_array != nullptr)
			{
				// There must be a uniform with matching name
				UniformStructArrayMap::iterator pos = mOwnedStructArrayUniforms.find(source_uniform_struct_array->mName);
				if (!errorState.check(pos != mOwnedStructArrayUniforms.end(), "Structure array uniform '%s' could not be found in shader %s: either it doesn't exist, or the type doesn't match", source_uniform_struct_array->mName.c_str(), mID.c_str()))
					return false;

				UniformStructArray* dest_uniform_struct_array = pos->second.get();

				// The uniform defined by the user must have at most the number of elements in the shader. Less is fine, since it will just mean that the undefined elements
				// will get default values.
				size_t source_size = source_uniform_struct_array->mStructs.size();
				size_t dest_size = dest_uniform_struct_array->mStructs.size();
				if (!errorState.check(source_size <= dest_size, "The number of elements (%d) in uniform %s exceeds the number of elements (%d) declared in shader %s", source_size, source_uniform_struct_array->mName.c_str(), dest_size, mID.c_str()))
					return false;

				// Apply recursively
				for (int index = 0; index < std::min(source_size, dest_size); ++index)
				{
					std::string path = utility::stringFormat("%s[%d]", source_uniform_struct_array->mName.c_str(), index);

					UniformStruct* source_uniform_struct = rtti_cast<UniformStruct>(source_uniform_struct_array->mStructs[index].get());
					UniformStruct* dest_uniform_struct = rtti_cast<UniformStruct>(dest_uniform_struct_array->mStructs[index].get());

					if (!applyUniformsRecursive(path, mID, *source_uniform_struct, *dest_uniform_struct, errorState))
						return false;
				}
			}
			else
			{
				// If the uniform is a structure, apply recursively
				UniformStruct* source_uniform_struct = rtti_cast<UniformStruct>(uniform.get());
				if (source_uniform_struct != nullptr)
				{
					// There must be a uniform with matching name
					UniformStructMap::iterator pos = mOwnedStructUniforms.find(source_uniform_struct->mName);
					if (!errorState.check(pos != mOwnedStructUniforms.end(), "Unable to match uniform '%s' with shader", source_uniform_struct->mName.c_str()))
						return false;

					// Recurse
					UniformStruct* dest_uniform_struct = pos->second.get();
					if (!applyUniformsRecursive(source_uniform_struct->mName, mID, *source_uniform_struct, *dest_uniform_struct, errorState))
						return false;
				}
				else
				{
					// Regular uniform; there must be a matching uniform
					Uniform* dest_uniform = findUniform(uniform->mName);
					if (!errorState.check(dest_uniform != nullptr, "Uniform '%s' could not be matched with an uniform in shader %s. Perhaps the uniform is not used in the shader or the name is incorrect.", uniform->mName.c_str(), mID.c_str()))
					{
						nap::Logger::warn(errorState.toString());
						continue;
					}

					// If the uniform is an array uniform, verify that the lengths match
					if (!verifyArrayUniforms(dest_uniform->mName, *uniform, *dest_uniform, mID, errorState))
						return false;

					// Copy the properties of the uniform as defined in the material over the default-constructed uniform
					nap::rtti::copyObject(*uniform, *dest_uniform);
				}
			}
		}

		const std::vector<opengl::UniformBufferObjectDeclaration>& ubo_declarations = mShader->getShader().getUniformBufferObjectDeclarations();

		std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layouts;
		for (int index = 0; index < ubo_declarations.size(); ++index)
		{
			const opengl::UniformBufferObjectDeclaration& uniform_buffer_object = ubo_declarations[index];

			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding			= uniform_buffer_object.mBinding;
			uboLayoutBinding.descriptorCount	= 1;
			uboLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.pImmutableSamplers = nullptr;
			uboLayoutBinding.stageFlags			= uniform_buffer_object.mStage;

			descriptor_set_layouts.push_back(uboLayoutBinding);
		}

		for (auto& kvp : sampler_declarations)
		{
			const opengl::UniformSamplerDeclaration& declaration = kvp.second;
			
			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			samplerLayoutBinding.binding			= declaration.mBinding;
			samplerLayoutBinding.descriptorCount	= 1;
			samplerLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags			= declaration.mStage;

			descriptor_set_layouts.push_back(samplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount	= (int)descriptor_set_layouts.size();
		layoutInfo.pBindings	= descriptor_set_layouts.data();

		if (!errorState.check(vkCreateDescriptorSetLayout(mRenderer->getDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS, "Failed to create descriptor set layout"))
			return false;

		return true;
	}


	const std::vector<Material::VertexAttributeBinding>& Material::sGetDefaultVertexAttributeBindings()
	{
		static std::vector<Material::VertexAttributeBinding> bindings;
		if (bindings.empty())
		{
			bindings.push_back({ VertexAttributeIDs::getPositionName(),		opengl::Shader::VertexAttributeIDs::getPositionVertexAttr() });
			bindings.push_back({ VertexAttributeIDs::getNormalName(),		opengl::Shader::VertexAttributeIDs::getNormalVertexAttr() });
			bindings.push_back({ VertexAttributeIDs::getTangentName(),		opengl::Shader::VertexAttributeIDs::getTangentVertexAttr() });
			bindings.push_back({ VertexAttributeIDs::getBitangentName(),	opengl::Shader::VertexAttributeIDs::getBitangentVertexAttr() });

			const int numChannels = 4;
			for (int channel = 0; channel != numChannels; ++channel)
			{
				bindings.push_back({ VertexAttributeIDs::GetColorName(channel), opengl::Shader::VertexAttributeIDs::getColorVertexAttr(channel) });
				bindings.push_back({ VertexAttributeIDs::getUVName(channel),	opengl::Shader::VertexAttributeIDs::getUVVertexAttr(channel) });
			}
		}
		return bindings;
	}

	const Material::VertexAttributeBinding* Material::findVertexAttributeBinding(const std::string& shaderAttributeID) const
	{
		// If no bindings are specified at all, use the default bindings. Note that we don't just initialize mVertexAttributeBindings to the default in init(),
		// because that would modify this object, which would cause the object diff during real-time editing to flag this object as 'modified', even though it's not.
		const std::vector<VertexAttributeBinding>& bindings = !mVertexAttributeBindings.empty() ? mVertexAttributeBindings : sGetDefaultVertexAttributeBindings();
		for (const VertexAttributeBinding& binding : bindings)
		{
			if (binding.mShaderAttributeID == shaderAttributeID)
			{
				return &binding;
			}
		}
		return nullptr;
	}
}