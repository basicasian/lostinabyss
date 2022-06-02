
#include "Texture.h"

Texture::Texture(std::string file, GLuint depthMap) : _init(true), _depthMap(depthMap) {

	DDSImage image = loadDDS(file.c_str());

	glGenTextures(1, &_handle);
	glBindTexture(GL_TEXTURE_2D, _handle);

	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
		image.width, image.height, 0, image.size, image.data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture()
{
}

void Texture::bind(unsigned int unit) {

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, _handle);

	glActiveTexture(GL_TEXTURE1 + unit);
	glBindTexture(GL_TEXTURE_2D, _depthMap);
}