
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
		int nrChannels;
		unsigned char* data = stbi_load(file.c_str(), &_width, &_height, &nrChannels, 0);

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}

		// free image memory
		stbi_image_free(data);
	}

	if (type == "video") {

		// load images	
		_width = 500;
		_height = 226;

		// get number of frames
		unsigned first = file.find("_");
		unsigned last = file.find(".");
		std::string filepre = file.substr(0, first+1);
		std::string filepost = file.substr(last);

		std::string frameNumber = file.substr(first+1, last - first -1);
		_frameNumber = std::stoi(frameNumber);

		for (int i = 0; i <= _frameNumber; i++) {
			
			std::string path;
			path.append(filepre);

			// create string of file
			if (i < 10) {
				path.append("0");
			} 
			// create string of file
			if (i < 100 && _frameNumber > 100) {
				path.append("0");
			}
			path.append(std::to_string(i));
			path.append(filepost);

			// std::cout << path << std::endl;

			// load file to data structure
			int width, height, nrChannels;

			stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis
			_imageData[i] = stbi_load(path.c_str(), &_width, &_height, &nrChannels, 0);
			stbi_set_flip_vertically_on_load(false); // set it right for other textures
			if (!_imageData[i])
			{
				std::cout << "Texture failed to load at path: " << path << std::endl;
			}
		}
		// generate texture
		glGenTextures(1, &_handle);
		glBindTexture(GL_TEXTURE_2D, _handle);

		// set the texture wrapping/filtering options (on the currently bound texture object)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, _imageData[_index]);
	}
	
}

void Texture::bind(unsigned int unit) {

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, _handle);

	glActiveTexture(GL_TEXTURE1 + unit);
	glBindTexture(GL_TEXTURE_2D, _depthMap);
}

void Texture::bindNormal(unsigned int unit) {

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, _handle);

	glActiveTexture(GL_TEXTURE1 + unit);
	glBindTexture(GL_TEXTURE_2D, _depthMap);

	glActiveTexture(GL_TEXTURE2 + unit);
	glBindTexture(GL_TEXTURE_2D, _normalMap);
}

void Texture::setNormalMap(GLuint normalMap) {
	_normalMap = normalMap;
}

GLuint Texture::getHandle() {
	return _handle;
}

Texture::~Texture()
{
}

Texture::Texture()
{
}

void Texture::updateVideo(double dt)
{
	if (!_init) return;

	_time += dt;
	if (_time > _frameRate) {
		_time -= _frameRate;

		// activate texture 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _handle);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, _imageData[_index]);
		// glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE, _imageData[_index]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		_index = (_index + 1) % _frameNumber;
	}
}