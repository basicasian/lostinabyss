#pragma once

#include "Shader.h"
#include "Utils.h"

class QuadGeometry
{
protected:
	unsigned int quadVAO = 0;
	unsigned int quadVBO;

public:

	QuadGeometry();
	~QuadGeometry();

	void renderQuad();
};
