/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Core/Object.h"
#include "Prerequisites.h"
#include "ShaderPipeline.h"
#include <glm/fwd.hpp>

namespace glsample {

	/**
	 *	Responsible for associated textures and
	 *	shader.
	 */
	class FVDECLSPEC Material : fragcore::Object {
	  public:
		Material() = default;
		// Material(ShaderPipeline *shader, RendererResourcePool *pool);

		// virtual void PushUniforms(UniformHandler &uniformObject, const Transform *) = 0;
		// virtual void PushDescriptors(DescriptorsHandler &descriptorSet) = 0;

		// void setTexture(int index, TextureObject *tex);
		// TextureObject *getTexture(int index);

		// void setMainColor(const Color &color);
		// Color getMainColor() const;

		// void setTextureOffset(int index, PVVector2 offset);

		// /**
		//  *
		//  * @param index
		//  * @return
		//  */
		// PVVector2 getTextureOffset(int index) const;

		// /**
		//  *
		//  * @param index
		//  * @param scale
		//  */
		// void setTextureScale(int index, PVVector2 scale);

		// /**
		//  *
		//  * @param index
		//  * @return
		//  */
		// PVVector2 getTextureScale(int index) const;

		// /**
		//  *	Bind material to current material on
		//  *	the current thread.
		//  */
		// void bind();

		// /**
		//  *
		//  * @return
		//  */
		// RenderQueue getRenderQueue() const;

		ShaderPipeline &getPipeline() noexcept { return *this->pipeline; }

	  public: /*  Get and set methods.  */
			  // int getInt(const char *name);
			  // void setInt(const char *name, int value);

		// void setIntArray(const char *name, int nrElements, int *elements);
		// int *getIntArray(const char *name, int *nrElements);

		// float getFloat(const char *name);
		// void setFloat(const char *name, float value);

		// void setFloatArray(const char *name, int nrElements, float *elements);
		// float *getFloatArray(const char *name, int *nrElements);

		// void setVectorArray(const char *name, int nrElements, PVVector4 *elements);
		// PVVector4 *getVectorArray(const char *name, int *nrElements);

		ShaderPipeline *pipeline{};
		unsigned int program = 0; // TODO: relocate.

		// Material properties.
		//	glm::vec4 ambient = glm::vec4(1, 1, 1, 1);
		//	glm::vec4 diffuse = glm::vec4(1);
		//	glm::vec4 emission = glm::vec4(0);
		//	glm::vec4 specular = glm::vec4(1);
		//	glm::vec4 transparent = glm::vec4(1);
		//	glm::vec4 reflectivity = glm::vec4(1);
		//
		/*	*/
		float shinininess = 1;
		float bumpiness = 1;
		float opacity = 1;
		float metalic = 0;
		int blend_func_mode = 0; /*	aiBlendMode*/
		int wireframe_mode = 0;
		bool culling_both_side_mode = false;
		float clipping = 1;
		/*	*/

		unsigned int shade_model = 0; /*	aiShadingMode	*/

		// MaterialTextureSampling texture_sampling[32];

		/*	Texture index.	*/
		// union {
		// 	struct {
		// 		int diffuseIndex = -1;			/*	*/
		// 		int normalIndex = -1;			/*	*/
		// 		int maskTextureIndex = -1;		/*	*/
		// 		int specularIndex = -1;			/*	*/
		// 		int emissionIndex = -1;			/*	*/
		// 		int reflectionIndex = -1;		/*	*/
		// 		int ambientOcclusionIndex = -1; /*	*/
		// 		int displacementIndex = -1;		/*	*/
		// 		int metalIndex = -1;			/*	*/
		// 		int heightbumpIndex = -1;		/*	*/
		// 		int pad1 = -1;
		// 		int pad2 = -1;
		// 		int pad3 = -1;
		// 		int pad4 = -1;
		// 		int pad5 = -1;
		// 		int pad6 = -1;
		// 		int pad7 = -1;
		// 		int pad8 = -1;
		// 		int pad9 = -1;
		// 		int pad10 = -1;
		// 		int pad11 = -1;
		// 		int pad12 = -1;
		// 		int pad13 = -1;
		// 		int pad14 = -1;
		// 		int pad15 = -1;
		// 		int pad16 = -1;
		// 		int pad17 = -1;
		// 		int pad18 = -1;
		// 		int pad19 = -1;
		// 		int pad20 = -1;
		// 		int pad21 = -1;
		// 		int pad22 = -1;
		// 	};
		// 	std::array<int, 32> texture_index{}; /*	TextureType.	*/
		// };

	  private: /*	Attributes.	*/
			   // std::vector<TextureObject*> textures; /*  */
			   // ShaderObject* shader;                 /*	*/
	};

} // namespace glsample
