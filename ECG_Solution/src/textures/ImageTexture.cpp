
#include "ImageTexture.h"


ImageTexture::ImageTexture(string file, GLuint depthMap) : Texture(), _init(true), _file(file), _depthMap(depthMap)
{
	// load image
	int width, height, nrChannels;
	unsigned char* data = stbi_load(_file.c_str(), &width, &height, &nrChannels, 0);

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

