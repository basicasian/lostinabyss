#include "ShadowmapTexture.h"

ShadowMapTexture::ShadowMapTexture(GLuint shadowWidth, GLuint shadowHeight) :
	_shadowWidth(shadowWidth),
	_shadowHeight(shadowHeight)
{
	// create a framebuffer object for rendering the depth map
	glGenFramebuffers(1, &_framebuffer);

	// create 2d textur from framebuffer's depth buffer: 
	glGenTextures(1, &_handle);
	glBindTexture(GL_TEXTURE_2D, _handle);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _shadowWidth, _shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// for area out of range, to not show it in shadow
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _handle, 0);
	glDrawBuffer(GL_NONE); // no colour
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

}

ShadowMapTexture::~ShadowMapTexture()
{
}

GLuint ShadowMapTexture::getHandle()
{
	return _handle;
}

GLuint ShadowMapTexture::getDepthFBO()
{
	return _framebuffer;
}

void ShadowMapTexture::activate()
{
	glViewport(0, 0, _shadowWidth, _shadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER,_framebuffer);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
}

void ShadowMapTexture::resetViewPort()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// reset viewport 
	glViewport(0, 0, _shadowWidth, _shadowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}