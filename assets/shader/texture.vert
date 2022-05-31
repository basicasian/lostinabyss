#version 430
/*
* Copyright 2019 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv; // texture

out VertexData {
	vec3 position_world;
	vec3 normal_world;
	vec2 uv;
	vec4 FragPosLightSpace;
} vert;


uniform mat4 modelMatrix;
uniform mat4 viewProjMatrix;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 position_world_ = modelMatrix * vec4(position, 1);
	vert.position_world = position_world_.xyz;

	vert.normal_world = normalMatrix * normal;
	vert.uv = uv;
	
	vert.FragPosLightSpace = lightSpaceMatrix * vec4(vert.position_world, 1.0);
	gl_Position = viewProjMatrix * position_world_;
}