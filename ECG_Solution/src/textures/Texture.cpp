
#include "Texture.h"


Texture::Texture(std::string file, GLuint depthMap, string type) : _init(true), _depthMap(depthMap), _type(type) {

	if (_type == "dds") {
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

	if (_type == "image") {
		// load image
		int width, height, nrChannels;
		unsigned char* data = stbi_load(file.c_str(), &width, &height, &nrChannels, 0);

		// generate texture
		glGenTextures(1, &_handle);
		glBindTexture(GL_TEXTURE_2D, _handle);

		// set the texture wrapping/filtering options (on the currently bound texture object)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}

		// free image memory
		stbi_image_free(data);
	}
	
}

void Texture::bind(unsigned int unit) {

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, _handle);

	glActiveTexture(GL_TEXTURE1 + unit);
	glBindTexture(GL_TEXTURE_2D, _depthMap);
}

Texture::~Texture()
{
}

Texture::Texture()
{
}