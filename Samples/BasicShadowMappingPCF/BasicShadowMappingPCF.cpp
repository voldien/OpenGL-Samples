
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
namespace glsample {

	class BasicShadowMapping : public GLSampleWindow {
	  public:
		BasicShadowMapping() : GLSampleWindow() {
			this->setTitle("ShadowMap");
			this->tessellationSettingComponent = std::make_shared<TessellationSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->tessellationSettingComponent);
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			alignas(16) glm::mat4 lightModelProject;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			float bias;
			float shadowStrength;
		} uniformBuffer;

		unsigned int shadowFramebuffer;
		unsigned int shadowTexture;
		unsigned int shadowWidth = 1024;
		unsigned int shadowHeight = 1024;

		GeometryObject plan;
		GeometryObject cube;
		GeometryObject sphere;

		unsigned int graphic_program;
		unsigned int shadow_program;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;
		class TessellationSettingComponent : public nekomimi::UIComponent {

		  public:
			TessellationSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Tessellation Settings");
			}
			virtual void draw() override {
				ImGui::DragFloat("Shadow Strength", &this->uniform.shadowStrength, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 10.0f);
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<TessellationSettingComponent> tessellationSettingComponent;

		const std::string vertexShaderPath = "Shaders/texture/texture.vert";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag";
		const std::string vertexShadowShaderPath = "Shaders/shadowmap/shadowmap.vert";
		const std::string fragmentShadowShaderPath = "Shaders/shadowmap/shadowmap.frag";

		virtual void Release() override {
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->shadow_program);

			glDeleteBuffers(1, &this->uniform_buffer);

			// glDeleteVertexArrays(1, &this->vao);
			// glDeleteBuffers(1, &this->vbo);
			// glDeleteBuffers(1, &this->ibo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath, FileSystem::getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath, FileSystem::getFileSystem());

			std::vector<char> vertex_shadow_source =
				IOUtil::readFile(vertexShadowShaderPath, FileSystem::getFileSystem());
			std::vector<char> fragment_shadow_source =
				IOUtil::readFile(fragmentShadowShaderPath, FileSystem::getFileSystem());

			/*	Load shaders	*/
			this->graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);
			this->shadow_program = ShaderLoader::loadGraphicProgram(&vertex_shadow_source, &fragment_shadow_source);

			/*	*/
			glUseProgram(this->graphic_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align(uniformBufferSize, (size_t)minMapBufferSize);

			// Create uniform buffer.
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create shadow map.	*/
				glGenFramebuffers(1, &shadowFramebuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

				glGenTextures(1, &this->shadowTexture);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT,
							 GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
				int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {

					/*  Delete  */
					glDeleteFramebuffers(1, &shadowFramebuffer);
					throw RuntimeException("Failed to create framebuffer, {}", frstat);
				}
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices, planVertices, cubeVertices, sphereVertices;
			std::vector<unsigned int> indices, planIndices, cubeIndices, sphereIndices;
			ProceduralGeometry::generatePlan(1, planVertices, planIndices, 1, 1);
			ProceduralGeometry::generateSphere(1, cubeVertices, cubeIndices);
			ProceduralGeometry::generateCube(1, sphereVertices, sphereIndices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, this->plan.vbo);
			glBufferData(GL_ARRAY_BUFFER,
						 (planVertices.size() + cubeVertices.size() + sphereVertices.size()) *
							 sizeof(ProceduralGeometry::Vertex),
						 nullptr, GL_STATIC_DRAW);

			glBufferSubData(GL_ARRAY_BUFFER, 0, planVertices.size() * sizeof(ProceduralGeometry::Vertex),
							planVertices.data());
			glBufferSubData(GL_ARRAY_BUFFER, planVertices.size() * sizeof(ProceduralGeometry::Vertex),
							cubeVertices.size() * sizeof(ProceduralGeometry::Vertex), cubeVertices.data());
			glBufferSubData(GL_ARRAY_BUFFER,
							(planVertices.size() + cubeVertices.size()) * sizeof(ProceduralGeometry::Vertex),
							sphereVertices.size() * sizeof(ProceduralGeometry::Vertex), sphereVertices.data());

			glGenBuffers(1, &plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						 (planIndices.size() + cubeIndices.size() + sphereIndices.size()) * sizeof(indices[0]), nullptr,
						 GL_STATIC_DRAW);

			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, planIndices.size() * sizeof(indices[0]), planIndices.data());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, planIndices.size() * sizeof(indices[0]),
							cubeIndices.size() * sizeof(indices[0]), cubeIndices.data());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (planIndices.size() + cubeIndices.size()) * sizeof(indices[0]),
							sphereIndices.size() * sizeof(indices[0]), sphereIndices.data());

			/*		*/
			this->plan.nrIndicesElements = planIndices.size();
			this->plan.indices_offset = 0;
			this->cube.nrIndicesElements = cubeIndices.size();
			this->cube.indices_offset = planVertices.size();
			this->sphere.nrIndicesElements = sphereIndices.size();
			this->sphere.indices_offset = planVertices.size() + cubeVertices.size();

			/*	Vertex.	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	UV.	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(12));

			/*	Normal.	*/
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(20));

			/*	Tangent.	*/
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(32));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			{

				/*	Compute light matrices.	*/
				float near_plane = 1.0f, far_plane = 7.5f;
				glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
				glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
												  glm::vec3(0.0f, 1.0f, 0.0f));
				glm::mat4 lightSpaceMatrix = lightProjection * lightView;

				glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, shadowWidth, shadowHeight);
				glUseProgram(this->shadow_program);
				glCullFace(GL_FRONT);

				/*	Setup the shadow.	*/

				glBindVertexArray(this->plan.vao);

				/*  Draw light source.  */
				glDrawElementsBaseVertex(GL_TRIANGLES, plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 plan.indices_offset);
				glDrawElementsBaseVertex(GL_TRIANGLES, cube.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 cube.indices_offset);
				glDrawElementsBaseVertex(GL_TRIANGLES, sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 sphere.indices_offset);

				glBindVertexArray(0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			{
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_COLOR_BUFFER_BIT);
				glUseProgram(this->graphic_program);

				glCullFace(GL_BACK);

				/**/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);

				/**/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);

				/*	Draw triangle*/
				glBindVertexArray(this->plan.vao);
				glDrawElementsBaseVertex(GL_TRIANGLES, plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 plan.indices_offset);
				glDrawElementsBaseVertex(GL_TRIANGLES, cube.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 cube.indices_offset);
				glDrawElementsBaseVertex(GL_TRIANGLES, sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 sphere.indices_offset);
				glBindVertexArray(0);
			}
		}

		virtual void update() override {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(10.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;

			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
	// class NormalMapGLSample : public GLSample<BasicNormalMap> {
	//  public:
	//	NormalMapGLSample(int argc, const char **argv) : GLSample<BasicNormalMap>(argc, argv) {}
	//	virtual void commandline(cxxopts::OptionAdder &options) override {
	//		options.add_options("Texture-Sample")("T,texture", "Texture Path",
	//											  cxxopts::value<std::string>()->default_value("texture.png"))(
	//			"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
	//	}
	//};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::BasicShadowMapping> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}