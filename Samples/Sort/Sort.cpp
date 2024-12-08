#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Sort : public GLSampleWindow {
	  public:
		Sort() : GLSampleWindow() {
			this->setTitle("Sort Compute");

			/*	*/
			this->sortSettingComponent = std::make_shared<SortSettingComponent>(this->params);
			this->addUIComponent(this->sortSettingComponent);
		}

		typedef struct uniform_buffer_block_t {
			float posX, posY;
			float mousePosX, mousePosY;
			float zoom = 1.0f; /*	*/
			float c = 0;	   /*	*/
			float ci = 1;	   /*	*/
			int nrSamples = 128;
		} UniformBuferBlock;
		UniformBuferBlock params;

		/*	*/
		const size_t round_robin_size = 2;
		unsigned int mandelbrot_framebuffer;
		unsigned int sort_mergesort_program;
		unsigned int julia_program;
		unsigned int mandelbrot_texture; // TODO add round robin.
		unsigned int mandelbrot_texture_width;
		unsigned int mandelbrot_texture_height;

		/*	*/
		int localWorkGroupSize[3];

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 3;
		unsigned int current_cells_buffer_binding = 0;
		unsigned int previous_cells_buffer_binding = 1;
		unsigned int image_output_binding = 2;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block_t);

		class SortSettingComponent : public nekomimi::UIComponent {

		  public:
			SortSettingComponent(struct uniform_buffer_block_t &uniform) : uniform(uniform) {
				this->setName("Sort Settings");
			}
			void draw() override {
				ImGui::DragInt("Number of Samples", &this->uniform.nrSamples, 1, 0, 2048);
				ImGui::DragFloat2("C", &this->uniform.c);
				ImGui::DragFloat("Zoom", &this->uniform.zoom, 1.0f, 0.001, 10.0f);
				ImGui::DragInt("Program", &this->program, 1.0f, 0, 1);

				if (ImGui::Button("Reset")) {
					/*	*/
				}
			}

			bool showWireFrame = false;
			int program{};

		  private:
			struct uniform_buffer_block_t &uniform;
		};
		std::shared_ptr<SortSettingComponent> sortSettingComponent;

		/*	*/
		const std::string computeMergeSortShaderPath = "Shaders/sort/merge.comp.spv";
		const std::string computeJuliaShaderPath = "Shaders/sort/julia.comp.spv";

		/*	Particle.	*/
		const std::string sortLineVertexShaderPath = "Shaders/sort/line.vert.spv";
		const std::string sortLineGeometryShaderPath = "Shaders/sort/line.geom.spv";
		const std::string sortLineFragmentShaderPath = "Shaders/sort/line.frag.spv";

		void Release() override {

			glDeleteProgram(this->sort_mergesort_program);
			glDeleteProgram(this->julia_program);
			glDeleteFramebuffers(1, &this->mandelbrot_framebuffer);
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteTextures(1, (const GLuint *)&this->mandelbrot_texture);
		}

		void Initialize() override {

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> mergesort_binary =
					IOUtil::readFileData<uint32_t>(this->computeMergeSortShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->sort_mergesort_program = ShaderLoader::loadComputeProgram(compilerOptions, &mergesort_binary);

				const std::vector<uint32_t> julia_binary =
					IOUtil::readFileData<uint32_t>(this->computeJuliaShaderPath, this->getFileSystem());

				/*	Load shader	*/
				this->julia_program = ShaderLoader::loadComputeProgram(compilerOptions, &julia_binary);
			}

			/*	*/
			{
				glUseProgram(this->sort_mergesort_program);
				int uniform_buffer_index = glGetUniformBlockIndex(this->sort_mergesort_program, "UniformBufferBlock");
				glUniformBlockBinding(this->sort_mergesort_program, uniform_buffer_index, this->uniform_buffer_binding);

				int buffer_read_index =
					glGetProgramResourceIndex(this->sort_mergesort_program, GL_SHADER_STORAGE_BLOCK, "ReadCells");
				int buffer_write_index =
					glGetProgramResourceIndex(this->sort_mergesort_program, GL_SHADER_STORAGE_BLOCK, "WriteCells");

				/*	*/
				glShaderStorageBlockBinding(this->sort_mergesort_program, buffer_read_index,
											this->current_cells_buffer_binding);
				glShaderStorageBlockBinding(this->sort_mergesort_program, buffer_write_index,
											this->previous_cells_buffer_binding);

				glGetProgramiv(this->sort_mergesort_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
				glUseProgram(0);

				/*	*/
				glUseProgram(this->julia_program);
				uniform_buffer_index = glGetUniformBlockIndex(this->julia_program, "UniformBufferBlock");
				glUniformBlockBinding(this->julia_program, uniform_buffer_index, this->uniform_buffer_binding);
				glUniform1i(glGetUniformLocation(this->julia_program, "img_output"), 0);
				glGetProgramiv(this->julia_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
				glUseProgram(0);
			}

			/*	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = Math::align<size_t>(this->uniformAlignBufferSize, minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			{
				glGenFramebuffers(1, &this->mandelbrot_framebuffer);

				glGenTextures(1, &this->mandelbrot_texture);
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->mandelbrot_texture_width = width;
			this->mandelbrot_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);
			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->mandelbrot_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->mandelbrot_texture, 0);

			const GLenum drawAttach = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &drawAttach);

			/*  Validate if created properly.*/
			const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			{

				if (this->sortSettingComponent->program == 0) {
					glUseProgram(this->sort_mergesort_program);
				} else {
					glUseProgram(this->julia_program);
				}

				glBindImageTexture(0, this->mandelbrot_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				glDispatchCompute(std::ceil(this->mandelbrot_texture_width / (float)localWorkGroupSize[0]),
								  std::ceil(this->mandelbrot_texture_height / (float)localWorkGroupSize[1]), 1);

				/*	Wait in till image has been written.	*/
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
			}

			/*	Draw sort */
		}

		void update() override {
			// const float elapsedTime = this->getTimer().getElapsed<float>();
			// this->camera.update(this->getTimer().deltaTime<float>());

			// /*	*/
			// {
			// 	const float xHalf = this->uniformStageBuffer.particleSetting.particleBox.x / 2.f;
			// 	const float yHalf = this->uniformStageBuffer.particleSetting.particleBox.y / 2.f;
			// 	glm::mat4 proj = glm::ortho(-xHalf, xHalf, -yHalf, yHalf, -10.0f, 10.0f);

			// 	glm::mat4 viewMatrix = glm::translate(glm::vec3(-xHalf, -yHalf, 0));
			// 	/*	*/
			// 	this->uniformStageBuffer.proj = proj;

			// 	this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();

			// 	this->uniformStageBuffer.model = glm::mat4(1.0f);
			// 	this->uniformStageBuffer.view = viewMatrix;
			// 	this->uniformStageBuffer.modelViewProjection =
			// 		this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

			// 	if (this->getInput().getMousePressed(Input::MouseButton::LEFT_BUTTON)) {
			// 		int x, y;
			// 		this->getInput().getMousePosition(&x, &y);
			// 		this->uniformStageBuffer.motion.normalizedPos =
			// 			glm::vec2(1, 1) - (glm::vec2(x, y) / glm::vec2(this->width(), this->height()));
			// 		this->uniformStageBuffer.motion.normalizedPos.x =
			// 			1.0f - this->uniformStageBuffer.motion.normalizedPos.x;
			// 	}
			// }

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount()) % nrUniformBuffer) * uniformAlignBufferSize,
				uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &params, sizeof(params));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class SortGLSample : public GLSample<Sort> {
	  public:
		SortGLSample() : GLSample<Sort>() {}

		void customOptions(cxxopts::OptionAdder &options) override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SortGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
