#include "ImportHelper.h"
#include "GraphicFormat.h"
#include "IO/FileIO.h"
#include "ImageImport.h"
#include "ModelImporter.h"
#include "RenderDesc.h"
#include "Util/ProcessDataUtil.h"
#include <GL/glew.h>
#include <ImageUtil.h>
#include <ProceduralGeometry.h>
#include <cstdint>
#include <exception>
// #include <half.hpp>
#include <iostream>
#include <ostream>

using namespace glsample;
using namespace fragcore;

using ModelTemp = struct model_temp_t {
	const ModelSystemObject *model;
	size_t index;
};

void ImportHelper::loadModelBuffer(ModelImporter &modelLoader, std::vector<MeshObject> &modelSet) {

	modelSet.resize(modelLoader.getModels().size());
	unsigned int tmp_ibo = 0;

	std::map<int, std::vector<ModelTemp>> map;
	std::map<int, int> strideVBOMap;
	std::map<int, int> strideVBAMap;
	/*	*/
	size_t indices_offset = 0;
	size_t indicesDataSize = 0;

	/*	Sort based on vertex stride.	*/
	for (size_t i = 0; i < modelLoader.getModels().size(); i++) {
		const ModelSystemObject &refModel = modelLoader.getModels()[i];

		assert(refModel.vertexStride > 0);

		map[refModel.vertexStride].push_back({&refModel, i});
		indicesDataSize += refModel.indicesStride * refModel.nrIndices;
	}

	{
		/*	*/
		glGenBuffers(1, &tmp_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesDataSize, nullptr, GL_STATIC_DRAW);

		uint8_t *elementPointer =
			(uint8_t *)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, indicesDataSize, GL_MAP_WRITE_BIT);

		size_t offset = 0;

		for (auto it = map.begin(); it != map.end(); it++) {
			const std::vector<ModelTemp> &ref = (*it).second;
			const int vertexStride = (*it).first;

			for (size_t i = 0; i < ref.size(); i++) {
				const ModelSystemObject &refModel = *ref[i].model;
				const size_t indicesByteSize = refModel.indicesStride * refModel.nrIndices;

				std::memcpy(&elementPointer[offset], refModel.indicesData, indicesByteSize);
				offset += indicesByteSize;
			}
		}
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	/*	Create array buffer, for rendering static geometry.	*/
	size_t nrVertices = 0;
	size_t nrIndices = 0;
	size_t boneDataSize = 0;

	for (auto it = map.begin(); it != map.end(); it++) {

		const std::vector<ModelTemp> &ref = (*it).second;
		const int vertexStride = (*it).first;

		size_t vertexDataSize = 0;
		for (size_t ref_index = 0; ref_index < ref.size(); ref_index++) {
			const ModelSystemObject &refModel = *ref[ref_index].model;

			nrVertices += refModel.nrVertices;
			/*	*/
			vertexDataSize += refModel.vertexStride * refModel.nrVertices;
		}

		unsigned int tmp_vbo = 0;

		/*	Allocate memory.	*/
		glGenBuffers(1, &tmp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_vbo);
		glBufferData(GL_ARRAY_BUFFER, vertexDataSize, nullptr, GL_STATIC_DRAW);

		uint8_t *vertexPointer = (uint8_t *)glMapBufferRange(GL_ARRAY_BUFFER, 0, vertexDataSize, GL_MAP_WRITE_BIT);

		/*	Map Buffer. */

		size_t offset = 0;

		for (size_t i = 0; i < ref.size(); i++) {
			const ModelSystemObject &refModel = *ref[i].model;

			const size_t vertexByteSize = refModel.vertexStride * refModel.nrVertices;

			std::memcpy(&vertexPointer[offset], refModel.vertexData, vertexByteSize);
			offset += vertexByteSize;
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		strideVBOMap[vertexStride] = tmp_vbo;

		const ModelSystemObject &refModel_base = *ref[0].model;

		unsigned int tmp_vao = 0;
		glGenVertexArrays(1, &tmp_vao);
		glBindVertexArray(tmp_vao);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp_ibo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_vbo);

		// TODO: based on conditions
		/*	Vertex.	*/
		glEnableVertexAttribArrayARB(AttributeMapping::Vertex);
		glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, vertexStride,
								 reinterpret_cast<void *>(refModel_base.vertexOffset));

		/*	UV.	*/
		glEnableVertexAttribArrayARB(1);
		glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, vertexStride,
								 reinterpret_cast<void *>(refModel_base.uvOffset));

		/*	Normal.	*/
		glEnableVertexAttribArrayARB(2);
		glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, vertexStride,
								 reinterpret_cast<void *>(refModel_base.normalOffset));

		/*	Tangent.	*/
		glEnableVertexAttribArrayARB(3);
		glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, vertexStride,
								 reinterpret_cast<void *>(refModel_base.tangentOffset));

		if (refModel_base.vertexColorOffset >= 0) {
		}

		/*	Bone.	*/
		if (refModel_base.boneIndexOffset > 0) {

			/*	BoneID.	*/
			glEnableVertexAttribArrayARB(4);
			glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, vertexStride,
								   reinterpret_cast<void *>(refModel_base.boneIndexOffset));

			/*	Weight.	*/
			glEnableVertexAttribArrayARB(5);
			glVertexAttribPointerARB(5, 4, GL_FLOAT, GL_FALSE, vertexStride,
									 reinterpret_cast<void *>(refModel_base.boneWeightOffset));

		} else {

			glDisableVertexAttribArrayARB(4);
			glDisableVertexAttribArrayARB(5);
		}

		glBindVertexArray(0);

		strideVBAMap[vertexStride] = tmp_vao;

		/*	*/
		size_t vertices_offset = 0;
		for (size_t i = 0; i < ref.size(); i++) {

			/*	*/
			const size_t pindex = ref[i].index;
			const ModelSystemObject &refModel = *ref[i].model;

			/*	*/
			const size_t vertexStride = refModel.vertexStride;
			const size_t IndicesStride = refModel.indicesStride;

			/*	*/
			const size_t vertexSize = refModel.nrVertices;
			const size_t indicesSize = refModel.nrIndices;

			/*	*/
			MeshObject &ref = modelSet[pindex];
			ref.indices_offset = indices_offset;
			ref.vertex_offset = vertices_offset;
			ref.nrIndicesElements = refModel.nrIndices;
			ref.nrVertices = refModel.nrVertices;
			ref.bound = refModel.bound;

			/*	*/
			vertices_offset += vertexSize;
			indices_offset += indicesSize;

			/*	*/
			ref.vao = tmp_vao;
			ref.ibo = tmp_ibo;
			ref.vbo = tmp_vbo;

			switch (refModel.primitiveType) {
			case 1:
				ref.primitiveType = fragcore::Primitive::Point;
				break;
			case 2:
				ref.primitiveType = fragcore::Primitive::Lines;
				break;
			default:
			case 4:
				ref.primitiveType = fragcore::Primitive::Triangles;
				break;
			}
		}
	}
}

void ImportHelper::loadTextures(ModelImporter &modelLoader, std::vector<TextureAssetObject> &textures) {

	std::vector<TextureAssetObject> &Reftextures = modelLoader.getTextures();

	std::vector<Image> images;
	images.resize(Reftextures.size());

	textures.resize(Reftextures.size());

	glsample::TextureImporter textureImporter(modelLoader.getFileSystem());
	MiscProcessingUtil process(modelLoader.getFileSystem());

	/*	*/
#pragma omp parallel for schedule(static, 4)
	for (size_t texture_index = 0; texture_index < Reftextures.size(); texture_index++) {

		std::vector<MaterialObject *> materials = modelLoader.getMaterials(texture_index);

		TextureAssetObject &tex = Reftextures[texture_index];
		TextureCompression compression = TextureCompression::Default;

		if (tex.data == nullptr) {

			try {
				/*	*/
				std::cout << "Loading " << tex.filepath << std::endl;
				fragcore::Ref<fragcore::IO> refIO =
					fragcore::Ref<fragcore::IO>(new fragcore::FileIO(tex.filepath, FileIO::READ));

				/*	*/
				fragcore::ImageLoader imageLoader;
				Image image = imageLoader.loadImage(refIO);

				/*	Convert BumpMap to NormalMap*/
				if (!materials.empty() && materials[0]->heightbumpIndex == texture_index) {

					// TODO: use gpu to convert image.
					image = std::move(ImageUtil::convert2NormalMap(image, 3.5f));
					materials[0]->heightbumpIndex = -1;
					materials[0]->normalIndex = texture_index;
				}

				// TODO: convert glossy/specular to roughness if used.

				/*	*/
				images[texture_index] = std::move(image);
				// tex.texture = textureImporter.loadImage2DRaw(image, colorSpace, compression);

			} catch (const std::exception &ex) {
				std::cerr << "Failed to load: " << tex.filepath << " " << ex.what() << std::endl;
			}
		} else {

			/*	Compressed data.	*/
			if (tex.height == 0 && tex.dataSize > 0 && tex.data && tex.width > 0) {

				/*	*/
				fragcore::Ref<fragcore::IO> refIO = fragcore::Ref<fragcore::IO>(
					new fragcore::BufferIO((const void *)tex.data, (unsigned long)tex.dataSize));

				/*	*/
				fragcore::ImageLoader imageLoader;

				try {

					Image image = imageLoader.loadImage(refIO);
					images[texture_index] = std::move(image);

				} catch (std::exception &ex) {
					std::cerr << "Failed to load: " << ex.what() << std::endl;
				}

				refIO->close();
			} else {

				/*	None-compressed.	*/
				fragcore::Image image(tex.width, tex.height, ImageFormat::ARGB32);
				image.setPixelData(tex.data, image.getSize());
				images[texture_index] = std::move(image);
				/*	*/
			}
		}

		/*	*/
	}

	std::cout << "Start Transfering ImageData to GPU" << std::endl;
	
#pragma omp master
	for (size_t texture_index = 0; texture_index < images.size(); texture_index++) {

		TextureAssetObject &tex = Reftextures[texture_index];
		ColorSpace colorSpace = ColorSpace::RawLinear;
		const TextureCompression compression = TextureCompression::Default;
		fragcore::GraphicFormat format = fragcore::GraphicFormat::NotSpecified;

		std::vector<MaterialObject *> materials = modelLoader.getMaterials(texture_index);

		/*	Determine color space, based on the texture usages.	*/
		if (!materials.empty()) {

			if (materials[0]->diffuseIndex == texture_index) {
				colorSpace = ColorSpace::SRGB;
			}
			if (materials[0]->normalIndex == texture_index) {
				format = fragcore::GraphicFormat::R8G8_UNorm;
			}
			if (materials[0]->emissionIndex == texture_index) {
				format = fragcore::GraphicFormat::E5B9G9R9_UFloatPack32;
			}
		}

		/*	Load texture only if image data is valid.	*/
		const Image &image = images[texture_index];
		if (image.getSize() > 0) {
			tex.texture = textureImporter.loadImage2DRaw(images[texture_index], colorSpace, compression, format);
		} else {
			tex.texture = 0;
		}

		/*	*/
		if (tex.texture > 0) {
			glObjectLabel(GL_TEXTURE, tex.texture, tex.filepath.size(), tex.filepath.data());
		}
	}

	glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

	textures = Reftextures;
}