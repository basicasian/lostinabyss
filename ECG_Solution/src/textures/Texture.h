/*
* Copyright 2020 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/
#pragma once

#include "stb_image.h"
#include <string>
#include <GL/glew.h>
#include "../Utils.h"

/*!
 * 2D texture
 */
class Texture
{
protected:
	boolean _init;

	GLuint _handle;
	GLuint _depthMap;
	GLuint _normalMap;

	string _type;
	int _width, _height;

	// for video
	int _frameNumber;
	unsigned char* _imageData[999];

	double _frameRate = 1 / 30.;
	double _time = 0;
	int _index = 0;


public:
	/*!
	 * Creates a texture from a file
	 */
	Texture(std::string file, GLuint depthMap, string type);

	Texture();

	~Texture();

	/*!
	 * Activates the texture unit and binds this texture
	 * @param unit: the texture unit
	 */
	void bind(unsigned int unit);

	void bindNormal(unsigned int unit);

	GLuint getHandle();

	void setNormalMap(GLuint normalMap);

	void updateVideo(double dt);

};
