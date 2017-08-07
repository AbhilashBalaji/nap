#pragma once

// External Includes
#include <stdint.h>
#include <GL/glew.h>
#include <unordered_map>

namespace opengl
{
	/**
	* Holds all available GL drawing modes
	*/
	enum class EDrawMode : uint8_t
	{
		POINTS = 1,
		LINES = 2,
		LINE_STRIP = 3,
		LINE_LOOP = 4,
		TRIANGLES = 5,
		TRIANGLE_STRIP = 6,
		TRIANGLE_FAN = 7,
		UNKNOWN = 0,
	};


	/**
	 * Returns the DrawMode's associated OpenGL mode
	 * @param mode the internal DrawMode
	 * @return the OpenGL draw mode enum, GL_INVALID_ENUM if mode is unknown
	 */
	GLenum getGLMode(EDrawMode mode);


	/**
	 * Returns the DrawMode associated with the OpenGL defined drawmode
	 * @param mode the OpenGL draw mode
	 * @return the correct DrawMode, UNKNOWN if that draw mode is unknown or unsupported
	 */
	EDrawMode getDrawMode(GLenum mode);

} // opengl

namespace std
{
	template <>
	struct hash<opengl::EDrawMode>
	{
		size_t operator()(const opengl::EDrawMode& v) const
		{
			return hash<uint8_t>()(static_cast<uint8_t>(v));
		}
	};
}
