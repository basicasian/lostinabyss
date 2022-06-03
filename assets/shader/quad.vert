#version 430

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
    // if debugging depth mapping
    // gl_Position = vec4(position, 1.0); 

    // if quad
    gl_Position = vec4(position.x, position.y, 0.0, 1.0); 

    TexCoords = texCoords;
}  
