/*
* Copyright 2020 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/
#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "Shader.h"
#include "textures/Texture.h"



/*!
 * Base material
 */
class Material
{
protected:
	/*!
	 * The shader used for rendering this material
	 */
	std::shared_ptr<Shader> _shader;
	/*!
	 * The material's coefficients (x = ambient, y = diffuse, z = specular)
	 */
	glm::vec3 _materialCoefficients; 

	/*!
	 * Alpha value, i.e. the shininess constant
	 */
	float _alpha;

public:
	/*!
	 * Base material constructor
	 * @param shader: The shader used for rendering this material
	 * @param materialCoefficients: The material's coefficients (x = ambient, y = diffuse, z = specular)
	 * @param alpha: Alpha value, i.e. the shininess constant
	 */
	Material(std::shared_ptr<Shader> shader, glm::vec3 materialCoefficients, float alpha);

	Material::Material(std::shared_ptr<Shader> shader);

	virtual ~Material();

	/*!
	 * @return The shader associated with this material
	 */
	Shader* getShader();

	/*!
	 * Sets this material's parameters as uniforms in the shader
	 */
	virtual void setUniforms();

	virtual void bindTexture(GLuint depthMap);

	void setShader(std::shared_ptr<Shader> shader);
};


/*!
 * Texture material
 */
class TextureMaterial : public Material
{
protected:
	/*!
	 * The diffuse texture of this material
	 */
	std::shared_ptr<Texture> _diffuseTexture;

	/*!
	 * The shadow texture of this material for shadowmapping
	 */
	std::shared_ptr<Texture> _shadowTexture;

public:
	/*!
	 * Texture material constructor
	 * @param shader: The shader used for rendering this material
	 * @param materialCoefficients: The material's coefficients (x = ambient, y = diffuse, z = specular)
	 * @param alpha: Alpha value, i.e. the shininess constant
	 * @param diffuseTexture: The diffuse texture of this material
	 */
	TextureMaterial(std::shared_ptr<Shader> shader, glm::vec3 materialCoefficients, float alpha, std::shared_ptr<Texture> diffuseTexture);
	
	TextureMaterial(std::shared_ptr<Shader> shader);

	virtual ~TextureMaterial();

	/*!
	 * Set's this material's parameters as uniforms in the shader
	 */
	virtual void setUniforms();

	virtual void setNormalUniforms();
};
