#version 430
/*
* Copyright 2019 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VertexData {
	vec3 position_world;
	vec3 normal_world;
	vec2 uv;
	vec4 FragPosLightSpace;
} vert;

uniform vec3 lightColor;

void main() {	

	FragColor = vec4(lightColor, 1.0);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if ( brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

}

