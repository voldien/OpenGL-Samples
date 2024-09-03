#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(triangles_adjacency) in; // six vertices in
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in vec3 vertex[];

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 specularColor;
	vec4 ambientColor;
	vec4 viewDir;
}
ubo;

layout(constant_id = 0) const float EPSILON = 0.0001;

// Emit a quad using a triangle strip
void EmitQuad(const vec3 StartVertex, const vec3 EndVertex) {

	const vec3 gLightPos = -ubo.direction.xyz * 100.0;


	const mat4 gWVP = ubo.modelViewProjection;
	// Vertex #1: the starting vertex (just a tiny bit below the original edge)
	vec3 LightDir = normalize(StartVertex - gLightPos);
	gl_Position = gWVP * vec4((StartVertex + LightDir * EPSILON), 1.0);
	EmitVertex();

	// Vertex #2: the starting vertex projected to infinity
	gl_Position = gWVP * vec4(LightDir, 0.0);
	EmitVertex();

	// Vertex #3: the ending vertex (just a tiny bit below the original edge)
	LightDir = normalize(EndVertex - gLightPos);
	gl_Position = gWVP * vec4((EndVertex + LightDir * EPSILON), 1.0);
	EmitVertex();

	// Vertex #4: the ending vertex projected to infinity
	gl_Position = gWVP * vec4(LightDir, 0.0);
	EmitVertex();

	EndPrimitive();
}

void main() {

	const vec3 e1 = vertex[2] - vertex[0];
	const vec3 e2 = vertex[4] - vertex[0];
	const vec3 e3 = vertex[1] - vertex[0];
	const vec3 e4 = vertex[3] - vertex[2];
	const vec3 e5 = vertex[4] - vertex[2];
	const vec3 e6 = vertex[5] - vertex[0];

	const vec3 gLightPos = ubo.direction.xyz * 100.0;
	vec3 Normal = normalize(cross(e1, e2));
	vec3 LightDir = normalize(gLightPos - vertex[0]);

	// Handle only light facing triangles
	if (dot(Normal, LightDir) > 0) {

		Normal = cross(e3, e1);

		if (dot(Normal, LightDir) <= 0) {
			vec3 StartVertex = vertex[0];
			vec3 EndVertex = vertex[2];
			EmitQuad(StartVertex, EndVertex);
		}

		Normal = cross(e4, e5);
		LightDir = gLightPos - vertex[2];

		if (dot(Normal, LightDir) <= 0) {
			vec3 StartVertex = vertex[2];
			vec3 EndVertex = vertex[4];
			EmitQuad(StartVertex, EndVertex);
		}

		Normal = cross(e2, e6);
		LightDir = gLightPos - vertex[4];

		if (dot(Normal, LightDir) <= 0) {
			vec3 StartVertex = vertex[4];
			vec3 EndVertex = vertex[0];
			EmitQuad(StartVertex, EndVertex);
		}

		// render the front cap
		LightDir = (normalize(vertex[0] - gLightPos));
		gl_Position = ubo.modelViewProjection * vec4((vertex[0] + LightDir * EPSILON), 1.0);
		EmitVertex();

		LightDir = (normalize(vertex[2] - gLightPos));
		gl_Position = ubo.modelViewProjection * vec4((vertex[2] + LightDir * EPSILON), 1.0);
		EmitVertex();

		LightDir = (normalize(vertex[4] - gLightPos));
		gl_Position = ubo.modelViewProjection * vec4((vertex[4] + LightDir * EPSILON), 1.0);
		EmitVertex();
		EndPrimitive();

		// render the back cap
		LightDir = vertex[0] - gLightPos;
		gl_Position = ubo.modelViewProjection * vec4(LightDir, 0.0);
		EmitVertex();

		LightDir = vertex[4] - gLightPos;
		gl_Position = ubo.modelViewProjection * vec4(LightDir, 0.0);
		EmitVertex();

		LightDir = vertex[2] - gLightPos;
		gl_Position = ubo.modelViewProjection * vec4(LightDir, 0.0);
		EmitVertex();
	}
}