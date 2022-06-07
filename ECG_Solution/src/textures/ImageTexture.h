#pragma once

#include "stb_image.h"
#include <string>
#include <GL/glew.h>
#include "../Utils.h"
#include "Texture.h"
#include <iostream>

class ImageTexture : public Texture
{
protected:
	GLuint _handle;
	bool _init;

	GLuint _depthMap;

	string _file;

public:
	ImageTexture(string file, GLuint depthMap);



};
