#include "propertymapper.h"

#include <QStringList>
#include <napqt/filterpopup.h>
#include <rtti/path.h>
#include <renderglobals.h>
#include <appcontext.h>

namespace napkin
{
	MaterialPropertyMapper::MaterialPropertyMapper(const PropertyPath& propPath, nap::BaseMaterial& material) :
		mPath(propPath), mMaterial(&material)
	{
		// Fetch shader using RTTI
		auto property_path = nap::rtti::Path::fromString(nap::material::shader);
		nap::rtti::ResolvedPath resolved_path;
		property_path.resolve(mMaterial, resolved_path);
		assert(resolved_path.isValid());
		nap::rtti::Variant prop_value = resolved_path.getValue();
		assert(prop_value.get_type().is_wrapper());
		mShader = prop_value.extract_wrapped_value().get_value<nap::BaseShader*>();
	}


	void MaterialPropertyMapper::map(QWidget* parent)
	{
		// Shader must be assigned
		if (mShader == nullptr)
		{
			nap::Logger::warn("Can't map '%s', missing shader", mPath.getName().c_str());
			return;
		}

		// Now handle the various mapping types
		if (mPath.getName() == nap::material::uniforms)
		{
			const auto* dec = selectVariableDeclaration(mShader->getUBODeclarations(), parent);
			if (dec != nullptr)
				addVariableBinding(*dec, mPath);
		}
		else if (mPath.getName() == nap::material::samplers)
		{
			const auto* dec = selectSamplerDeclaration(parent);
			if (dec != nullptr)
				addSamplerBinding(*dec, mPath);
		}
		else if (mPath.getName() == nap::material::buffers)
		{
			const auto* dec = selectBufferDeclaration(mShader->getSSBODeclarations(), parent);
			if (dec != nullptr)
				addBufferBinding(dec->getBufferDeclaration(), mPath);
		}
	}


	bool MaterialPropertyMapper::mappable() const
	{
		return mShader != nullptr;
	}


	const nap::ShaderVariableDeclaration* MaterialPropertyMapper::selectVariableDeclaration(const nap::BufferObjectDeclarationList& list, QWidget* parent)
	{
		QStringList names;
		std::unordered_map<std::string, const nap::ShaderVariableDeclaration*> dec_map;
		dec_map.reserve(list.size());
		for (const auto& dec : list)
		{
			if (dec.mName != nap::uniform::mvpStruct)
			{
				dec_map.emplace(dec.mName, &dec);
				names << QString::fromStdString(dec.mName);
			}
		}
		auto selection = nap::qt::FilterPopup::show(parent, names);
		return selection.isEmpty() ? nullptr : dec_map[selection.toStdString()];
	}


	const nap::SamplerDeclaration* MaterialPropertyMapper::selectSamplerDeclaration(QWidget* parent)
	{
		QStringList names;
		const auto& shader_decs = mShader->getSamplerDeclarations();
		std::unordered_map<std::string, const nap::SamplerDeclaration*> dec_map;
		dec_map.reserve(shader_decs.size());
		for (const auto& dec : shader_decs)
		{
			dec_map.emplace(dec.mName, &dec);
			names << QString::fromStdString(dec.mName);
		}
		auto selection = nap::qt::FilterPopup::show(parent, names);
		return selection.isEmpty() ? nullptr : dec_map[selection.toStdString()];
	}


	template<typename T>
	static T* createBinding(const std::string& name, const nap::rtti::TypeInfo& uniformType, const PropertyPath& propertyPath, Document& doc)
	{
		// Create uniform struct
		assert(propertyPath.isArray());
		int iidx = propertyPath.getArrayLength();
		int oidx = doc.arrayAddNewObject(propertyPath, uniformType, iidx);
		assert(iidx == oidx);

		// Fetch created uniform
		auto uni_path = propertyPath.getArrayElement(oidx);
		auto uni_value = uni_path.getValue();
		assert(uni_value.get_type().is_wrapper());
		auto* uni_obj = uni_value.extract_wrapped_value().get_value<T*>();
		assert(uni_obj != nullptr);

		// Assign name and ID
		uni_obj->mName = name;
		doc.setObjectName(*uni_obj, name, true);
		return uni_obj;
	}


	void MaterialPropertyMapper::addSamplerBinding(const nap::SamplerDeclaration& declaration, const PropertyPath& propPath)
	{
		// Get document
		auto* doc = AppContext::get().getDocument();
		assert(doc != nullptr);

		// Only 2D samplers are supports
		if (declaration.mType != nap::SamplerDeclaration::EType::Type_2D)
		{
			nap::Logger::warn("Data type of shader variable %s is not supported", declaration.mName.c_str());
			return;
		}

		// Create binding
		bool is_array = declaration.mNumArrayElements > 1;
		nap::rtti::TypeInfo sampler_type = is_array ? RTTI_OF(nap::Sampler2DArray) : RTTI_OF(nap::Sampler2D);
		createBinding<nap::Sampler>(declaration.mName, sampler_type, propPath, *doc);
	}

	
	void MaterialPropertyMapper::addVariableBinding(const nap::ShaderVariableDeclaration& declaration, const PropertyPath& path)
	{
		// Get document
		auto* doc = AppContext::get().getDocument();
		assert(doc != nullptr);

		// Handle struct declaration
		auto dec_type = declaration.get_type();
		if (dec_type.is_derived_from(RTTI_OF(nap::ShaderVariableStructDeclaration)))
		{
			// Create struct binding
			auto* new_uniform = createBinding<nap::UniformStruct>(declaration.mName, RTTI_OF(nap::UniformStruct), path, *doc);

			// Get path to members property
			PropertyPath members_path(*new_uniform,
				new_uniform->get_type().get_property(nap::uniform::uniforms), *doc);

			// Add variable binding for every member
			const auto& struct_dec = static_cast<const nap::ShaderVariableStructDeclaration&>(declaration);
			for (const auto& member_dec : struct_dec.mMembers)
				addVariableBinding(*member_dec, members_path);
		}

		// Handle value declaration
		if (dec_type.is_derived_from(RTTI_OF(nap::ShaderVariableValueDeclaration)))
		{
			const auto& value_dec = static_cast<const nap::ShaderVariableValueDeclaration&>(declaration);
			static const std::unordered_map<nap::EShaderVariableValueType, nap::rtti::TypeInfo> vuni_map =
			{
				{ nap::EShaderVariableValueType::Float,	RTTI_OF(nap::UniformFloat)	},
				{ nap::EShaderVariableValueType::Int,	RTTI_OF(nap::UniformInt)	},
				{ nap::EShaderVariableValueType::UInt,	RTTI_OF(nap::UniformUInt)	},
				{ nap::EShaderVariableValueType::Vec2,	RTTI_OF(nap::UniformVec2)	},
				{ nap::EShaderVariableValueType::Vec3,	RTTI_OF(nap::UniformVec3)	},
				{ nap::EShaderVariableValueType::Vec4,	RTTI_OF(nap::UniformVec4)	},
				{ nap::EShaderVariableValueType::IVec4,	RTTI_OF(nap::UniformIVec4)	},
				{ nap::EShaderVariableValueType::UVec4,	RTTI_OF(nap::UniformUVec4)	},
				{ nap::EShaderVariableValueType::Mat4,	RTTI_OF(nap::UniformMat4)	}
			};

			// Make sure the declared type is supported
			// TODO: Add support for Mat2 & Mat3
			auto found_it = vuni_map.find(value_dec.mType);
			if (found_it == vuni_map.end())
			{
				nap::Logger::warn("Data type of shader variable %s is not supported", value_dec.mName.c_str());
				return;
			}

			// Create and add value binding
			createBinding<nap::UniformValue>(declaration.mName, found_it->second, path, *doc);
		}

		// Handle value array declaration
		if (dec_type.is_derived_from(RTTI_OF(nap::ShaderVariableValueArrayDeclaration)))
		{
			const auto& array_dec = static_cast<const nap::ShaderVariableValueArrayDeclaration&>(declaration);
			static const std::unordered_map<nap::EShaderVariableValueType, nap::rtti::TypeInfo> vuni_map =
			{
				{ nap::EShaderVariableValueType::Float,	RTTI_OF(nap::UniformFloatArray)	},
				{ nap::EShaderVariableValueType::Int,	RTTI_OF(nap::UniformIntArray)	},
				{ nap::EShaderVariableValueType::UInt,	RTTI_OF(nap::UniformUIntArray)	},
				{ nap::EShaderVariableValueType::Vec2,	RTTI_OF(nap::UniformVec2Array)	},
				{ nap::EShaderVariableValueType::Vec3,	RTTI_OF(nap::UniformVec3Array)	},
				{ nap::EShaderVariableValueType::Vec4,	RTTI_OF(nap::UniformVec4Array)	},
				{ nap::EShaderVariableValueType::IVec4,	RTTI_OF(nap::UniformIVec4Array)	},
				{ nap::EShaderVariableValueType::UVec4,	RTTI_OF(nap::UniformUVec4Array)	},
				{ nap::EShaderVariableValueType::Mat4,	RTTI_OF(nap::UniformMat4Array)	}
			};

			// Make sure the declared type is supported
			// TODO: Add support for Mat2 & Mat3
			auto found_it = vuni_map.find(array_dec.mElementType);
			if (found_it == vuni_map.end())
			{
				nap::Logger::warn("Data type of shader variable %s is not supported", array_dec.mName.c_str());
				return;
			}

			// Create and add value binding
			auto* array_uniform = createBinding<nap::UniformValueArray>(declaration.mName, found_it->second, path, *doc);

			// Get path to values property
			PropertyPath values_path(*array_uniform,
				array_uniform->get_type().get_property(nap::uniform::values), *doc);

			// Add value entries
			for (int i = 0; i < array_dec.mNumElements; i++)
				doc->arrayAddValue(values_path);
		}

		// Handle struct array
		if (dec_type.is_derived_from(RTTI_OF(nap::ShaderVariableStructArrayDeclaration)))
		{
			const auto& array_dec = static_cast<const nap::ShaderVariableStructArrayDeclaration&>(declaration);

			// Create and add value binding
			auto* struct_uni = createBinding<nap::UniformStructArray>(declaration.mName, RTTI_OF(nap::UniformStructArray), path, *doc);

			// Get path to structs property
			PropertyPath structs_path(*struct_uni,
				struct_uni->get_type().get_property(nap::uniform::structs), *doc);

			// Add value entries
			for (const auto& entry : array_dec.mElements)
				addVariableBinding(*entry, structs_path);
		}
	}


	const nap::BufferObjectDeclaration* MaterialPropertyMapper::selectBufferDeclaration(const nap::BufferObjectDeclarationList& list, QWidget* parent)
	{
		QStringList names;
		std::unordered_map<std::string, const nap::BufferObjectDeclaration*> dec_map;
		dec_map.reserve(list.size());
		for (const auto& dec : list)
		{
			dec_map.emplace(dec.mName, &dec);
			names << QString::fromStdString(dec.mName);
		}
		auto selection = nap::qt::FilterPopup::show(parent, names);
		return selection.isEmpty() ? nullptr : dec_map[selection.toStdString()];
	}


	void MaterialPropertyMapper::addBufferBinding(const nap::ShaderVariableDeclaration& declaration, const PropertyPath& propPath)
	{


		// Get document
		auto* doc = AppContext::get().getDocument();
		assert(doc != nullptr);
	}
}
