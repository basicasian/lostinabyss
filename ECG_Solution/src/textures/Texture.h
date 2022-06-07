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

	string _type;

public:
	/*!
	 * Creates a texture from a file
	 * @param file: path to the texture file (a DSS image)
	 */
	Texture(std::string file, GLuint depthMap, string type);
	
	Texture();

	~Texture();
		 	 
	/*!
	 * Activates the texture unit and binds this texture
	 * @param unit: the texture unit
	 */
	void bind(unsigned int unit);

};
