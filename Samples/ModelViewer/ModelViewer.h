#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 */
	class ModelViewer : public GLSampleWindow {
	  public:
		ModelViewer();

		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		unsigned int vbo;
		unsigned int vao;

		unsigned int triangle_program;

		const std::string vertexShaderPath = "Shaders/triangle/triangle.vert.spv";
		const std::string fragmentShaderPath = "Shaders/triangle/triangle.frag.spv";

		const std::vector<Vertex> vertices = {
			{0.5f, -0.5f, 0.0f, 1.0f, 0.0f}, /*	Vertex (2), Color(3)	*/
			{0.0f, 0.5f, 1.0f, 1.0f, 1.0f},	 /*	Vertex (2), Color(3)	*/
			{-0.5f, -0.5f, 0.0f, 0.0f, 1.0f} /*	Vertex (2), Color(3)	*/
		};
		void Release() override;

		void Initialize() override;

		void draw() override;
		void update() override;
	};

} // namespace glsample
