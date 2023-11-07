#include "Importer/ImageImport.h"
#include "IOUtil.h"
#include <GL/glew.h>
#include <GLHelper.h>
#include <ImageLoader.h>
#include <magic_enum.hpp>
#include <stdexcept>

using namespace fragcore;
using namespace glsample;
TextureImporter::TextureImporter(IFileSystem *filesystem) : filesystem(filesystem) {}

int TextureImporter::loadImage2D(const std::string &path) {

	ImageLoader imageLoader;
	Ref<IO> io = Ref<IO>(this->filesystem->openFile(path.c_str(), IO::IOMode::READ));
	Image image = imageLoader.loadImage(io);
	io->close();

	return loadImage2DRaw(image);
}

int TextureImporter::loadImage2DRaw(const Image &image) {
	GLenum target = GL_TEXTURE_2D;
	GLuint texture;

	GLenum format, internalformat, type;

	switch (image.getFormat()) {
	case TextureFormat::RGB24:
		format = GL_RGB;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::BGR24:
		format = GL_BGR;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::ARGB32:
		break;
	case TextureFormat::BGRA32:
		format = GL_BGRA;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::RGBA32:

		format = GL_RGBA;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::RGBAFloat:
		format = GL_RGBA;
		internalformat = GL_RGBA16F;
		type = GL_FLOAT;
		break;
	case TextureFormat::RGBFloat:
		format = GL_RGB;
		type = GL_FLOAT;
		internalformat = GL_RGBA16F;
		break;
	case TextureFormat::Alpha8:
		format = GL_RED;
		type = GL_UNSIGNED_BYTE;
		internalformat = GL_R8;
		break;
	default:
		throw RuntimeException("None Supported Format: {}", magic_enum::enum_name(image.getFormat()));
	}

	FVALIDATE_GL_CALL(glGenTextures(1, &texture));

	FVALIDATE_GL_CALL(glBindTexture(target, texture));

	/*	*/
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	/*	wrap and filter	*/
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT));

	/*	*/
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16));

	float border[4] = {1, 1, 1, 1};
	FVALIDATE_GL_CALL(glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &border[0]));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_LOD, 1));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LOD, 5));

	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_LOD_BIAS, 0.0f));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 5));

	FVALIDATE_GL_CALL(
		glTexImage2D(target, 0, internalformat, image.width(), image.height(), 0, format, type, image.getPixelData()));

	FVALIDATE_GL_CALL(glGenerateMipmap(target));

	FVALIDATE_GL_CALL(glBindTexture(target, 0));

	return texture;
}

int TextureImporter::loadCubeMap(const std::string &px, const std::string &nx, const std::string &py,
								 const std::string &ny, const std::string &pz, const std::string &nz) {

	std::vector<std::string> paths = {px, nx, py, ny, pz, nz};
	return loadCubeMap(paths);
}

int TextureImporter::loadCubeMap(const std::vector<std::string> &paths) {
	ImageLoader imageLoader;

	GLenum target = GL_TEXTURE_CUBE_MAP;
	GLuint texture;
	FVALIDATE_GL_CALL(glGenTextures(1, &texture));

	FVALIDATE_GL_CALL(glBindTexture(target, texture));

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	/*	wrap and filter	*/
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16));

	float border[4] = {1, 1, 1, 1};
	FVALIDATE_GL_CALL(glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &border[0]));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_LOD, 1));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LOD, 5));

	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_LOD_BIAS, 0.0f));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 5));

	for (size_t i = 0; i < paths.size(); i++) {
		Ref<IO> io = Ref<IO>(filesystem->openFile(paths[i].c_str(), IO::IOMode::READ));
		Image image = imageLoader.loadImage(io);

		GLenum format, internalformat, type;
		switch (image.getFormat()) {
		case TextureFormat::RGB24:
		case TextureFormat::BGR24:
			format = GL_BGR;
			internalformat = GL_RGBA8;
			type = GL_UNSIGNED_BYTE;
			break;
		case TextureFormat::RGBA32:
		case TextureFormat::ARGB32:
		case TextureFormat::BGRA32:
			format = GL_RGBA;
			internalformat = GL_RGBA8;
			type = GL_UNSIGNED_BYTE;
			break;
		default:
			throw RuntimeException("Invalid");
		}

		FVALIDATE_GL_CALL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalformat, image.width(),
									   image.height(), 0, format, type, image.getPixelData()));
	}
	FVALIDATE_GL_CALL(glGenerateMipmap(target));

	FVALIDATE_GL_CALL(glBindTexture(target, 0));

	return 0;
}