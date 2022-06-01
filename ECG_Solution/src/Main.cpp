/*
* Copyright 2021 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/


#include "Utils.h"
#include <sstream>
#include "Camera.h"
#include "Shader.h"
#include "Geometry.h"
#include "Material.h"
#include "Light.h"
#include "Texture.h"
#include "UserInterface.h"
#include "ModelLoader.h"
#include "bullet/BulletWorld.h"
#include "bullet/BulletBody.h"

#include <stb_image.h>
#include <iostream>

/* --------------------------------------------- */
// Prototypes
/* --------------------------------------------- */

static void APIENTRY DebugCallbackDefault(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam);
static std::string FormatDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void setPerFrameUniforms(Shader* shader, Camera& camera, std::vector<DirectionalLight> dirLights, std::vector<PointLight> pointLights);
void setPerFrameUniforms(Shader* shader, Camera& camera, std::vector<DirectionalLight> dirLights, std::vector<PointLight> pointLights, glm::mat4 lightSpaceMatrix, glm::vec3 lightPos);
glm::mat4 lookAtView(glm::vec3 eye, glm::vec3 at, glm::vec3 up);

void renderQuad();
unsigned int loadTexture(char const* path);

/* --------------------------------------------- */
// Global variables
/* --------------------------------------------- */

static bool _wireframe = false;
static bool _culling = true;
static bool _dragging = false;
static bool _strafing = false;
static float _zoom = 6.0f;

int _timer = 300;
boolean _gameLost = false;

int window_width, window_height, _refresh_rate;
GLFWmonitor* monitor;
bool fullscreen;
float _brightness;

std::vector<DirectionalLight> dirLights;
std::vector<PointLight> pointLights;




/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char** argv)
{
	/* --------------------------------------------- */
	// Load settings.ini
	/* --------------------------------------------- */

	INIReader reader("assets/settings.ini");

	window_width = reader.GetInteger("window", "width", 800);
	window_height = reader.GetInteger("window", "height", 800);
	_refresh_rate = reader.GetInteger("window", "refresh_rate", 60);
	_brightness = reader.GetInteger("window", "brightness", 1);
	fullscreen = reader.GetBoolean("window", "fullscreen", false);
	std::string window_title = reader.Get("window", "title", "Lost in Abyss");
	float fov = float(reader.GetReal("camera", "fov", 60.0f));
	float nearZ = float(reader.GetReal("camera", "near", 0.1f));
	float farZ = float(reader.GetReal("camera", "far", 100.0f));
	string _fontpath = "assets/fonts/Roboto-Regular.ttf";

	std::shared_ptr<UserInterface> _ui;

	/* --------------------------------------------- */
	// Create context
	/* --------------------------------------------- */

	glfwSetErrorCallback([](int error, const char* description) { std::cout << "GLFW error " << error << ": " << description << std::endl; });

	if (!glfwInit()) {
		EXIT_WITH_ERROR("Failed to init GLFW");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL version 4.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Request core profile
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);  // Create an OpenGL debug context 
	glfwWindowHint(GLFW_REFRESH_RATE, _refresh_rate); // Set refresh rate
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Enable antialiasing (4xMSAA)
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Open window
	monitor = nullptr;

	if (fullscreen)
		monitor = glfwGetPrimaryMonitor();

	GLFWwindow* window = glfwCreateWindow(window_width, window_height, window_title.c_str(), monitor, nullptr);

	if (!window) EXIT_WITH_ERROR("Failed to create window");


	// This function makes the context of the specified window current on the calling thread. 
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true;
	GLenum err = glewInit();

	// If GLEW wasn't initialized
	if (err != GLEW_OK) {
		EXIT_WITH_ERROR("Failed to init GLEW: " << glewGetErrorString(err));
	}

	// Debug callback
	if (glDebugMessageCallback != NULL) {
		// Register your callback function.

		glDebugMessageCallback(DebugCallbackDefault, NULL);
		// Enable synchronous callback. This ensures that your callback function is called
		// right after an error has occurred. This capability is not defined in the AMD
		// version.
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}


	/* --------------------------------------------- */
	// Init framework
	/* --------------------------------------------- */

	if (!initFramework()) {
		EXIT_WITH_ERROR("Failed to init framework");
	}

	// set callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// set GL defaults
	glClearColor(1, 1, 1, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	/* --------------------------------------------- */
	// Initialize scene and render loop
	/* --------------------------------------------- */
	{
		// Load shader(s)
		std::shared_ptr<Shader> textureShader = std::make_shared<Shader>("texture.vert", "texture.frag");
		// for shadow mapping
		std::shared_ptr<Shader> depthShader = std::make_shared<Shader>("depth.vert", "depth.frag");
		std::shared_ptr<Shader> quadShader = std::make_shared<Shader>("quad.vert", "quad.frag");

		// Initialize bullet world
		BulletWorld bulletWorld = BulletWorld(btVector3(0, -10, 0));


		unsigned int testTexture = loadTexture("assets/textures/wood.png");

		// Create textures
		std::shared_ptr<Texture> woodTexture = std::make_shared<Texture>("wood_texture.dds");
		std::shared_ptr<Texture> tileTexture = std::make_shared<Texture>("tiles_diffuse.dds");

		// Create materials
		std::shared_ptr<Material> woodTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.7f, 0.1f), 2.0f, woodTexture);
		std::shared_ptr<Material> tileTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.7f, 0.1f), 2.0f, tileTexture);
		std::shared_ptr<Material> depthMaterial = std::make_shared<TextureMaterial>(depthShader);

		std::shared_ptr<Material> catModelMaterial = std::make_shared<TextureMaterial>(textureShader);
		
		// Create geometry
		Geometry mainBox(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)), Geometry::createCubeGeometry(0.5f, 0.5f, 0.5f), woodTextureMaterial);
		Geometry testPlatform(glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f)), Geometry::createCubeGeometry(5.0f, 1.0f, 5.0f), woodTextureMaterial);
		Geometry testBox(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, -4.0f, 6.0f)), Geometry::createCubeGeometry(5.5f, 2.5f, 0.5f), depthMaterial);
		Geometry testBox2(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, -5.0f, 6.0f)), Geometry::createCubeGeometry(2.5f, 2.5f, 0.5f), depthMaterial);

		glm::mat4 catModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));
		ModelLoader cat("assets/objects/cat/cat.obj", catModel, catModelMaterial);
		BulletBody btCat(btTag, Geometry::createCubeGeometry(0.4f, 0.5f, 0.2f), 1.0f, true, glm::vec3(0.0f, 10.0f, 0.0f), bulletWorld._world);

		glm::mat4 sceneModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		ModelLoader scene("assets/objects/scene.obj", sceneModel, catModelMaterial);
		
		for (const auto& mesh : scene.getMeshes()) {
			// store bullet body in a list or another data structure
			BulletBody btScene(btTag, mesh._aiMesh, mesh._transformationMatrix, 0.0f, false, bulletWorld._world);
		}
		
		// Initialize camera
		Camera camera(fov, float(window_width) / float(window_height), nearZ, farZ);

		// Initialize user interface/HUD
		_ui = std::make_shared<UserInterface>("userinterface.vert", "userinterface.frag", window_width, window_height, _brightness, _fontpath);

		// Initialize lights and put them into vector
		// NOTE: to set up number of lights "#define NR_DIR_LIGHTS" and "#define NR_POINT_LIGHTS" in "texture.frag" has to be updated!
		#pragma region directional lights

		// lighting info
		glm::vec3 lightPos(0.0f, -1.0f, -1.0f);

		DirectionalLight dirL1(glm::vec3(1.0f, 1.0f, 1.0f), lightPos);
		DirectionalLight dirL2(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.5f));
		DirectionalLight dirL3(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
		dirLights.push_back(dirL1);
		dirLights.push_back(dirL2);
		dirLights.push_back(dirL3);
	#pragma endregion

		#pragma region point lights
		PointLight pointL1(glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(1.0f, 2.5f, 10.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		PointLight pointL2(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(6.5f, 1.5f, 4.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		PointLight pointL3(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(-3.5f, 0.5f, 4.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL1);
		pointLights.push_back(pointL2);
		pointLights.push_back(pointL3);
	#pragma endregion

		// Render loop
		float lastT = float(glfwGetTime());
		float dt = 0.0f;
		float t_sum = 0.0f;
		double mouse_x, mouse_y;
		int fpsCounter = 0;
		double lastTime = glfwGetTime();
		int fps = 0;


		// shadowmapping 
		// create a framebuffer object for rendering the depth map
		GLuint depthMapFBO;
		glGenFramebuffers(1, &depthMapFBO);

		// create 2d texture, framebuffer's depth buffer: 
		const unsigned int SHADOW_WIDTH = window_width, SHADOW_HEIGHT = window_height;
		GLuint depthMap = 0;
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// attach depth texture as FBO's depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE); // no colour
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		
		// shader configuration
		quadShader -> use();
		quadShader -> setUniform("depthMap", 0);

		while (!glfwWindowShouldClose(window)) {
			// Clear backbuffer
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Poll events
			glfwPollEvents();

			// Update camera
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			camera.update(int(mouse_x), int(mouse_y), _zoom, _dragging, _strafing);

			// 1. render depth of scene to texture (from light's perspective)
			glm::mat4 lightProjection, lightView;
			glm::mat4 lightSpaceMatrix;
			float near_plane = 1.0f, far_plane = 7.5f;
			// note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
			//lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane);
			lightProjection = glm::ortho(-10.0f, 1.0f, -10.0f, 1.0f, near_plane, far_plane);

			lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
			// lightView = lookAtView(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
			lightSpaceMatrix = lightProjection * lightView;

			depthShader->use();
			depthShader->setUniform("lightSpaceMatrix", lightSpaceMatrix);

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0); 
			glBindTexture(GL_TEXTURE_2D, testTexture);
			testBox.drawDepth();
			testBox2.drawDepth();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// reset viewport
			glViewport(0, 0, window_width, window_height);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			setPerFrameUniforms(textureShader.get(), camera, dirLights, pointLights, lightSpaceMatrix, lightPos);
			// setPerFrameUniforms(textureShader.get(), camera, dirLights, pointLights);

			// Render
			mainBox.draw();
			//testBox.draw();
			testPlatform.draw();
			cat.Draw();
			cat.SetModelMatrix(glm::translate(glm::mat4(1.0f), btCat.getPosition()));
			scene.Draw();
			

			double t = glfwGetTime();
			double dt = t - lastT;
			if ((int)floor(lastT) != (int)floor(t)) {
				fps = fpsCounter;
				fpsCounter = 0;
			}
			fpsCounter++;

			lastT = t;

			// draw user interface
			//_ui->updateUI(fps, false, glm::vec3(0, 0, 0));
			_ui->updateUI(fps, false, glm::vec3(0, 0, 0));

			// bullet
			bulletWorld.stepSimulation(
				dt, // btScalar timeStep: seconds, not milliseconds, passed since the last call 
				1, // maxSubSteps: should generally stay at one so Bullet interpolates current values on its own
				btScalar(1.) / btScalar(60.)); // fixedTimeStep: inversely proportional to the simulation's resolution

			// render depth map to quad for visual debugging
			quadShader->use();
			quadShader->setUniform("near_plane", near_plane);
			quadShader->setUniform("far_plane", far_plane);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			renderQuad();

			// Swap buffers
			glfwSwapBuffers(window);
		}
	}


	/* --------------------------------------------- */
	// Destroy framework
	/* --------------------------------------------- */

	destroyFramework();


	/* --------------------------------------------- */
	// Destroy context and exit
	/* --------------------------------------------- */

	glfwTerminate();

	return EXIT_SUCCESS;
}



void setPerFrameUniforms(Shader* shader, Camera& camera, std::vector<DirectionalLight> dirLights, std::vector<PointLight> pointLights)
{
	shader->use();
	shader->setUniform("viewProjMatrix", camera.getViewProjectionMatrix());
	shader->setUniform("camera_world", camera.getPosition());
	shader->setUniform("brightness", _brightness);

	for (int i = 0; i < dirLights.size(); i++) {
		DirectionalLight& dirL = dirLights[i];
		shader->setUniform("dirLights[" + std::to_string(i) + "].color", dirL.color);
		shader->setUniform("dirLights[" + std::to_string(i) + "].direction", dirL.direction);
	}
	for (int i = 0; i < pointLights.size(); i++) {
		PointLight& pointL = pointLights[i];
		shader->setUniform("pointLights[" + std::to_string(i) + "].color", pointL.color);
		shader->setUniform("pointLights[" + std::to_string(i) + "].position", pointL.position);
		shader->setUniform("pointLights[" + std::to_string(i) + "].attenuation", pointL.attenuation);
	}
}

void setPerFrameUniforms(Shader* shader, Camera& camera, std::vector<DirectionalLight> dirLights, std::vector<PointLight> pointLights, glm::mat4 lightSpaceMatrix, glm::vec3 lightPos)
{
	shader->use();
	shader->setUniform("viewProjMatrix", camera.getViewProjectionMatrix());
	shader->setUniform("camera_world", camera.getPosition());
	shader->setUniform("brightness", _brightness);
	shader->setUniform("lightSpaceMatrix", lightSpaceMatrix);
	shader->setUniform("lightPos", lightPos);

	for (int i = 0; i < dirLights.size(); i++) {
		DirectionalLight& dirL = dirLights[i];
		shader->setUniform("dirLights[" + std::to_string(i) + "].color", dirL.color);
		shader->setUniform("dirLights[" + std::to_string(i) + "].direction", dirL.direction);
	}
	for (int i = 0; i < pointLights.size(); i++) {
		PointLight& pointL = pointLights[i];
		shader->setUniform("pointLights[" + std::to_string(i) + "].color", pointL.color);
		shader->setUniform("pointLights[" + std::to_string(i) + "].position", pointL.position);
		shader->setUniform("pointLights[" + std::to_string(i) + "].attenuation", pointL.attenuation);
	}
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		_dragging = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		_dragging = false;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		_strafing = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		_strafing = false;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	_zoom -= float(yoffset) * 0.5f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// F1 - Wireframe
	// F2 - Culling
	// F11 - Fullscreen toggle
	// Esc - Exit

	if (action != GLFW_RELEASE) return;

	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, true);
		break;
	case GLFW_KEY_F1:
		_wireframe = !_wireframe;
		glPolygonMode(GL_FRONT_AND_BACK, _wireframe ? GL_LINE : GL_FILL);
		break;
	case GLFW_KEY_F2:
		_culling = !_culling;
		if (_culling) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);
		break;
	case GLFW_KEY_F11:

		// glfwGetWindowMonitor(window) returns NULL if windowed
		if (glfwGetWindowMonitor(window) == nullptr) {
			glfwSetWindowMonitor(window, monitor, 0, 0, window_width, window_height, _refresh_rate);
		}
		else {
			glfwSetWindowMonitor(window, nullptr, 0, 0, window_width, window_height, _refresh_rate);
		}

		break;
	}
}

static void APIENTRY DebugCallbackDefault(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) {
	if (id == 131185 || id == 131218) return; // ignore performance warnings from nvidia
	std::string error = FormatDebugOutput(source, type, id, severity, message);
	std::cout << error << std::endl;
}

static std::string FormatDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg) {
	std::stringstream stringStream;
	std::string sourceString;
	std::string typeString;
	std::string severityString;

	// The AMD variant of this extension provides a less detailed classification of the error,
	// which is why some arguments might be "Unknown".
	switch (source) {
	case GL_DEBUG_CATEGORY_API_ERROR_AMD:
	case GL_DEBUG_SOURCE_API: {
		sourceString = "API";
		break;
	}
	case GL_DEBUG_CATEGORY_APPLICATION_AMD:
	case GL_DEBUG_SOURCE_APPLICATION: {
		sourceString = "Application";
		break;
	}
	case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
		sourceString = "Window System";
		break;
	}
	case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
	case GL_DEBUG_SOURCE_SHADER_COMPILER: {
		sourceString = "Shader Compiler";
		break;
	}
	case GL_DEBUG_SOURCE_THIRD_PARTY: {
		sourceString = "Third Party";
		break;
	}
	case GL_DEBUG_CATEGORY_OTHER_AMD:
	case GL_DEBUG_SOURCE_OTHER: {
		sourceString = "Other";
		break;
	}
	default: {
		sourceString = "Unknown";
		break;
	}
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR: {
		typeString = "Error";
		break;
	}
	case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
		typeString = "Deprecated Behavior";
		break;
	}
	case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
		typeString = "Undefined Behavior";
		break;
	}
	case GL_DEBUG_TYPE_PORTABILITY_ARB: {
		typeString = "Portability";
		break;
	}
	case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
	case GL_DEBUG_TYPE_PERFORMANCE: {
		typeString = "Performance";
		break;
	}
	case GL_DEBUG_CATEGORY_OTHER_AMD:
	case GL_DEBUG_TYPE_OTHER: {
		typeString = "Other";
		break;
	}
	default: {
		typeString = "Unknown";
		break;
	}
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: {
		severityString = "High";
		break;
	}
	case GL_DEBUG_SEVERITY_MEDIUM: {
		severityString = "Medium";
		break;
	}
	case GL_DEBUG_SEVERITY_LOW: {
		severityString = "Low";
		break;
	}
	default: {
		severityString = "Unknown";
		break;
	}
	}

	stringStream << "OpenGL Error: " << msg;
	stringStream << " [Source = " << sourceString;
	stringStream << ", Type = " << typeString;
	stringStream << ", Severity = " << severityString;
	stringStream << ", ID = " << id << "]";

	return stringStream.str();
}

glm::mat4 lookAtView(glm::vec3 eye, glm::vec3 at, glm::vec3 up)
{
	glm::vec3 zaxis = normalize(at - eye);
	glm::vec3 xaxis = normalize(cross(zaxis, up));
	glm::vec3 yaxis = cross(xaxis, zaxis);

	zaxis = -zaxis;

	glm::mat4 viewMatrix = {
	  glm::vec4(xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, eye)),
	  glm::vec4(yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, eye)),
	  glm::vec4(zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, eye)),
	  glm::vec4(0, 0, 0, 1)
	};

	return viewMatrix;
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
