#version 330 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

// Input Vertex Attributes
in vec3	in_Position;
in vec3 in_UV0;
in vec3 in_Normals;


// Output to fragment shader
out vec3 passUVs;					//< vetex uv's
out mat4 passModelMatrix;			//< Matrix to transform vertex from object to world space
out vec3 cameraLocation;			//< camera location
out vec3 passVert;					//< Vertex position in object space 
out vec3 passNormals;				//< Vertex normal

void main(void)
{
	// Extract camera location
	cameraLocation = vec3(inverse(viewMatrix)[3]);

	// Forward uvs to fragment shader
	passUVs = in_UV0;

	// Pass along model matrix for light calculations
	passModelMatrix = modelMatrix;

	// Pass along normals for light calculations
	passNormals = in_Normals * -1.0;

	// Pass along vertex position in object space
	passVert = in_Position;

	// Calculate frag position
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(in_Position, 1.0);
}