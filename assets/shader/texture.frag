#version 430
/*
* Copyright 2019 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/

in VertexData {
	vec3 position_world;
	vec3 normal_world;
	vec2 uv;
	vec4 FragPosLightSpace;
} vert;

out vec4 color;

uniform float brightness;

uniform vec3 camera_world;

uniform vec3 lightPos;

uniform vec3 materialCoefficients; // x = ambient, y = diffuse, z = specular 
uniform float specularAlpha;
uniform sampler2D diffuseTexture;
uniform sampler2D shadowTexture;

uniform struct DirectionalLight {
	vec3 color;
	vec3 direction;
} ;

uniform struct PointLight {
	vec3 color;
	vec3 position;
	vec3 attenuation; // x = light.constant, y = light.linear, z = light.quadratic
} ;

#define NR_DIR_LIGHTS 1
uniform DirectionalLight dirLights[NR_DIR_LIGHTS];
#define NR_POINT_LIGHTS 3
uniform PointLight pointLights[NR_POINT_LIGHTS];


vec3 phong(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 diffuseC, float diffuseF, vec3 specularC, float specularF, float alpha, bool attenuate, vec3 attenuation, float shadow) {
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);

	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(0, dot(reflectDir, viewDir)), alpha);

	// combine results
	float d = length(lightDir); // distance
	lightDir = normalize(lightDir);

	float att = 1.0;	
	if (attenuate) {
		att = 1.0f / (attenuation.x + d * attenuation.y + d * d * attenuation.z);
	}
	
	// material colour * light colour * shading
	if (attenuate) {
		return (diffuseF * (diffuseC * diff) + specularF * (specularC * spec)) * att;
	} else {
		return (1-shadow) * (diffuseF * (diffuseC * diff) + specularF * (specularC * spec)) * att;
	}
}

/*
vec3 phong(vec3 n, vec3 l, vec3 v, vec3 diffuseC, float diffuseF, vec3 specularC, float specularF, float alpha, bool attenuate, vec3 attenuation) {
	float d = length(l);
	l = normalize(l);
	float att = 1.0;	
	if(attenuate) att = 1.0f / (attenuation.x + d * attenuation.y + d * d * attenuation.z);
	vec3 r = reflect(-l, n);
	return (diffuseF * diffuseC * max(0, dot(n, l)) + specularF * specularC * pow(max(0, dot(r, v)), alpha)) * att; 
}*/

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowTexture, projCoords.xy).r; 

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

	// check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

	/*
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(vert.normal_world);
    vec3 lightDir = normalize(lightPos - vert.position_world);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;*/
        
    return shadow;  
}

void main() {	
	
	vec3 normal = normalize(vert.normal_world);
	vec3 viewDir = normalize(camera_world - vert.position_world);
	
	vec3 texColor = texture(diffuseTexture, vert.uv).rgb;
	color = vec4(texColor * materialCoefficients.x, 1); // ambient
		
	// calculate shadow
	float shadow = ShadowCalculation(vert.FragPosLightSpace);  

	// phase 1: Directional lighting
	// add directional light contribution
	for(int i = 0; i < NR_DIR_LIGHTS; i++) {
	color.rgb += brightness * phong(normal, -dirLights[i].direction, viewDir, dirLights[i].color * texColor, materialCoefficients.y, dirLights[i].color, materialCoefficients.z, specularAlpha, false, vec3(0), shadow);
	}
	// phase 2: Point lights
	// add point light contribution
	for(int i = 0; i < NR_POINT_LIGHTS; i++){
	//color.rgb += brightness * phong(normal, pointLights[i].position - vert.position_world, viewDir, pointLights[i].color * texColor, materialCoefficients.y, pointLights[i].color, materialCoefficients.z, specularAlpha, true, pointLights[i].attenuation, shadow);
	}
      


}

