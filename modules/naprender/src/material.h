#pragma once

// External includes
#include <nap/serviceablecomponent.h>
#include <nap/attribute.h>
#include <nap/resourcelinkattribute.h>

// Local includes
#include "shaderresource.h"

namespace nap
{
	/**
	* Instance of a shader that lives in your object tree
	* Holds a pointer to the actual shader resource program
	* Can be used to draw an object to screen
	* Typically a material instance is a child of a mesh component
	*/
	class Material : public AttributeObject
	{
		RTTI_ENABLE_DERIVED_FROM(AttributeObject)
	public:
		// Default constructor
		Material();

		/**
		* Binds the GLSL shader resource program
		* Note that this call will fail when the shader is not initialized properly
		* @return if the shader was bound successfully
		*/
		bool bind();

		/**
		* Unbinds the GLSL shader resource program
		* @return if the shader was unbound successfully
		*/
		bool unbind();

		/**
		* @return if there's a shader resource linked to this instance
		*/
		bool hasShader() const						{ return shaderResource.isLinked(); }

		/**
		 * Utility for getting the shader resource
		 * @return the link as a shader resource, nullptr if not linked
		 */
		ShaderResource* getResource() const			{ return mResource; }

		/**
		* Link to the shader this material uses
		* By default this link is empty, needs to be set
		* when using this material for drawing
		*/
		ResourceLinkAttribute shaderResource =		{ this, "shader", RTTI_OF(ShaderResource) };

		/**
		 * Holds all uniform shader variables
		 */
		ObjectLinkAttribute uniforms =				{ this, "uniforms", RTTI_OF(AttributeObject) };

	private:
		
		/**
		 * Update uniform values associated with this material
		 * Note that this function can only be called when the shader
		 * has been loaded correctly
		 */
		void resolveUniforms();

		/**
		 * Clear uniform shader bindings
		 */
		void clearUniforms();

		/**
		 * Occurs when the resource changes, ie: link is removed or new
		 * object is linked in. In that case we want to listen to when the 
		 * shader is loaded or reloaded
		 */
		void onResourceChanged(AttributeBase& value);

		/**
		 * Occurs when the shader this material points to
		 * is loaded or reloaded, based on the success we 
		 * update our uniforms
		 */
		void onShaderLoaded(bool success);

		/**
		 * Internal resource cache
		 */
		ShaderResource* mResource = nullptr;

		NSLOT(shaderResourceChanged, AttributeBase&, onResourceChanged)
		NSLOT(shaderLoaded, bool, onShaderLoaded)
	};
}

RTTI_DECLARE(nap::Material)
