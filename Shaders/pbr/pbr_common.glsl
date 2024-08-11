struct point_light {
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

struct tessellation_settings {
	float tessLevel;
	float gDispFactor;
};

struct light_settings {
	vec4 ambientColor;
	vec4 specularColor;
	vec4 direction;
	vec4 lightColor;
	point_light point_light[4];
	float gamma;
	float exposure;
};

struct camera_settings {
	vec4 gEyeWorldPos;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 viewProjection;
	mat4 modelViewProjection;

	light_settings lightsettings;
	tessellation_settings tessellation;
	/*	Camera settings.	*/
	camera_settings camera;
}
ubo;

struct OutputPatch {
	vec3 WorldPos_B030;
	vec3 WorldPos_B021;
	vec3 WorldPos_B012;
	vec3 WorldPos_B003;
	vec3 WorldPos_B102;
	vec3 WorldPos_B201;
	vec3 WorldPos_B300;
	vec3 WorldPos_B210;
	vec3 WorldPos_B120;
	vec3 WorldPos_B111;
	vec3 Normal[3];
	vec2 TexCoord[3];
};

const float PI = 3.14159265359;