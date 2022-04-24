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


/* --------------------------------------------- */
// Prototypes
/* --------------------------------------------- */

static void APIENTRY DebugCallbackDefault(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam);
static std::string FormatDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void setPerFrameUniforms(Shader* shader, Camera& camera, DirectionalLight& dirL, PointLight& pointL);


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
	float _brightness = reader.GetInteger("window", "brightness", 1);
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
		std::shared_ptr<Shader> skyboxShader = std::make_shared<Shader>("skybox.vert", "skybox.frag");

		// Create textures
		std::shared_ptr<Texture> woodTexture = std::make_shared<Texture>("wood_texture.dds");
		std::shared_ptr<Texture> tileTexture = std::make_shared<Texture>("tiles_diffuse.dds");
		std::vector<std::string> files
		{
			"assets/textures/skybox/back.jpg",
			"assets/textures/skybox/bottom.jpg",
			"assets/textures/skybox/front.jpg",
			"assets/textures/skybox/left.jpg",
			"assets/textures/skybox/right.jpg",
			"assets/textures/skybox/top.jpg",
		};


		// Create materials
		std::shared_ptr<Material> woodTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.7f, 0.1f), 2.0f, woodTexture);
		std::shared_ptr<Material> tileTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.7f, 0.3f), 8.0f, tileTexture);

		// Create geometry
		Geometry cube = Geometry(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 0.0f)), Geometry::createCubeGeometry(1.5f, 1.5f, 1.5f), woodTextureMaterial);
		Geometry cylinder = Geometry(glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, -1.0f, 0.0f)), Geometry::createCylinderGeometry(32, 1.3f, 1.0f), tileTextureMaterial);
		Geometry sphere = Geometry(glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, -1.0f, 0.0f)), Geometry::createSphereGeometry(64, 32, 1.0f), tileTextureMaterial);


		// Initialize camera
		Camera camera(fov, float(window_width) / float(window_height), nearZ, farZ);

		// Initialize lights
		DirectionalLight dirL(glm::vec3(0.8f), glm::vec3(0.0f, -1.0f, -1.0f));
		PointLight pointL(glm::vec3(1.0f), glm::vec3(0.0f), glm::vec3(1.0f, 0.4f, 0.1f));

		// Initialize user interface/HUD
		_ui = std::make_shared<UserInterface>("userinterface.vert", "userinterface.frag", window_width, window_height, _brightness, _fontpath);

		// Render loop
		float lastT = float(glfwGetTime());
		float dt = 0.0f;
		float t_sum = 0.0f;
		double mouse_x, mouse_y;
		int fpsCounter = 0;
		double lastTime = glfwGetTime();
		int fps = 0;

		while (!glfwWindowShouldClose(window)) {
			// Clear backbuffer
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Poll events
			glfwPollEvents();

			// Update camera
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			camera.update(int(mouse_x), int(mouse_y), _zoom, _dragging, _strafing);

			// Set per-frame uniforms
			setPerFrameUniforms(textureShader.get(), camera, dirL, pointL);

			// Render
			cube.draw();
			cylinder.draw();
			sphere.draw();

			double t = glfwGetTime();
			double dt = t - lastT;
			if ((int)floor(lastT) != (int)floor(t)) {
				fps = fpsCounter;
				fpsCounter = 0;
			}
			fpsCounter++;

			lastT = t;

			// draw user interface
			_ui->updateUI(fps, false, glm::vec3(0, 0, 0));

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


void setPerFrameUniforms(Shader* shader, Camera& camera, DirectionalLight& dirL, PointLight& pointL)
{
	shader->use();
	shader->setUniform("viewProjMatrix", camera.getViewProjectionMatrix());
	shader->setUniform("camera_world", camera.getPosition());

	shader->setUniform("dirL.color", dirL.color);
	shader->setUniform("dirL.direction", dirL.direction);
	shader->setUniform("pointL.color", pointL.color);
	shader->setUniform("pointL.position", pointL.position);
	shader->setUniform("pointL.attenuation", pointL.attenuation);
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		_dragging = true;
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		_dragging = false;
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		_strafing = true;
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
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