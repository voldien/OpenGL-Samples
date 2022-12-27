
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SobelEdgeDetection : public GLSampleWindow {
	  public:
		SobelEdgeDetection() : GLSampleWindow() {
			this->setTitle("SobelEdgeDetection");
			shadowSettingComponent = std::make_shared<BasicShadowMapSettingComponent>(this->uniform);
			this->addUIComponent(shadowSettingComponent);
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			alignas(16) glm::mat4 lightModelProject;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec3 lightPosition;

			float bias = 0.01f;
			float shadowStrength = 1.0f;
		} uniform;

		/*  */
		unsigned int colorFramebuffer;
		unsigned int colorTexture;
		unsigned int colorWidth = 1024;
		unsigned int colorHeight = 1024;

		unsigned int sobel_texture;

		/*  */
		GeometryObject plan;
		GeometryObject cube;
		GeometryObject sphere;

		unsigned int graphic_program;
		unsigned int sobel_program;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class BasicShadowMapSettingComponent : public nekomimi::UIComponent {
		  public:
			BasicShadowMapSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Basic Shadow Mapping Settings");
			}
			virtual void draw() override {
				ImGui::DragFloat("Shadow Strength", &this->uniform.shadowStrength, 1, 0.0f, 1.0f);
				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 1.0f);
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				// TODO add support for multiple shadow resolutions.
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<BasicShadowMapSettingComponent> shadowSettingComponent;

		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/shadowmap/texture.vert";
		const std::string fragmentGraphicShaderPath = "Shaders/shadowmap/texture.frag";

		const std::string sobelEdgeDetectionComputeShaderPath = "Shaders/sobeledgedetection/sobeledgedetection.comp";

		virtual void Release() override {
			/*  */
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->sobel_program);

			/*  */
			glDeleteBuffers(1, &this->uniform_buffer);

			/*  */
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
			/*  */
			glDeleteVertexArrays(1, &this->sphere.vao);
			/*  */
			glDeleteVertexArrays(1, &this->cube.vao);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	*/
			std::vector<char> vertex_source = IOUtil::readFileString(vertexGraphicShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				IOUtil::readFileString(fragmentGraphicShaderPath, this->getFileSystem());

			/*  */
			std::vector<char> mandelbrot_source =
				IOUtil::readFileString(sobelEdgeDetectionComputeShaderPath, this->getFileSystem());

			/*	Load shaders	*/
			this->graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);
			this->sobel_program = ShaderLoader::loadComputeProgram({&mandelbrot_source});

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->sobel_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	Setup Sobel Program.    */
			glUseProgram(this->sobel_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->sobel_program, "UniformBufferBlock");
			glUniformBlockBinding(this->sobel_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1iARB(glGetUniformLocation(this->mandelbrot_program, "img_output"), 0);
			glUniform1iARB(glGetUniformLocation(this->mandelbrot_program, "img_output"), 1);
			glUseProgram(0);

			/*	Setup graphic Program.  */
			glUseProgram(this->graphic_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->graphic_program, "ShadowTexture"), 1);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align(uniformBufferSize, (size_t)minMapBufferSize);

			// Create uniform buffer.
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create shadow map.	*/
				glGenFramebuffers(1, &colorFramebuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, colorFramebuffer);

				/*	*/
				glGenTextures(1, &this->colorTexture);
				glBindTexture(GL_TEXTURE_2D, this->colorTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, colorWidth, colorHeight, 0, GL_DEPTH_COMPONENT,
							 GL_FLOAT, nullptr);
				/*	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, colorTexture, 0);
				int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {

					/*  Delete  */
					glDeleteFramebuffers(1, &colorFramebuffer);
					// TODO add error message.
					throw RuntimeException("Failed to create framebuffer, {}", glewGetErrorString(frstat));
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

			/*	Write each of the geometries to the single VBO.	*/
			glBufferSubData(GL_ARRAY_BUFFER, 0, planVertices.size() * sizeof(ProceduralGeometry::Vertex),
							planVertices.data());
			glBufferSubData(GL_ARRAY_BUFFER, planVertices.size() * sizeof(ProceduralGeometry::Vertex),
							cubeVertices.size() * sizeof(ProceduralGeometry::Vertex), cubeVertices.data());
			glBufferSubData(GL_ARRAY_BUFFER,
							(planVertices.size() + cubeVertices.size()) * sizeof(ProceduralGeometry::Vertex),
							sphereVertices.size() * sizeof(ProceduralGeometry::Vertex), sphereVertices.data());

			/*	*/
			glGenBuffers(1, &plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						 (planIndices.size() + cubeIndices.size() + sphereIndices.size()) * sizeof(indices[0]), nullptr,
						 GL_STATIC_DRAW);

			/*	*/
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, planIndices.size() * sizeof(indices[0]), planIndices.data());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, planIndices.size() * sizeof(indices[0]),
							cubeIndices.size() * sizeof(indices[0]), cubeIndices.data());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (planIndices.size() + cubeIndices.size()) * sizeof(indices[0]),
							sphereIndices.size() * sizeof(indices[0]), sphereIndices.data());

			/*	Setup */
			this->plan.nrIndicesElements = planIndices.size();
			this->plan.indices_offset = 0;
			this->cube.nrIndicesElements = cubeIndices.size();
			this->cube.indices_offset = planVertices.size();
			this->sphere.nrIndicesElements = sphereIndices.size();
			this->sphere.indices_offset = planVertices.size() + cubeVertices.size();

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
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

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
				this->uniform.lightModelProject = lightSpaceMatrix;

				glBindFramebuffer(GL_FRAMEBUFFER, this->colorFramebuffer);

				/*  */
				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, this->colorWidth, this->colorHeight);
				glUseProgram(this->sobel_program);
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				/*	Setup the shadow.	*/

				glBindVertexArray(this->plan.vao);

				/*  Draw Shadow Object.  */
				glDrawElementsBaseVertex(GL_TRIANGLES, plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 plan.indices_offset);
				glDrawElementsBaseVertex(GL_TRIANGLES, cube.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 cube.indices_offset);
				glDrawElementsBaseVertex(GL_TRIANGLES, sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										 sphere.indices_offset);

				glBindVertexArray(0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			/*	Compute sobel edge detection.	*/
			{
				glUseProgram(this->sobel_program);
				glBindImageTexture(0, this->colorTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
				glBindImageTexture(1, this->sobel_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				int localWorkGroupSize[3];

				glGetProgramiv(this->sobel_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

				glDispatchCompute(std::ceil(this->colorWidth / (float)localWorkGroupSize[0]),
								  std::ceil(this->colorHeight / (float)localWorkGroupSize[1]), 1);

				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
			}

			/*	Blit sobel result framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->colorFramebuffer);

			glBlitFramebuffer(0, 0, this->colorWidth, this->colorHeight, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
							  GL_NEAREST);
		}

		virtual void update() {
			/*	Update Camera.	*/
			camera.update(getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			// this->mvp.model = glm::scale(this->mvp.model, glm::vec3(1.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(p, &this->uniform, sizeof(uniform));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	class SobelEdgeDetectionGLSample : public GLSample<SobelEdgeDetection> {
	  public:
		SobelEdgeDetectionGLSample(int argc, const char **argv) : GLSample<SobelEdgeDetection>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SobelEdgeDetectionGLSample sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}