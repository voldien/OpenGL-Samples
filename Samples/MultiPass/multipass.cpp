#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class MultiPass : public GLSampleWindow {
	  public:
		MultiPass() : GLSampleWindow() { this->setTitle("MultiPass"); }

		/*	*/
		GeometryObject plan;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
		} mvp;

		unsigned int mandelbrot_framebuffer;
		unsigned int texture_program;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		std::vector<unsigned int> gl_textures;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		/*	*/
		const std::string vertexShaderPath = "Shaders/multipass/multipass.vert";
		const std::string fragmentShaderPath = "Shaders/multipass/multipass.frag";

		virtual void Release() override {
			glDeleteProgram(this->texture_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->texture_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->texture_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->texture_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->texture_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->texture_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->texture_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateCube(1, vertices, indices);

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

			/*	*/
			glGenFramebuffers(1, &this->mandelbrot_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);

			gl_textures.resize(4);

			glGenTextures(this->gl_textures.size(), this->gl_textures.data());
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(gl_textures.size());
			for (size_t i = 0; i < gl_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->gl_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width(), this->height(), 0, GL_RGB, GL_UNSIGNED_BYTE,
							 nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->gl_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			glDrawBuffers(drawAttach.size(), drawAttach.data());

			/*  Validate if created properly.*/
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual void draw() override {

			this->update();
			/*	*/
			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				/*	*/
				glUseProgram(this->texture_program);

				/*	Draw triangle*/
				// glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
				// glEnableVertexAttribArrayARB(0);
				// glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
				// glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			}

			/*	Blit image targets to screen.	*/
			// For each.
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->mandelbrot_framebuffer);
			for (size_t i = 0; i < gl_textures.size(); i++) {
				glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
				glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height, 0, 0, width,
								  height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}
		}

		virtual void update() {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::MultiPass> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}