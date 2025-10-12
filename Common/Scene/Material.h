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
#include "GLDataStructure.h"
#include "Importer/ModelImporter.h"
#include "Prerequisites.h"
#include "SampleHelper.h"
#include "Scene/RenderQueue.h"
#include "ShaderPipeline.h"
#include <glm/fwd.hpp>
#include <vector>

namespace glsample {

	/**
	 *	Responsible for associated textures and
	 *	shader.
	 */
	class FVDECLSPEC Material : public fragcore::Object {
	  public:
		Material();
		// Material(ShaderPipeline *shader, RendererResourcePool *pool);

		/*	*/
		void bindBuffer(const unsigned int index, const UBORange &buffer);
		void bindBuffer(const unsigned int index, const UBOObject &buffer, const size_t offset,
						const size_t sizeInBytes);

		void pushData(const void *pdata, const size_t data, const size_t offset = 0);

		void setTexture(const int index, glsample::Texture *texture);
		glsample::Texture *getTexture(const int index);

		void setSampler(const int index, TextureSampler *sampler);
		TextureSampler *getSampler(const int index);

		RenderQueue getRenderQueue() const noexcept;

		void setPipeline(ShaderPipeline *pipeline);
		ShaderPipeline &getPipeline() noexcept { return *this->pipeline; }

		bool isTessellationEnabled() const noexcept { return this->tessellationSettings.gDispFactor && false; }

		TessellationSettings &getTessellationSettings() { return this->tessellationSettings; }
		const GraphicShaderSettings &getGraphicSettings() const noexcept { return this->graphicSettings; }
		GraphicShaderSettings &getGraphicSettings() noexcept { return this->graphicSettings; }

	  public: /*  Get and set methods.  */
		/*	Contains a set of programs, with different options compiled against.	*/
		ShaderPipeline *pipeline{};
		std::vector<ShaderPipeline *> subpasses;
		unsigned int program = 0; // TODO: relocate.

		std::vector<Texture *> textures;
		/*	*/
		std::map<std::string, unsigned int> name2Location;

		// Material properties.
		glm::vec4 ambient = glm::vec4(1, 1, 1, 1);
		glm::vec4 diffuse = glm::vec4(1);
		glm::vec4 emission = glm::vec4(0);
		glm::vec4 specular = glm::vec4(1);
		glm::vec4 transparent = glm::vec4(1);
		glm::vec4 reflectivity = glm::vec4(1);

		/*	*/
		float shinininess = 1;
		float bumpiness = 1;
		float opacity = 1;
		float metalic = 0;

		/*	*/

		TessellationSettings tessellationSettings;
		GraphicShaderSettings graphicSettings;

		unsigned int shade_model = 0; /*	aiShadingMode	*/

		MaterialTextureSampling texture_sampling[32];

		/*	Texture index.	*/
		union {
			struct {
				int diffuseIndex = -1;			/*	*/
				int normalIndex = -1;			/*	*/
				int maskTextureIndex = -1;		/*	*/
				int specularIndex = -1;			/*	*/
				int emissionIndex = -1;			/*	*/
				int reflectionIndex = -1;		/*	*/
				int ambientOcclusionIndex = -1; /*	*/
				int displacementIndex = -1;		/*	*/
				int metalIndex = -1;			/*	*/
				int heightbumpIndex = -1;		/*	*/
				int pad1 = -1;
				int pad2 = -1;
				int pad3 = -1;
				int pad4 = -1;
				int pad5 = -1;
				int pad6 = -1;
				int pad7 = -1;
				int pad8 = -1;
				int pad9 = -1;
				int pad10 = -1;
				int pad11 = -1;
				int pad12 = -1;
				int pad13 = -1;
				int pad14 = -1;
				int pad15 = -1;
				int pad16 = -1;
				int pad17 = -1;
				int pad18 = -1;
				int pad19 = -1;
				int pad20 = -1;
				int pad21 = -1;
				int pad22 = -1;
			};
			std::array<int, 32> texture_index; /*	TextureType.	*/
		};

	  private: /*	Attributes.	*/
			   // std::vector<TextureObject*> textures; /*  */
			   // ShaderObject* shader;                 /*	*/

	  public:
		static RenderQueue getDefaultQueueDomain(const Material &material) noexcept;
	};

	// class FVDECLSPEC MaterialInstance : fragcore::Object {
	//   public:
	// };

} // namespace glsample
