
#include "VideoTexture.h"


VideoTexture::VideoTexture(string file_name, string file_type) 
{
	for (int i = 0; i < 8; i++) {
		_files[i] = file_name;

		_files[i].append(std::to_string(i));
		_files[i].append(file_type);

	}

	loadTexture();
}


VideoTexture::~VideoTexture()
{

}

void VideoTexture::loadTexture()
{
	/*
	_width = 500;
	_height = 500;
	_init = true;

	for (unsigned int i = 0; i < 8; i++) {
		int nrComponents;

		_imageData[i] = stbi_load(_files[i].c_str(), &_width, &_height, &nrComponents, 0);
		if (!_imageData[i])
		{
			std::cout << "Texture failed to load at path: " << _files[i] << std::endl;
		}
	}
	glTexImage2D(getTexture(), 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(getTexture(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(getTexture(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(getTexture(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(getTexture(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/


	int width, height, nrComponents;
	unsigned char* data = stbi_load(_files[0].c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RED;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		else
			std::cout << "Unknown color components from texture: " << _files[0] << std::endl;

		glTexImage2D(getTexture(), 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glTexParameteri(getTexture(), GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(getTexture(), GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(getTexture(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(getTexture(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenerateMipmap(getTexture());
	}
	else
	{
		std::cout << "Texture failed to load at path: " << _files[0] << std::endl;
	}

	stbi_image_free(data);

}

void VideoTexture::updateVideo(double dt)
{
	if (!_init) return;

	_time += dt;
	if (_time > _frameRate) {
		_time -= _frameRate;

		// this->bind(0, getTexture());

		glTexSubImage2D(getTexture(), 0, 0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE, _imageData[_index]);
		glTexParameteri(getTexture(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(getTexture(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(getTexture(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(getTexture(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		_index = (_index + 1) % 8;
	}
}

GLenum VideoTexture::getTexture() {
	return GL_TEXTURE_2D;
}


