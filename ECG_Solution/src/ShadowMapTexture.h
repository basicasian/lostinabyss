
#pragma once


#include <string>
#include <GL/glew.h>
#include "Utils.h"

/*!
 * depth map texture for shadow mapping
 */
class ShadowMapTexture
{
protected:

	GLuint _framebuffer = -1;

	GLuint _handle;

	GLuint _shadowWidth;
	GLuint _shadowHeight;

public:

	/*!
	 * Creates a shadow depth map texture
	 * @param width: shadowmap width
	 * @param height: shadowmap height
	 */
	ShadowMapTexture(GLuint shadowWidth, GLuint shadowHeight);

	~ShadowMapTexture();

	GLuint getHandle();

	GLuint getDepthFBO();

	void activate();
	void resetViewPort();

};
