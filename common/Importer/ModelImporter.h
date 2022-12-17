#pragma once
#include "../GLSampleSession.h"
#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/camera.h>
#include <assimp/light.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>
#include <assimp/vector3.h>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

typedef struct model_object {
	GeometryObject geometryObject;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	struct model_object *parent;
	std::string name;
} NodeObject;

typedef struct model_system_object {
	size_t nrVertices;
	size_t nrIndices;
	size_t vertexStride;
	size_t indicesStride;
	void *vertexData;
	void *indicesData;

} ModelSystemObject;

using namespace Assimp;
class ModelImporter {
  public:
	IFileSystem *fileSystem;
	ModelImporter(IFileSystem *fileSystem) : fileSystem(fileSystem) {}
	~ModelImporter() { clear(); }

	void loadContent(const std::string &path, unsigned long int supportFlag);

	void clear();

  protected:
	void initScene(const aiScene *scene);
	// {
	//	unsigned int z;
	//
	//	unsigned int vertexSize = 0;
	//	unsigned int indicesSize = 0;
	//
	//	unsigned int VertexIndex = 0, IndicesIndex = 0, bonecount = 0, initilzebone = 0;
	//
	//	indicesSize = 4; /*TODO remove later, if not it cased the geometry notbe rendered proparly*/
	//	float *vertices = (float *)malloc(aimesh->mNumVertices * vertexSize);
	//	unsigned char *Indice = (unsigned char *)malloc(indicesSize * aimesh->mNumFaces * 3);
	//
	//	float *temp = vertices;
	//	unsigned char *Itemp = Indice;
	//
	//	const aiVector3D Zero = aiVector3D(0, 0, 0);
	//
	//	for (unsigned int x = 0; x < aimesh->mNumVertices; x++) {
	//		const aiVector3D *Pos = &(aimesh->mVertices[x]);
	//		aiVector3D *pNormal = &(aimesh->mNormals[x]);
	//		const aiVector3D *pTexCoord = aimesh->HasTextureCoords(0) ? &(aimesh->mTextureCoords[0][x]) : &Zero;
	//		// VDVector3 mtangent;
	//
	//		*vertices++ = Pos->x;
	//		*vertices++ = Pos->y;
	//		*vertices++ = Pos->z;
	//
	//		*vertices++ = pTexCoord->x;
	//		*vertices++ = pTexCoord->y;
	//
	//		pNormal->Normalize();
	//		*vertices++ = pNormal->x;
	//		*vertices++ = pNormal->y;
	//		*vertices++ = pNormal->z;
	//
	//		// mtangent = tangent(VDVector3(pNormal->x, pNormal->y, pNormal->z));
	//		// mtangent.makeUnitVector();
	//
	//		*vertices++ = mtangent.x();
	//		*vertices++ = mtangent.y();
	//		*vertices++ = mtangent.z();
	//
	//	} /**/
	//
	//	vertices = temp;
	//
	//	/*	some issues with this I thing? */
	//	for (unsigned int x = 0; x < aimesh->mNumFaces; x++) {
	//		const aiFace &face = aimesh->mFaces[x];
	//		assert(face.mNumIndices == 3); // Check if Indices Count is 3 other case error
	//		memcpy(Indice, &face.mIndices[0], indicesSize);
	//		Indice += indicesSize;
	//		memcpy(Indice, &face.mIndices[1], indicesSize);
	//		Indice += indicesSize;
	//		memcpy(Indice, &face.mIndices[2], indicesSize);
	//		Indice += indicesSize;
	//	}
	//
	//	Indice = Itemp;
	//
	//	/*	*/
	//
	//	/**/
	//	free(vertices);
	//	free(Indice);
	//}

	/**
	 *
	 */
	void initNoodeRoot(const aiNode *nodes, NodeObject *parent = nullptr);

	// VDMaterial *initMaterial(aiMaterial *material);

	ModelSystemObject *initMesh(const aiMesh *mesh, unsigned int index);

	// VDTexture2D *initTexture(aiTexture *texture, unsigned int index);
	//
	// VDCamera *initCamera(aiCamera *camera, unsigned int index);
	//
	// VDAnimationClip *initAnimation(const aiAnimation *animation, unsigned int index);
	//
	// VDLight *initLight(const aiLight *light, unsigned int index);

	const std::vector<NodeObject *> getNodes() const { return this->nodes; }
	const NodeObject *getNodeRoot() const { return this->rootNode; }

  private:
	aiScene *sceneRef;
	std::vector<NodeObject *> nodes;
	NodeObject *rootNode;
};
