#pragma once
#include "ModelImporter.h"

namespace glsample {

	class FVDECLSPEC ImportHelper {
	  public:
		static void loadModelBuffer(ModelImporter &modelLoader, std::vector<GeometryObject> &modelSet) {

			modelSet.resize(modelLoader.getModels().size());
			unsigned int tmp_ibo;
			unsigned int tmp_vbo;
			unsigned int tmp_vao;
			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &tmp_vao);
			glBindVertexArray(tmp_vao);

			size_t vertexDataSize = 0;
			size_t indicesDataSize = 0;
			for (size_t i = 0; i < modelLoader.getModels().size(); i++) {
				vertexDataSize += modelLoader.getModels()[i].vertexStride * modelLoader.getModels()[i].nrVertices;
				indicesDataSize += modelLoader.getModels()[i].indicesStride * modelLoader.getModels()[i].nrIndices;
			}

			glGenBuffers(1, &tmp_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_vbo);
			glBufferData(GL_ARRAY_BUFFER, vertexDataSize, nullptr, GL_STATIC_DRAW);
			size_t offset = 0;
			for (size_t i = 0; i < modelLoader.getModels().size(); i++) {
				size_t vertexSize = modelLoader.getModels()[i].vertexStride * modelLoader.getModels()[i].nrVertices;
				glBufferSubData(GL_ARRAY_BUFFER, offset, vertexSize, modelLoader.getModels()[i].vertexData);
				offset += vertexSize;
			}

			/*	*/
			glGenBuffers(1, &tmp_ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp_ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesDataSize, nullptr, GL_STATIC_DRAW);
			offset = 0;
			for (size_t i = 0; i < modelLoader.getModels().size(); i++) {
				size_t indicesSize = modelLoader.getModels()[i].indicesStride * modelLoader.getModels()[i].nrIndices;

				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, indicesSize, modelLoader.getModels()[i].indicesData);
				offset += indicesSize;
			}

			/*	Vertex.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	UV.	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			/*	Normal.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(20));

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(32));

			glBindVertexArray(0);

			size_t indices_offset = 0;
			size_t vertices_offset = 0;
			
			for (size_t i = 0; i < modelLoader.getModels().size(); i++) {
				const size_t vertexSize = modelLoader.getModels()[i].nrVertices;
				const size_t indicesSize = modelLoader.getModels()[i].nrIndices;

				/*	*/
				GeometryObject &ref = modelSet[i];
				ref.indices_offset = indices_offset;
				ref.vertex_offset = vertices_offset;
				ref.nrIndicesElements = modelLoader.getModels()[i].nrIndices;
				ref.nrVertices = modelLoader.getModels()[i].nrVertices;

				/*	*/
				ref.vao = tmp_vao;
				ref.vbo = tmp_vbo;
				ref.ibo = tmp_ibo;

				vertices_offset += vertexSize;
				indices_offset += indicesSize;
			}
		}
	};
} // namespace glsample