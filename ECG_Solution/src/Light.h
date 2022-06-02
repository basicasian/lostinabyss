/*
* Copyright 2020 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/
#pragma once


#include <glm\glm.hpp>

/*!
 * Directional light, a light that gets emitted in a specific direction
 */
struct DirectionalLight {
	/*!
	 * Default constructor
	 */
	DirectionalLight() {
		_enabled = false;
	}

	/*!
	 * Directional light constructor
	 * A light that gets emitted in a specific direction.
	 * @param color: color of the light
	 * @param direction: direction of the light
	 * @param enabled: if the light is enabled
	 */
	DirectionalLight(glm::vec3 color, glm::vec3 direction, bool enabled = true)
		: _color(color), _direction(glm::normalize(direction)), _enabled(enabled)
	{
		// 1. render depth of scene to texture (from light's perspective)
		glm::vec3 lightPos = glm::vec3(0.f, 50.f, 0.f);
		glm::mat4 lightProjection, lightView;
		float near_plane = -10.0f, far_plane = 70.0f;
		lightProjection = glm::ortho(-20.0f, 40.0f, -40.0f, 20.0f, near_plane, far_plane);

		lightView = glm::lookAt(lightPos, lightPos + _direction, glm::vec3(0.0, 0.0, 1.0));
		_lightSpaceMatrix = lightProjection * lightView;
	}

	/*!
	 * If the light is enabled
	 */
	bool _enabled;

	/*!
	 * Color of the light
	 */
	glm::vec3 _color;

	/*!
	 * Direction of the light
	 */
	glm::vec3 _direction;

	/*!
	 * Light space matrix for shadow calculation
	 */
	glm::mat4 _lightSpaceMatrix;
};

/*!
 * Point light, a light that gets emitted from a single point in all directions
 */
struct PointLight {
	/*!
	 * Default constructor
	 */
	PointLight() {
		_enabled = false;
	}

	/*!
	 * Point light constructor
	 * A light that gets emitted from a single point in all directions
	 * @param color: color of the light
	 * @param position: position of the light
	 * @param attenuation: the light's attenuation (x = constant, y = linear, z = quadratic)
	 * @param enabled: if the light is enabled
	 */
	PointLight(glm::vec3 color, glm::vec3 position, glm::vec3 attenuation, bool enabled = true)
		: _color(color), _position(position), _attenuation(attenuation), _enabled(enabled)
	{}

	/*!
	 * If the light is enabled
	 */
	bool _enabled;

	/*!
	 * Color of the light
	 */
	glm::vec3 _color;

	/*!
	 * Position of the light
	 */
	glm::vec3 _position;

	/*!
	 * The light's attenuation (x = constant, y = linear, z = quadratic)
	 */
	glm::vec3 _attenuation;
};
