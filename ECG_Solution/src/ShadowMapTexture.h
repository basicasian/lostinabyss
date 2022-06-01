#pragma once

#include "Texture.h"

class ShadowmapTexture : public Texture
{
private:
	GLuint _size;
	GLuint _frameBuffer;

protected:
	GLenum getTarget();

public:

	/*!
	 * Creates a shadow map texture referencing the frame buffer
	 */
	ShadowmapTexture(GLuint size, GLuint frameBuffer);

	/*!
	 * Bind texture to frame buffer
	 */
	void load();
};
