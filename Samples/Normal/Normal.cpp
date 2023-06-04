#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Normal : public GLSampleWindow {
	  public:
		Normal() : GLSampleWindow() {
			this->setTitle("Normal");
			this->normalSettingComponent = std::make_shared<NormalSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->normalSettingComponent);
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			/*	light source.	*/

			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			/*	*/
			float normalLength = 1.0f;

		} uniformBuffer;

		/*	*/
		GeometryObject plan;

		/*	Textures.	*/
		unsigned int diffuse_texture;

		/*	*/
		unsigned int graphic_program;
		unsigned int normal_vertex_program;
		unsigned int normal_triangle_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class NormalSettingComponent : public nekomimi::UIComponent {

		  public:
			NormalSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Normal Settings");
			}
			virtual void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Normal Setting");
				ImGui::DragFloat("Face Normal Length", &this->uniform.normalLength);
				ImGui::Checkbox("Triangle Normal", &this->showTriangleNormal);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;
			bool showTriangleNormal = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<NormalSettingComponent> normalSettingComponent;

		/*	Graphic shader paths.	*/
		const std::string vertexGraphicShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/texture/texture.frag.spv";

		/*	Normal shader paths.	*/
		const std::string vertexNormalShaderPath = "Shaders/normal/normal.vert.spv";
		const std::string geomtryNormalVertexShaderPath = "Shaders/normal/normal.geom.spv";
		const std::string geomtryNormalTriangleShaderPath = "Shaders/normal/normal-triangle.geom.spv";
		const std::string fragmentNormalShaderPath = "Shaders/normal/normal.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->normal_triangle_program);
			glDeleteProgram(this->normal_vertex_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			/*	Load shader source.	*/
			const std::vector<uint32_t> vertex_normal_binary =
				IOUtil::readFileData<uint32_t>(this->vertexNormalShaderPath, this->getFileSystem());
			const std::vector<uint32_t> geomtry_normal_binary =
				IOUtil::readFileData<uint32_t>(this->geomtryNormalVertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_normal_vertex_binary =
				IOUtil::readFileData<uint32_t>(this->fragmentNormalShaderPath, this->getFileSystem());
			const std::vector<uint32_t> geomtry_normal_triangle_binary =
				IOUtil::readFileData<uint32_t>(this->geomtryNormalTriangleShaderPath, this->getFileSystem());
			/*	*/
			const std::vector<uint32_t> vertex_graphic_binary =
				IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_graphic_binary =
				IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->normal_vertex_program = ShaderLoader::loadGraphicProgram(
				compilerOptions, &vertex_normal_binary, &fragment_normal_vertex_binary, &geomtry_normal_binary);
			this->normal_triangle_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_normal_binary, &fragment_normal_vertex_binary,
												 &geomtry_normal_triangle_binary);
			this->graphic_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary, &fragment_graphic_binary);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normal_vertex_program);
			unsigned int uniform_buffer_index =
				glGetUniformBlockIndex(this->normal_vertex_program, "UniformBufferBlock");
			glUniformBlockBinding(this->normal_vertex_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normal_triangle_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->normal_triangle_program, "UniformBufferBlock");
			glUniformBlockBinding(this->normal_triangle_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->graphic_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateTorus(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glGenBuffers(1, &this->plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->plan.nrIndicesElements = indices.size();

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

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->normalSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	Draw geometry.  */
			{
				glUseProgram(this->graphic_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glDepthMask(GL_TRUE);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
				glUseProgram(0);
			}

			/*  Draw surface normals.   */
			{
				glDepthMask(GL_FALSE);
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glBindVertexArray(this->plan.vao);
				if (this->normalSettingComponent->showTriangleNormal) {
					glUseProgram(this->normal_triangle_program);
					glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				} else {
					glUseProgram(this->normal_vertex_program);
					glDrawElements(GL_POINTS, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				}

				glBindVertexArray(0);
				glUseProgram(0);
				glDepthMask(GL_TRUE);
			}
		}

		virtual void update() override {

			int width, height;
			this->getSize(&width, &height);

			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(10.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			this->uniformBuffer.ViewProj = this->uniformBuffer.proj * this->uniformBuffer.view;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class NormalGLSample : public GLSample<Normal> {
	  public:
		NormalGLSample() : GLSample<Normal>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::NormalGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}