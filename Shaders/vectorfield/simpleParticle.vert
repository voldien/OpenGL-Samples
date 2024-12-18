#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 velocity;
/*	*/
layout(location = 0) smooth out vec4 vColor;

#include "base.glsl"


void main() {
	gl_Position = ubo.view * vec4(vertex.xy, 0.0, 1.0);

	const vec3 velocity = velocity.xyz;
	const float reduce = (1.0 / -10.0);
	const float green = (1.0 / 20.0);
	const float blue = (1.0 / 5.0);

	/*	Compute particle color.	*/
	// const float reduce = (1.0 / 20.0);
	// const float green = (1.0 / 20.0);
	// const float blue = (1.0 / 50.0);

	/*	*/
	//vColor =  vec4(velocity.x * 10 * reduce * length(velocity), velocity.x * green, length(velocity) * blue, 1.0);
	vColor = vec4(hsv2rgb(velocity.xyz), 1);
}

