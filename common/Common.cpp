#include "Common.h"
#include "GLHelper.h"
#include "GLSampleSession.h"
#include "RenderDesc.h"
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

void Common::mergeMeshBuffers(const std::vector<MeshObject> &sphereMesh, std::vector<MeshObject> &mergeMeshes) {}

int Common::createColorTexture(unsigned int width, unsigned int height, const fragcore::Color &color) {
	GLuint texRef;

	FVALIDATE_GL_CALL(glGenTextures(1, (GLuint *)&texRef));
	FVALIDATE_GL_CALL(glBindTexture(GL_TEXTURE_2D, texRef));
	FVALIDATE_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, color.data()));

	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

	/*	*/
	FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0));

	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	/*	No Mipmap.	*/
	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

	FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

	FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
	glBindTexture(GL_TEXTURE_2D, 0);

	return texRef;
}

void Common::createFrameBuffer(FrameBuffer *framebuffer, unsigned int nrAttachments) {

	glGenFramebuffers(1, &framebuffer->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);

	glGenTextures(nrAttachments, framebuffer->attachments.data());
	glGenTextures(1, &framebuffer->depthbuffer);
	framebuffer->nrAttachments = nrAttachments;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Common::updateFrameBuffer(FrameBuffer *framebuffer, const std::initializer_list<fragcore::TextureDesc> &desc,
							   const fragcore::TextureDesc &depthstencil) {

	unsigned int attachment_index = 0;
	std::array<GLenum, 16> attachments_mapping;
	for (auto it = desc.begin(); it != desc.end(); it++) {
		const fragcore::TextureDesc &target_desc = *(it);

		const unsigned int width = target_desc.width;
		const unsigned int height = target_desc.height;
		const unsigned int depth = target_desc.depth;
		const unsigned int multisamples = target_desc.nrSamples;
		const GLenum internal_format = GL_RGBA16F;

		GLenum texture_type = GL_TEXTURE_2D;
		if (depth > 1) {
			texture_type = GL_TEXTURE_2D_ARRAY;
		}

		if (multisamples > 0) {
			texture_type = GL_TEXTURE_2D_MULTISAMPLE;
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->framebuffer);

		// TODO: verify if texture
		glBindTexture(texture_type, framebuffer->attachments[attachment_index]);

		if (multisamples > 0) {
			glTexImage2DMultisample(texture_type, multisamples, internal_format, width, height, GL_TRUE);
		} else {
			if (depth > 1) {
				glTexImage3D(texture_type, 0, internal_format, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE,
							 nullptr);
			} else {
				glTexImage2D(texture_type, 0, internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			}
		}

		if (multisamples == 0) {

			glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, GL_MIRROR_CLAMP_TO_EDGE);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_R, GL_MIRROR_CLAMP_TO_EDGE);

			FVALIDATE_GL_CALL(glTexParameteri(texture_type, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(texture_type, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(texture_type, GL_TEXTURE_BASE_LEVEL, 0));
		}

		glBindTexture(texture_type, 0);

		const GLenum attachment = GL_COLOR_ATTACHMENT0 + attachment_index;
		if (depth == 1) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture_type, framebuffer->attachments[attachment_index],
								   0);
		} else {
			// TODO: add a
			glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, framebuffer->attachments[attachment_index], 0, 0);
		}

		attachments_mapping[attachment_index] = attachment;

		attachment_index++;
	}

	glDrawBuffers(framebuffer->nrAttachments, attachments_mapping.data());

	/*	Depth/stencil buffer.	*/
	{

		const unsigned int multisamples = depthstencil.nrSamples;
		const unsigned int width = depthstencil.width;
		const unsigned int height = depthstencil.height;
		const unsigned int depth = depthstencil.depth;

		GLenum texture_type = GL_TEXTURE_2D;
		if (multisamples > 0) {
			texture_type = GL_TEXTURE_2D_MULTISAMPLE;
		}

		/*	*/
		glBindTexture(texture_type, framebuffer->depthbuffer);
		if (multisamples > 0) {
			glTexImage2DMultisample(texture_type, multisamples, GL_DEPTH_COMPONENT32, width, height, GL_TRUE);
		} else {
			glTexImage2D(texture_type, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
						 nullptr);
		}
		if (multisamples == 0) {

			glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			glTexParameteri(texture_type, GL_TEXTURE_MAX_LOD, 0);
			glTexParameterf(texture_type, GL_TEXTURE_LOD_BIAS, 0.0f);
			glTexParameteri(texture_type, GL_TEXTURE_BASE_LEVEL, 0);
		}

		glBindTexture(texture_type, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_type, framebuffer->depthbuffer, 0);
	}

	/*  Validate if created properly.*/
	const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
		throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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