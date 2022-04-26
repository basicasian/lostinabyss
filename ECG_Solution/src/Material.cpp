/*
* Copyright 2020 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/
#include "Material.h"

/* --------------------------------------------- */
// Base material
/* --------------------------------------------- */

Material::Material(std::shared_ptr<Shader> shader, glm::vec3 materialCoefficients, float alpha)
	: _shader(shader), _materialCoefficients(materialCoefficients), _alpha(alpha)
{
}

Material::Material(std::shared_ptr<Shader> shader)
	: _shader(shader)
{
}

Material::~Material()
{
}


void Material::setShader(std::shared_ptr<Shader> shader) {
	_shader = shader;
}

Shader* Material::getShader()
{
	return _shader.get();
}

void Material::setUniforms()
{
	_shader->setUniform("materialCoefficients", _materialCoefficients);
	_shader->setUniform("specularAlpha", _alpha);
}

/* --------------------------------------------- */
// Texture material
/* --------------------------------------------- */

TextureMaterial::TextureMaterial(std::shared_ptr<Shader> shader, glm::vec3 materialCoefficients, float alpha, std::shared_ptr<Texture> diffuseTexture)
	: Material(shader, materialCoefficients, alpha), _diffuseTexture(diffuseTexture)
{
}

TextureMaterial::TextureMaterial(std::shared_ptr<Shader> shader)
	: Material(shader)
{
}

TextureMaterial::~TextureMaterial()
{
}

void TextureMaterial::setUniforms()
{
	Material::setUniforms();

	_diffuseTexture->bind(0);
	_shader->setUniform("diffuseTexture", 0);
}
