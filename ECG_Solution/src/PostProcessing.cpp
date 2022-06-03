#include "PostProcessing.h"

PostProcessing::PostProcessing(GLuint width, GLuint height)
{
	// create a framebuffer object
	glGenFramebuffers(1, &_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

	// create texture
	glGenTextures(1, &_textureHandle);
	glBindTexture(GL_TEXTURE_2D, _textureHandle);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textureHandle, 0);

	// create render buffer
	glGenRenderbuffers(1, &_frambufferDepthRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, _frambufferDepthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// attach the renderbuffer object to the depth and stencil attachment of the framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _frambufferDepthRbo);

	_created = true;
}

PostProcessing::~PostProcessing()
{
	if (_created) {
		glDeleteFramebuffers(1, &_framebuffer);
		glDeleteTextures(1, &_textureHandle);
		glDeleteRenderbuffers(1, &_frambufferDepthRbo);
	}
}


void PostProcessing::bindInitalFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
}

GLuint PostProcessing::getHandle()
{
	return _textureHandle;
}