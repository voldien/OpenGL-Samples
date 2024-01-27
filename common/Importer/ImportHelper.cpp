#include "ImportHelper.h"
#include "ImageImport.h"
#include <GL/glew.h>
#include <ProceduralGeometry.h>
#include <exception>
#include <iostream>
#include <ostream>
using namespace glsample;
using namespace fragcore;

void ImportHelper::loadModelBuffer(ModelImporter &modelLoader, std::vector<GeometryObject> &modelSet) {

	modelSet.resize(modelLoader.getModels().size());
	unsigned int tmp_ibo;
	unsigned int tmp_vbo;
	unsigned int tmp_vao;

	// TODO compute number of buffer required to fill.
	// TODO: use mapping instead.

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

	const size_t vertexStride = modelLoader.getModels()[0].vertexStride;
	const size_t IndicesStride = modelLoader.getModels()[0].indicesStride;

	// TODO add proper stride.
	/*	Vertex.	*/
	glEnableVertexAttribArrayARB(0);
	glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, vertexStride, reinterpret_cast<void *>(0));

	/*	UV.	*/
	glEnableVertexAttribArrayARB(1);
	glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, vertexStride, reinterpret_cast<void *>(12));

	/*	Normal.	*/
	glEnableVertexAttribArrayARB(2);
	glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, vertexStride, reinterpret_cast<void *>(20));

	/*	Tangent.	*/
	glEnableVertexAttribArrayARB(3);
	glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, vertexStride, reinterpret_cast<void *>(32));

	/*	BoneID.	*/
	if (modelLoader.getModels()[0].boneOffset > 0) {
		glEnableVertexAttribArrayARB(4);
		glVertexAttribPointerARB(4, 4, GL_INT, GL_FALSE, vertexStride, reinterpret_cast<void *>(32));

		/*	BoneID.	*/
		glEnableVertexAttribArrayARB(5);
		glVertexAttribPointerARB(5, 4, GL_FLOAT, GL_FALSE, vertexStride, reinterpret_cast<void *>(32));
	}

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

void ImportHelper::loadTextures(ModelImporter &modelLoader, std::vector<TextureAssetObject> &textures) {

	std::vector<TextureAssetObject> &Reftextures = modelLoader.getTextures();

	textures.resize(Reftextures.size());

	glsample::TextureImporter textureImporter(modelLoader.getFileSystem());
	ColorSpace colorSpace;

	for (size_t i = 0; i < Reftextures.size(); i++) {
		TextureAssetObject &tex = Reftextures[i];

		if (tex.data == nullptr) {
			try {
				std::cout << "Loading " << tex.filepath << std::endl;
				tex.texture = textureImporter.loadImage2D(tex.filepath);
				glObjectLabel(GL_TEXTURE, tex.texture, tex.filepath.size(), tex.filepath.data());

			} catch (const std::exception &ex) {
				std::cerr << "Failed to load: " << tex.filepath << " " << ex.what() << std::endl;
			}

		} else {

			/*	Compressed data.	*/
			if (tex.height == 0 && tex.dataSize > 0 && tex.data && tex.width > 0) {

				fragcore::Ref<fragcore::IO> refIO = fragcore::Ref<fragcore::IO>(
					new fragcore::BufferIO((const void *)tex.data, (unsigned long)tex.dataSize));

				/*	*/
				fragcore::ImageLoader imageLoader;

				try {

					Image image = imageLoader.loadImage(refIO);
					tex.texture = textureImporter.loadImage2DRaw(image);
				} catch (std::exception &ex) {
					std::cerr << "Failed to load: " << ex.what() << std::endl;
				}

				refIO->close();
			} else {

				/*	None-compressed.	*/
				fragcore::Image image(tex.width, tex.height, TextureFormat::ARGB32);
				image.setPixelData(tex.data, image.getSize());

				tex.texture = textureImporter.loadImage2DRaw(image);
			}
		}
	}
	textures = Reftextures;
}