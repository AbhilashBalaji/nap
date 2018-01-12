#version 150 core

in vec4  pass_Color;
in vec3  pass_Uvs;
in float pass_PID;

out vec4 out_Color;

uniform sampler2D	particleTextureOne;
uniform sampler2D	particleTextureTwo;

void main(void)
{
	// Get uv coordinates
	vec2 uvs = vec2(pass_Uvs.x, pass_Uvs.y);
	
	// Check which image to get
	int tex_id = int(pass_PID+0.1) % 2;

	// Get texture color
	vec4 tex_color = vec4(0,0,0,0);
	if(tex_id == 1)
	{
		tex_color = texture(particleTextureTwo, uvs);
	}
	else
	{
		tex_color = texture(particleTextureOne, uvs);
	}

	// Boost colors a bit
	float r = pow(tex_color.r,0.9);
	float g = pow(tex_color.g,0.9);
	float b = pow(tex_color.b,0.9);
	tex_color = vec4(r,g,b,tex_color.a);

	// Set output color
	out_Color = tex_color * pass_Color;
}