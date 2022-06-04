#pragma once

#include "Shader.h"
#include "Utils.h"
#include "QuadGeometry.h"

class PostProcessing
{

protected:
	bool _created = false;

	// inital framebuffer
	GLuint _framebuffer;
	GLuint _textureColorbuffer[2];
	GLuint _frambufferDepthRbo;

	// ping pong buffer for blurring
	GLuint _pingpongFBO[2];
	GLuint _pingpongColorbuffers[2];

	// blur
	bool _horizontal = true, _first_iteration = true;
	unsigned int _amount = 10;
	float _exposure = 1.0f;

	// quad
	static QuadGeometry _quadGeometry;

public:


	PostProcessing(GLuint width, GLuint height);

	~PostProcessing();

	void bindInitalFrameBuffer();

	void blurFragments(Shader* blurShader);

	void renderBloomFinal(Shader* bloomResultShader);
};

QuadGeometry PostProcessing::_quadGeometry = QuadGeometry();