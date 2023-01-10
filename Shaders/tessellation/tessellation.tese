#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(triangles, equal_spacing, ccw) in;

layout(location = 0) in vec3 WorldPos_ES_in[];
layout(location = 1) in vec2 TexCoord_ES_in[];
layout(location = 2) in vec3 Normal_ES_in[];
layout(location = 3) in vec3 Tangent_ES_in[];

layout(location = 0) out vec3 WorldPos_FS_in;
layout(location = 1) out vec2 TexCoord_FS_in;
layout(location = 2) out vec3 Normal_FS_in;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	vec4 gEyeWorldPos;
	float gDispFactor;
	float tessLevel;
}
ubo;

layout(binding = 2) uniform sampler2D gDisplacementMap;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main() {
	/*	Interpolate the attributes of the output vertex using the barycentric coordinates	*/
	TexCoord_FS_in = interpolate2D(TexCoord_ES_in[0], TexCoord_ES_in[1], TexCoord_ES_in[2]);

	Normal_FS_in = interpolate3D(Normal_ES_in[0], Normal_ES_in[1], Normal_ES_in[2]);
	Normal_FS_in = normalize(Normal_FS_in);

	vec3 Tangent_FS_in = interpolate3D(Tangent_ES_in[0], Tangent_ES_in[1], Tangent_ES_in[2]);
	Tangent_FS_in = normalize(Tangent_FS_in);

	WorldPos_FS_in = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	/*	Displace the vertex along the normal	*/
	float heightMapDisp = texture(gDisplacementMap, TexCoord_FS_in.xy).r;
	WorldPos_FS_in += Normal_FS_in * heightMapDisp * ubo.gDispFactor;

	/*	Recompute the actual normal after displacement.	*/
	const float width = textureSize(gDisplacementMap, 0).x;
	const float height = textureSize(gDisplacementMap, 0).y;
	const vec2 uv = TexCoord_FS_in.xy;

	const vec2 du = vec2(1.0 / width, 0);
	const vec2 dv = vec2(0, 1.0 / height);

	const float dhdu =
		ubo.gDispFactor / (2.0 / width) * (texture(gDisplacementMap, uv + du).r - texture(gDisplacementMap, uv - du).r);
	const float dhdv = ubo.gDispFactor / (2.0 / height) *
					   (texture(gDisplacementMap, uv + dv).r - texture(gDisplacementMap, uv - dv).r);

	// Tangent_FS_in = normalize(Tangent_FS_in - dot(Tangent_FS_in, Normal_FS_in) * Normal_FS_in);
	vec3 bittagnet = cross(Tangent_FS_in, Normal_FS_in);

	Normal_FS_in = normalize(Normal_FS_in + Tangent_FS_in * dhdu + bittagnet * dhdv);

	/*	*/
	gl_Position = ubo.modelViewProjection * vec4(WorldPos_FS_in, 1.0);
}