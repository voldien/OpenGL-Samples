#include "Importer/ImageImport.h"
#include "FragDef.h"
#include <GL/glew.h>
#include <GLHelper.h>
#include <ImageLoader.h>
#include <cmath>
#include <limits>
#include <magic_enum.hpp>

using namespace fragcore;
using namespace glsample;

TextureImporter::TextureImporter(IFileSystem *filesystem) : filesystem(filesystem) {
	glCreateBuffers(this->pbos.size(), this->pbos.data());
	for (size_t i = 0; i < this->pbos.size(); i++) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, this->pbos[i]);
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

TextureImporter::~TextureImporter() { glDeleteBuffers(this->pbos.size(), this->pbos.data()); }

unsigned int TextureImporter::loadImage2D(const std::string &path, const ColorSpace colorSpace,
										  const TextureCompression compression) {

	ImageLoader imageLoader;
	Ref<IO> io = Ref<IO>(this->filesystem->openFile(path.c_str(), IO::IOMode::READ));
	Image image = imageLoader.loadImage(io);
	io->close();

	const unsigned int texture_index = this->loadImage2DRaw(image, colorSpace);
	if (texture_index >= 0) {
		glObjectLabel(GL_TEXTURE, texture_index, path.size(), path.data());
	}
	return texture_index;
}

unsigned int TextureImporter::loadImage2DRaw(const Image &image, const ColorSpace colorSpace,
											 const TextureCompression compression) {

	GLenum target = GL_TEXTURE_2D;
	GLuint texture = 0;

	GLenum format = 0, internalformat = 0, type = 0;

	// TODO: relocat for reuse.
	switch (image.getFormat()) {
	case ImageFormat::RGB24: /*	Multiple Channels.	*/
		format = GL_RGB;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case ImageFormat::BGR24:
		format = GL_BGR;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case ImageFormat::ARGB32:
		break;
	case ImageFormat::BGRA32:
		format = GL_BGRA;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case ImageFormat::RGBA32:
		format = GL_RGBA;
		internalformat = GL_RGBA8;
		type = GL_UNSIGNED_BYTE;
		break;
	case ImageFormat::RGBAFloat:
		format = GL_RGBA;
		internalformat = GL_RGBA16F;
		type = GL_FLOAT;
		break;
	case ImageFormat::RGBFloat:
		format = GL_RGB;
		type = GL_FLOAT;
		internalformat = GL_RGBA16F;
		break;
	case ImageFormat::R8:
	case ImageFormat::Alpha8: /*	Single Channel.	*/
		format = GL_RED;
		type = GL_UNSIGNED_BYTE;
		internalformat = GL_R8;
		break;
	case ImageFormat::RFloat:
		format = GL_RED;
		type = GL_FLOAT;
		internalformat = GL_R32F;
		break;
	case ImageFormat::R16:
		format = GL_RED_INTEGER;
		type = GL_SHORT;
		internalformat = GL_R16I;
		break;
	case ImageFormat::R16U:
		format = GL_RED_INTEGER;
		type = GL_UNSIGNED_SHORT;
		internalformat = GL_R16UI;
		break;
	case ImageFormat::R32:
		format = GL_RED_INTEGER;
		type = GL_INT;
		internalformat = GL_R32I;
		break;
	case ImageFormat::R32U:
		format = GL_RED_INTEGER;
		type = GL_UNSIGNED_INT;
		internalformat = GL_R32UI;
		break;
	default:
		throw NotSupportedException("None Supported Image Format: {}", magic_enum::enum_name(image.getFormat()));
	}

	if (colorSpace == ColorSpace::SRGB) {
		switch (internalformat) {
		case GL_RGBA8:
			internalformat = GL_SRGB8_ALPHA8;
			break;
		case GL_RGB8:
			internalformat = GL_SRGB8;
			break;
		case GL_RGBA16F:
			internalformat = GL_RGBA16F;
			break;
		case GL_R8:
			internalformat = GL_SR8_EXT; // GL_SLUMINANCE
			break;
		default:
			throw NotSupportedException("None Supported SRGB Format: {} ({})", magic_enum::enum_name(image.getFormat()),
										internalformat);
		}
	}

	/*	Find best match compressed type.	*/
	if (compression != TextureCompression::None) {
		if (compression == TextureCompression::Default) {
			switch (internalformat) {
			case GL_R8:
				internalformat = GL_COMPRESSED_RED;
				break;
			case GL_RGBA8:
				internalformat = GL_COMPRESSED_RGBA_ARB;
				break;
			case GL_RGB8:
				internalformat = GL_COMPRESSED_RGB_ARB;
				break;
			case GL_SRGB8_ALPHA8:
				internalformat = GL_COMPRESSED_SRGB_ALPHA;
				break;
			case GL_SRGB8:
				internalformat = GL_COMPRESSED_SRGB;
				break;
			case GL_RGB16F:
			case GL_RGB32F:
			case GL_RGBA16F:
			case GL_RGBA32F:
				internalformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
				break;
			default:
				break; /*	Can't find any suitable compression type.	*/
				throw NotSupportedException("None Supported Compression Format: {} ({})",
											magic_enum::enum_name(image.getFormat()), internalformat);
			}
		} else if (compression == TextureCompression::ASTC) {
			switch (internalformat) {
			case GL_R8:
				internalformat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
				break;
			case GL_RGBA8:
				internalformat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
				break;
			case GL_RGB8:
				internalformat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
				break;
			case GL_SRGB8_ALPHA8:
				internalformat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
				break;
			case GL_SRGB8:
				internalformat = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
				break;
			case GL_RGB16F:
			case GL_RGB32F:
			case GL_RGBA16F:
			case GL_RGBA32F:
				internalformat = GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
				break;
			default:
				throw NotSupportedException("None Supported ASTC Format: {} ({})",
											magic_enum::enum_name(image.getFormat()), internalformat);
			}
		}
	}

	const size_t power_of_2 = std::floor(std::log(Math::max(image.width(), image.height())) / std::log(2));
	const size_t max_mipmap = Math::clamp<size_t>(power_of_2 - 4, 0, std::numeric_limits<size_t>::max());

	GLuint currentPBO = this->pbos[current_pbo++];
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, currentPBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, image.getSize(), nullptr, GL_STREAM_COPY);
	void *ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER_ARB, 0, image.getSize(),
								 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	memcpy(ptr, image.getPixelData(), image.getSize());
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	FVALIDATE_GL_CALL(glGenTextures(1, &texture));

	FVALIDATE_GL_CALL(glBindTexture(target, texture));

	/*	Offload with PBO buffer.	*/

	/*	Alignment.	*/
	FVALIDATE_GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	FVALIDATE_GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 4));

	/*	wrap and filter.	*/
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT));

	/*	Filtering.	*/
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	/*	*/
	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16));

	/*	*/
	const float border[4] = {1, 1, 1, 1};
	FVALIDATE_GL_CALL(glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &border[0]));

	/*	*/
	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_MIN_LOD, -1000));
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LOD, 1000));

	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_LOD_BIAS, 0.0f));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, max_mipmap));

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, currentPBO);
	FVALIDATE_GL_CALL(glTexImage2D(target, 0, internalformat, image.width(), image.height(), 0, format, type, nullptr));
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	FVALIDATE_GL_CALL(glGenerateMipmap(target));

	FVALIDATE_GL_CALL(glBindTexture(target, 0));

	current_pbo = (current_pbo + 1) % pbos.size();

	return texture;
}

unsigned int TextureImporter::loadCubeMap(const std::string &px, const std::string &nx, const std::string &py,
										  const std::string &ny, const std::string &pz, const std::string &nz,
										  const ColorSpace colorSpace, const TextureCompression compression) {

	std::vector<std::string> paths = {px, nx, py, ny, pz, nz};
	return loadCubeMap(paths, colorSpace, compression);
}

unsigned int TextureImporter::loadCubeMap(const std::vector<std::string> &paths, const ColorSpace colorSpace,
										  const TextureCompression compression) {
	ImageLoader imageLoader;

	const GLenum target = GL_TEXTURE_CUBE_MAP;
	GLuint texture = 0;

	FVALIDATE_GL_CALL(glGenTextures(1, &texture));

	FVALIDATE_GL_CALL(glBindTexture(target, texture));

	FVALIDATE_GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	FVALIDATE_GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 4));

	/*	wrap and filter	*/
	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	/*	*/
	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16));

	const float border[4] = {1, 1, 1, 1};
	FVALIDATE_GL_CALL(glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &border[0]));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_LOD, 1));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LOD, 5));

	FVALIDATE_GL_CALL(glTexParameterf(target, GL_TEXTURE_LOD_BIAS, 0.0f));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));

	FVALIDATE_GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 5));

	// const size_t power_of_2 = std::floor(std::log(Math::max(image.width(), image.height())) / std::log(2));
	// const size_t max_mipmap = Math::clamp<size_t>(power_of_2 - 4, 0, std::numeric_limits<size_t>::max());

	for (size_t i = 0; i < paths.size(); i++) {
		Ref<IO> io = Ref<IO>(filesystem->openFile(paths[i].c_str(), IO::IOMode::READ));
		Image image = imageLoader.loadImage(io);

		GLenum format = 0, internalformat = 0, type = 0;
		switch (image.getFormat()) {
		case ImageFormat::RGB24:
		case ImageFormat::BGR24:
			format = GL_BGR;
			internalformat = GL_RGBA8;
			type = GL_UNSIGNED_BYTE;
			break;
		case ImageFormat::RGBA32:
		case ImageFormat::ARGB32:
		case ImageFormat::BGRA32:
			format = GL_RGBA;
			internalformat = GL_RGBA8;
			type = GL_UNSIGNED_BYTE;
			break;
		default:
			throw RuntimeException("None Supported Format: {}", magic_enum::enum_name(image.getFormat()));
		}

		FVALIDATE_GL_CALL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalformat, image.width(),
									   image.height(), 0, format, type, image.getPixelData()));
	}
	FVALIDATE_GL_CALL(glGenerateMipmap(target));

	FVALIDATE_GL_CALL(glBindTexture(target, 0));

	return 0;
}