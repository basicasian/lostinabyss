
#pragma once


#include <string>
#include <GL/glew.h>
#include "../Utils.h"
#include "Texture.h"
#include <stb_image.h>

/*!
 * texture for showing video
 */
class VideoTexture 
{
protected:

	string _files[8];
	GLuint _textureId; // ID of texture

	unsigned char* _imageData[8];

	int _width, _height;
	bool _init = true;

	int _index = 0;
	double _frameRate = 1 / 30.;
	double _time = 0;


protected:
	GLenum getTexture();

public:

	/*!
	 * Creates a shadow depth map texture
	 * @param width: shadowmap width
	 * @param height: shadowmap height
	 */
	VideoTexture(string file_name, string file_type);

	~VideoTexture();

	void loadTexture();

	void updateVideo(double dt);

};
