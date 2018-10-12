#version 150 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform float blendValue;
uniform float normalBlendValue;

in vec3	in_Position;
in float in_Tip;
in vec3 in_OriginalPosition;
in vec3 in_DisplacedPosition;

out float pass_Tip;

void main(void)
{
	// Blend the non-displacement with the displacement normal based on the blend value
	vec3 blend_dis = mix(in_DisplacedPosition, in_Position, normalBlendValue);

	// Blend original position with target position
	vec3 blend_pos = mix(in_OriginalPosition, blend_dis, blendValue);

	// Calculate position
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(blend_pos, 1.0);

	// Pass color
	pass_Tip = in_Tip;
}