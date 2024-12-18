#include "Common.h"
#include "GLHelper.h"
#include <ProceduralGeometry.h>

using namespace glsample;
using namespace fragcore;

void Common::loadPlan(MeshObject &planMesh, const float scale, const int segmentX, const int segmentY) {

	std::vector<ProceduralGeometry::Vertex> vertices;
	std::vector<unsigned int> indices;
	ProceduralGeometry::generatePlan(scale, vertices, indices, segmentX, segmentY);

	/*	Create array buffer, for rendering static geometry.	*/
	glGenVertexArrays(1, &planMesh.vao);
	glBindVertexArray(planMesh.vao);

	/*	Create array buffer, for rendering static geometry.	*/
	glGenBuffers(1, &planMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, planMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
				 GL_STATIC_DRAW);

	/*	*/
	glGenBuffers(1, &planMesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planMesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

	/*	Vertex.	*/
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

	/*	UV.	*/
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(12));

	/*	Normal.	*/
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(20));

	/*	Tangent.	*/
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(32));

	glBindVertexArray(0);

	planMesh.nrIndicesElements = indices.size();
	planMesh.indices_offset = 0;
	planMesh.vertex_offset = 0;
	planMesh.nrVertices = vertices.size();
}

void Common::loadSphere(MeshObject &sphereMesh, const float radius, const int slices, const int segements) {
	std::vector<ProceduralGeometry::Vertex> vertices;
	std::vector<unsigned int> indices;
	ProceduralGeometry::generateSphere(radius, vertices, indices, slices, segements);

	/*	Create array buffer, for rendering static geometry.	*/
	glGenVertexArrays(1, &sphereMesh.vao);
	glBindVertexArray(sphereMesh.vao);

	/*	Create array buffer, for rendering static geometry.	*/
	glGenBuffers(1, &sphereMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, sphereMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
				 GL_STATIC_DRAW);

	/*	*/
	glGenBuffers(1, &sphereMesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereMesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

	/*	Vertex.	*/
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

	/*	UV.	*/
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(12));

	/*	Normal.	*/
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(20));

	/*	Tangent.	*/
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(32));

	glBindVertexArray(0);

	sphereMesh.nrIndicesElements = indices.size();
	sphereMesh.indices_offset = 0;
	sphereMesh.vertex_offset = 0;
	sphereMesh.nrVertices = vertices.size();
}

void Common::loadCube(MeshObject &cubeMesh, const float scale, const int segmentX, const int segmentY) {

	std::vector<ProceduralGeometry::Vertex> vertices;
	std::vector<unsigned int> indices;
	ProceduralGeometry::generateCube(scale, vertices, indices, segmentX);

	/*	Create array buffer, for rendering static geometry.	*/
	glGenVertexArrays(1, &cubeMesh.vao);
	glBindVertexArray(cubeMesh.vao);

	/*	Create array buffer, for rendering static geometry.	*/
	glGenBuffers(1, &cubeMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cubeMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
				 GL_STATIC_DRAW);

	/*	*/
	glGenBuffers(1, &cubeMesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeMesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

	/*	Vertex.	*/
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

	/*	UV.	*/
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(12));

	/*	Normal.	*/
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(20));

	/*	Tangent.	*/
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), reinterpret_cast<void *>(32));

	glBindVertexArray(0);

	cubeMesh.nrIndicesElements = indices.size();
	cubeMesh.indices_offset = 0;
	cubeMesh.vertex_offset = 0;
	cubeMesh.nrVertices = vertices.size();
}

void Common::mergeMeshBuffers(const std::vector<MeshObject> &sphereMesh, std::vector<MeshObject> &mergeMeshes) {

}

int Common::createColorTexture(unsigned int width, unsigned int height, const fragcore::Color &color) {
	GLuint texRef;

	FVALIDATE_GL_CALL(glGenTextures(1, (GLuint *)&texRef));
	FVALIDATE_GL_CALL(glBindTexture(GL_TEXTURE_2D, texRef));
	FVALIDATE_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, color.data()));

	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	/*	Border clamped to max value, it makes the outside area.	*/
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	/*	No Mipmap.	*/
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

	FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
	glBindTexture(GL_TEXTURE_2D, 0);

	return texRef;
}

void glsample::refreshWholeRoundRobinBuffer(unsigned int bufferType, unsigned int buffer, const unsigned int robin,
											const void *data, const size_t alignSize, const size_t bufferSize,
											const size_t offset) {

	/*	Bind buffer and update region with new data.	*/
	glBindBuffer(bufferType, buffer);

	uint8_t *mappedData = (uint8_t *)glMapBufferRange(bufferType, offset, alignSize * robin,
													  GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	for (size_t i = 0; i < robin; i++) {
		memcpy(&mappedData[i * alignSize], data, bufferSize);
	}

	glUnmapBuffer(bufferType);
}