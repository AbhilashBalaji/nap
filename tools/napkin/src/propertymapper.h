/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local includes
#include "propertypath.h"

// External Includes
#include <material.h>
#include <materialinstance.h>

namespace napkin
{
	/**
	 * Allows the user to select and create a property based on available shader bindings.
	 * Works for every type of material and material instance resource.
	 */
	class MaterialPropertyMapper final
	{
	public:
		/**
		 * Constructor
		 */
		MaterialPropertyMapper(const PropertyPath& propertyPath);

		/**
		 * Copy is not allowed
		 */
		MaterialPropertyMapper(MaterialPropertyMapper&) = delete;

		/**
		 * Copy assignment is not allowed
		 */
		MaterialPropertyMapper& operator=(const MaterialPropertyMapper&) = delete;

		/**
		 * Move is not allowed
		 */
		MaterialPropertyMapper(MaterialPropertyMapper&&) = delete;

		/**
		 * Move assignment is not allowed
		 */
		MaterialPropertyMapper& operator=(MaterialPropertyMapper&&) = delete;

		/**
		 * Returns if the property can be mapped using this object or not.
		 * @return property mapper instance, nullptr if it can't be mapped by this object
		 */
		static std::unique_ptr<MaterialPropertyMapper> mappable(const PropertyPath& path);

		/**
		 * Select and create a specific shader binding based on the selected property
		 * @param parent the widget to parent the selection dialog to
		 */
		void map(QWidget* parent);

		/**
		 * @return the shader, nullptr if shader could not be resolved
		 */
		const nap::BaseShader* getShader() const	{ return mShader; }

	private:
		// Uniform value bindings
		const nap::ShaderVariableDeclaration* selectVariableDeclaration(const nap::BufferObjectDeclarationList& list, QWidget* parent);
		const nap::ShaderVariableDeclaration* selectVariableDeclaration(const nap::ShaderVariableStructDeclaration& uniform, QWidget* parent);
		void addVariableBinding(const nap::ShaderVariableDeclaration& declaration, const PropertyPath& propPath);

		// Sampler bindings
		const nap::SamplerDeclaration* selectSamplerDeclaration(QWidget* parent);
		void addSamplerBinding(const nap::SamplerDeclaration& declaration, const PropertyPath& propPath);

		// Buffer bindings
		const nap::BufferObjectDeclaration* selectBufferDeclaration(const nap::BufferObjectDeclarationList& list, QWidget* parent);
		void addBufferBinding(const nap::BufferObjectDeclaration& declaration, const PropertyPath& propPath);

		// Fallback
		void addUserBinding(QWidget* parent);

		// Find shader on any type of material
		void resolveShader(const nap::BaseMaterial& material);

		PropertyPath mPath;						///< Current property path
		nap::BaseShader* mShader = nullptr;		///< Shader to resolve against
		bool mNested = false;					///< If the path is a nested uniform
		rttr::variant mRootUniforms;			///< All root uniforms
	};
}

