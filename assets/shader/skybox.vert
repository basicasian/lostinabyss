#version 430 core
layout (location = 0) in vec3 position;

out vec3 FragTexture;

uniform mat4 viewProjMatrix;
uniform mat4 modelMatrix;

void main()
{
    FragTexture = position;
	vec4 pos = modelMatrix * viewProjMatrix * vec4(position, 1.0f);
   	gl_Position = pos.xyww;
}