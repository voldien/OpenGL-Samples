#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

/*	*/
layout(binding = 1) uniform sampler2D tex1;

// uniform mat4 view;
// uniform float zoom = 1.0;
// uniform float thick = 10.0205;
// uniform vec2 screen = vec2(1.0f);

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;
};

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;
	float time;
	float zoom;
	vec4 ambientColor;
	vec4 color;

	float thick; // = 10.0205;
	vec2 screen; //= vec2(1.0f);

	particle_setting setting;
}
ubo;

layout(location = 0) smooth in vec3 vVertex;

/*	Compute horizontal and vertical grid lines.	*/
float resultHorVer(const in vec3 pos, float scale) {

	/*	*/
	const float z = scale;
	const vec2 invScreen = 1.0f / ubo.screen;

	vec2 ex_UV = (invScreen * vec2(gl_FragCoord.xy)) * scale + pos.xy;

	return texture(tex1, ex_UV).a;
}

/*  Compute the grid color. */
vec4 computeColor() {

	/*	constants.	*/
	const float zoomLevel = 68.0;
	const float grid1Intensity = 0.75;
	const float grid2Intensity = 0.45;

	/*	*/
	const float zscale = (zoomLevel - ubo.zoom);
	const vec3 pos1 = (ubo.view * vec4(vVertex, 0.0)).xyz;
	const vec3 pos2 = (ubo.view * vec4(vVertex, 0.0)).xyz + (ubo.view * vec4(0.5, 0.5, 0, 0)).xyz;

	/*  Compute grid color.   */
	float color = resultHorVer(pos1, zscale) * grid1Intensity;
	color += resultHorVer(pos2, zscale) * grid2Intensity;
	/*  Final color.	*/
	return vec4(color, color, color, 1.0);
}

void main() { fragColor = computeColor(); }
