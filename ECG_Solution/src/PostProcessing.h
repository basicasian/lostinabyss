#pragma once

#include "Shader.h"
#include "Utils.h"

class PostProcessing
{

protected:
	bool _created = false;

	GLuint _framebuffer;
	GLuint _textureHandle;
	GLuint _frambufferDepthRbo;

public:


	PostProcessing(GLuint width, GLuint height);

	~PostProcessing();

	GLuint getHandle();

	void bindInitalFrameBuffer();
};
