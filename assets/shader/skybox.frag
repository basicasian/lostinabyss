#version 430 
out vec4 FragColor;

in vec3 FragTexture;

uniform samplerCube diffuseTexture;

void main()
{
    FragColor = texture(diffuseTexture, FragTexture);
}

