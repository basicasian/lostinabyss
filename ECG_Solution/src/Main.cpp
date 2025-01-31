/*
* Copyright 2021 Vienna University of Technology.
* Institute of Computer Graphics and Algorithms.
* This file is part of the ECG Lab Framework and must not be redistributed.
*/


#include "Utils.h"
#include <sstream>
#include "Camera.h"
#include "CameraPlayer.h"
#include "Shader.h"
#include "Geometry.h"
#include "Material.h"
#include "Light.h"
#include "textures/Texture.h"
#include "textures/ShadowMapTexture.h"
#include "UserInterface.h"
#include "ModelLoader.h"
#include "bullet/BulletWorld.h"
#include "bullet/BulletBody.h"
#include "PostProcessing.h"
#include "QuadGeometry.h"

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
void poll_keys(GLFWwindow* window, double dt);

void setPerFrameUniformsTexture(Shader* shader,std::vector<DirectionalLight> dirLights, std::vector<std::shared_ptr<PointLight>> pointLights);
void setPerFrameUniformsDepth(Shader* depthShader, std::vector<DirectionalLight> dirLights);
void setPerFrameUniformsLight(Shader* shader, std::shared_ptr<PointLight> pointL);

glm::mat4 lookAtView(glm::vec3 eye, glm::vec3 at, glm::vec3 up);

/* --------------------------------------------- */
// Global variables
/* --------------------------------------------- */

static bool _hud = true;
static bool _wireframe = false;
static bool _culling = true;
static bool _dragging = false;
static bool _strafing = false;
static bool _normalToggle = true;
static bool _lightsOn = true;
static float _zoom = 6.0f;
static CameraPlayer _player(glm::vec3(0.0f, 5.0f, 0.0f));


double _start;
int _timer = 100;
bool _gameLost = false;
bool _gameWon = false;

int window_width, window_height, _refresh_rate;
GLFWmonitor* monitor;
bool fullscreen;
float _brightness;
float exposure = 1.0f;

std::vector<DirectionalLight> dirLights;
std::vector<std::shared_ptr<PointLight>> pointLights;

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
	float farZ = float(reader.GetReal("camera", "far", 1000.0f));
	string _fontpath = "assets/fonts/Roboto-Regular.ttf";
	BulletBody winPlatform;
	BulletBody movingPlatform;

	_player.setProjectionMatrix(fov, farZ, nearZ, (float)window_width / (float)window_height);
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
	monitor = NULL;

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

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

		// for bloom
		std::shared_ptr<Shader> quadShader = std::make_shared<Shader>("quad.vert", "quad.frag"); // for debugging shadow + rendering end bloom result

		std::shared_ptr<Shader> blurShader = std::make_shared<Shader>("quad.vert", "blur.frag");
		std::shared_ptr<Shader> bloomResultShader = std::make_shared<Shader>("quad.vert", "bloomresult.frag");
				
		std::shared_ptr<Shader> lightShader = std::make_shared<Shader>("texture.vert", "lightbox.frag");

		// Initialize bullet world
		BulletWorld bulletWorld = BulletWorld(btVector3(0, -10, 0));
		_player.addToWorld(bulletWorld);

		// Create textures
		std::shared_ptr<ShadowMapTexture> shadowMapTexture = std::make_shared<ShadowMapTexture>(window_width, window_height);

		std::shared_ptr<Texture> justDoItTexture = std::make_shared<Texture>("assets/textures/videotextures/justdoit/frame_191.jpg", shadowMapTexture->getHandle(), "video");
		std::shared_ptr<Texture> goodGameTexture = std::make_shared<Texture>("assets/textures/videotextures/goodgame/frame_47.jpg", shadowMapTexture->getHandle(), "video");
		std::shared_ptr<Texture> imageTexture = std::make_shared<Texture>("assets/textures/smiley.png", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> furTexture = std::make_shared<Texture>("assets/textures/fur.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> furNormalTexture = std::make_shared<Texture>("assets/textures/fur_normal.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> abstractTexture = std::make_shared<Texture>("assets/textures/abstract.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> abstractNormalTexture = std::make_shared<Texture>("assets/textures/abstract_normal.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> brickTexture = std::make_shared<Texture>("assets/textures/brick.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> brickNormalTexture = std::make_shared<Texture>("assets/textures/brick_normal.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> woodTexture = std::make_shared<Texture>("assets/textures/wood.jpg", shadowMapTexture->getHandle(), "image");
		std::shared_ptr<Texture> woodNormalTexture = std::make_shared<Texture>("assets/textures/wood_normal.jpg", shadowMapTexture->getHandle(), "image");

		// set normal map 
		brickTexture->setNormalMap(brickNormalTexture->getHandle());
		woodTexture->setNormalMap(woodNormalTexture->getHandle());
		furTexture->setNormalMap(furNormalTexture->getHandle());
		abstractTexture->setNormalMap(abstractNormalTexture->getHandle());

		// Create materials
		std::shared_ptr<Material> woodTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, woodTexture);
		std::shared_ptr<Material> brickTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, brickTexture);
		std::shared_ptr<Material> furTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, furTexture);
		std::shared_ptr<Material> abstractTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, abstractTexture);
		std::shared_ptr<Material> imageTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, imageTexture);
		std::shared_ptr<Material> goodGameTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, goodGameTexture);
		std::shared_ptr<Material> justDoItTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, justDoItTexture);

		std::shared_ptr<Material> sceneMaterial = std::make_shared<TextureMaterial>(textureShader);
		std::shared_ptr<Material> depthMaterial = std::make_shared<TextureMaterial>(depthShader);
		std::shared_ptr<Material> lightMaterial = std::make_shared<TextureMaterial>(lightShader);

		// Create geometry
		Geometry goodGameScreen(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 41.0f, 27.0f)), Geometry::createCubeGeometry(0.01f, 5.0f, 5.0f), goodGameTextureMaterial);
		Geometry goodGameWall(glm::translate(glm::mat4(1.0f), glm::vec3(-40.25f, 41.0f, 27.0f)), Geometry::createCubeGeometry(0.5f, 5.0f, 5.0f), woodTextureMaterial);
		Geometry justDoItScreen(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, -4.0f)), Geometry::createCubeGeometry(5.0f, 3.0f, 0.01f), justDoItTextureMaterial);
		Geometry justDoItWall(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, -4.25f)), Geometry::createCubeGeometry(5.0f, 3.0f, 0.5f), woodTextureMaterial);
		std::shared_ptr<BulletBody> btWall = std::make_shared<BulletBody>(btObject, Geometry::createCubeGeometry(5.0f, 3.0f, 0.5f), 0.0f, true, glm::vec3(0.0f, 2.5f, -4.25f), bulletWorld._world);

		Geometry box1(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 3.0f, 5.0f)), Geometry::createCubeGeometry(1.0f, 1.0f, 1.0f), abstractTextureMaterial);
		BulletBody btBox1(btObject, Geometry::createCubeGeometry(1.0f, 1.0f, 1.0f), 1.0f, true, glm::vec3(1.0f, 3.0f, 5.0f), bulletWorld._world);
		Geometry box2(glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 5.0f)), Geometry::createCubeGeometry(1.0f, 1.0f, 1.0f), furTextureMaterial);
		BulletBody btBox2(btObject, Geometry::createCubeGeometry(1.0f, 1.0f, 1.0f), 1.0f, true, glm::vec3(3.0f, 3.0f, 5.0f), bulletWorld._world);
		Geometry box3(glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 5.0f)), Geometry::createCubeGeometry(1.0f, 1.0f, 1.0f), brickTextureMaterial);
		BulletBody btBox3(btObject, Geometry::createCubeGeometry(1.0f, 1.0f, 1.0f), 1.0f, true, glm::vec3(3.0f, 3.0f, 5.0f), bulletWorld._world);

		glm::mat4 sceneModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		ModelLoader scene("assets/objects/scene.obj", sceneModel, sceneMaterial);
		
		for (const auto& mesh : scene.getMeshes()) {
			string name(mesh._aiMesh->mName.C_Str());
			
			if (!(name.compare("hull"))) {
				BulletBody btScene(btObject, mesh._aiMesh, mesh._transformationMatrix, 0.0f, false, bulletWorld._world);
			}
			else if (!(name.compare("win"))) {
				std::cout << "winplatform found" << std::endl;
				winPlatform = BulletBody(btWin, mesh._aiMesh, mesh._transformationMatrix, 0.0f, true, bulletWorld._world);
			}
			else if (!(name.compare("move"))) {
				movingPlatform = BulletBody(btWin, mesh._aiMesh, mesh._transformationMatrix, 0.0f, true, bulletWorld._world);
			}
			else if (name.find("Cube") != string::npos) {
				BulletBody btScene(btPlatform, mesh._aiMesh, mesh._transformationMatrix, 0.0f, true, bulletWorld._world);
			}
			
		}

		std::vector<std::shared_ptr<Geometry>> balls;
		std::vector< std::shared_ptr<BulletBody>> bulletBalls;

		for (int i = 0; i < 5; i++) {
			std::shared_ptr<Geometry> ball = std::make_shared<Geometry>(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 3.0f, 1.0f)), Geometry::createSphereGeometry(15.0f, 15.0f, 0.5f), imageTextureMaterial);
			std::shared_ptr<BulletBody> btBall = std::make_shared<BulletBody>(btObject, Geometry::createSphereGeometry(5.0f, 5.0f, 0.5f), 1.0f, true, glm::vec3(1.0f, 3.0f, 1.0f), bulletWorld._world);
			balls.push_back(ball);
			bulletBalls.push_back(btBall);
		}
		
		// Initialize help classes
		// bloom/ blur
		PostProcessing blurProcessor = PostProcessing(window_width, window_height);

		// shadowmap debugging
		QuadGeometry _quadGeometry = QuadGeometry();

		// user interface/HUD
		_ui = std::make_shared<UserInterface>("userinterface.vert", "userinterface.frag", window_width, window_height, _brightness, _fontpath);

		// Initialize lights and put them into vector
		// NOTE: to set up number of lights "#define NR_DIR_LIGHTS" and "#define NR_POINT_LIGHTS" in "texture.frag" has to be updated!
		#pragma region directional lights

		//white
		DirectionalLight dirL1(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f));
		dirLights.push_back(dirL1);
		// orangey
		DirectionalLight dirL2(glm::vec3(0.3f, 0.2f, 0.1f), glm::vec3(0.0f, -1.0f, -1.0f));
		dirLights.push_back(dirL2);
		// pinkish
		DirectionalLight dirL3(glm::vec3(0.3f, 0.2f, 0.3f), glm::vec3(0.0f, -1.0f, 1.0f));
		dirLights.push_back(dirL3);
		#pragma endregion

		#pragma region point lights
		// white
		std::shared_ptr<PointLight> pointL2 = std::make_shared<PointLight> (glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL2);
		// turqoise
		std::shared_ptr<PointLight> pointL1 = std::make_shared<PointLight>(glm::vec3(0.0f, 3.0f, 3.0f), glm::vec3(7.0f, 1.5f, 5.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL1);
		// yellow
		std::shared_ptr<PointLight> pointL3 = std::make_shared<PointLight>(glm::vec3(3.0f, 3.0f, 0.0f), glm::vec3(15.0f, 6.0f, 1.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL3);
		// green
		std::shared_ptr<PointLight> pointL4 = std::make_shared<PointLight>(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(-30.0f, 17.0f, 35.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL4);
		// pink
		std::shared_ptr<PointLight> pointL7 = std::make_shared<PointLight>(glm::vec3(4.0f, 0.0f, 4.0f), glm::vec3(20.0f, 8.0f, -35.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL7);
		// pink
		std::shared_ptr<PointLight> pointL8 = std::make_shared<PointLight>(glm::vec3(4.0f, 0.0f, 4.0f), glm::vec3(-25.0f, 6.5f, 20.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL8);
		// white 
		std::shared_ptr<PointLight> pointL5 = std::make_shared<PointLight>(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(-25.0f, 5.0f, -25.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL5);
		// white
		std::shared_ptr<PointLight> pointL6 = std::make_shared<PointLight>(glm::vec3(0.0f, 3.0f, 3.0f), glm::vec3(20.0f, 10.0f, 15.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL6);


		// light cubes
		std::vector< std::shared_ptr<Geometry>> lightCubes;

		for (int i = 0; i < pointLights.size(); i++) {
			std::shared_ptr<PointLight> pointL = pointLights[i];

			glm::mat4 trans = glm::mat4(1.0f);
			trans = glm::translate(trans, pointL->_position);
			trans = glm::rotate(trans, glm::radians(1.0f * i), glm::vec3(1.0, 1.0, 1.0));
			std::shared_ptr<Geometry> lightbox = std::make_shared<Geometry>(trans, Geometry::createCubeGeometry(1.0f * i + 0.5f, 1.0f * i + 0.5f, 1.0f * i + 0.5f), lightMaterial);

			BulletBody btLight(btObject, Geometry::createCubeGeometry(1.0f * i + 0.5f, 1.0f * i + 0.5f, 1.0f * i + 0.5f), 0.0f, true, pointL->_position, bulletWorld._world);
			lightCubes.push_back(lightbox);
		}

		#pragma endregion

		// Render loop
		float lastT = float(glfwGetTime());
		_start = lastT;
		float dt = 0.0f;
		float t_sum = 0.0f;
		double mouse_x, mouse_y;
		int fpsCounter = 0;
		double lastTime = glfwGetTime();
		int fps = 0;

		double last_mouse_x, last_mouse_y;
		glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);

		// shader configuration
		quadShader->use();
		quadShader->setUniform("screenTexture", 0);

		blurShader->use();
		blurShader->setUniform("image", 0);

		bloomResultShader->use();
		bloomResultShader->setUniform("scene", 0);
		bloomResultShader->setUniform("bloomBlur", 1);

		while (!glfwWindowShouldClose(window)) {
			// Clear backbuffer
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Poll events
			glfwPollEvents();

			// Update camera
			poll_keys(window, dt);
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			//camera.update(int(mouse_x), int(mouse_y), _zoom, _dragging, _strafing);
			_player.inputMouseMovement(mouse_x - last_mouse_x, last_mouse_y - mouse_y);
			last_mouse_x = mouse_x;
			last_mouse_y = mouse_y;

			// shadowmapping (render depth of scene to texture - is done in dirLight constructor)
			setPerFrameUniformsDepth(depthShader.get(), dirLights);
			shadowMapTexture->bind();

			box1.drawShader(depthShader.get());
			box2.drawShader(depthShader.get());
			box3.drawShader(depthShader.get());
			goodGameWall.drawShader(depthShader.get());
			goodGameScreen.drawShader(depthShader.get());
			justDoItScreen.drawShader(depthShader.get());
			justDoItWall.drawShader(depthShader.get());
			scene.DrawShader(depthShader.get());
			for (int i = 0; i < balls.size(); i++) {
				balls.at(i)->drawShader(depthShader.get());
			}

			shadowMapTexture->resetViewPort();

			// shadowmapping (render scene as normal using the generated depth/shadow map)
			// bloom (start initial framebuffer )
			blurProcessor.bindInitalFrameBuffer();

			setPerFrameUniformsTexture(textureShader.get(), dirLights, pointLights);

			// render
			for (int i = 0; i < balls.size(); i++) {
				balls.at(i)->draw();
				balls.at(i)->setModelMatrix(glm::translate(glm::mat4(1.0f), bulletBalls.at(i)->getPosition()));
			}

			// render all objects with normal maps here
			if (_normalToggle) {
				textureShader->use();
				textureShader->setUniform("ifNormal", true);
			}
			goodGameWall.draw();	
			justDoItWall.draw();
			box1.draw();
			box1.setModelMatrix(glm::translate(glm::mat4(1.0f), btBox1.getPosition()));
			box2.draw();
			box2.setModelMatrix(glm::translate(glm::mat4(1.0f), btBox2.getPosition()));
			box3.draw();
			box3.setModelMatrix(glm::translate(glm::mat4(1.0f), btBox3.getPosition()));
			textureShader->use();
			textureShader->setUniform("ifNormal", false);

			// all others
			justDoItScreen.draw();
			goodGameScreen.draw();
			scene.Draw();

			// light cubes
			for (int i = 0; i < pointLights.size(); i++) {
				setPerFrameUniformsLight(lightShader.get(), pointLights[i]);	
				lightCubes[i] -> drawShader(lightShader.get());
			}

			double t = glfwGetTime();
			double dt = t - lastT;
			if ((int)floor(lastT) != (int)floor(t)) {
				fps = fpsCounter;
				fpsCounter = 0;
			}
			fpsCounter++;

			lastT = t;

			// check win/lose condition
			if (!_gameLost && t - _start > _timer && !_gameWon) {
				_gameLost = true;
			} else if (!_gameWon && !_gameLost) {
				_gameWon = bulletWorld.checkWinCondition();
			}

			// draw user interface
			if (_hud) {
				_ui->updateUI(fps, _gameLost, _gameWon, _timer - (t - _start), glm::vec3(0, 0, 0));
			}

			// bloom (fragments and render to quad) - has to be after all draw calls!
			blurProcessor.blurFragments(blurShader.get(), bloomResultShader.get());

			// update video texture
			goodGameTexture->updateVideo(dt);
			justDoItTexture->updateVideo(dt);

			// bullet
			bulletWorld.stepSimulation(
				dt, // btScalar timeStep: seconds, not milliseconds, passed since the last call 
				1, // maxSubSteps: should generally stay at one so Bullet interpolates current values on its own
				btScalar(1.) / btScalar(60.)); // fixedTimeStep: inversely proportional to the simulation's resolution

			// render depth map to quad for visual shadow map debugging
			quadShader->use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, shadowMapTexture->getHandle());
			// _quadGeometry.renderQuad(); // remove comment to see shadow map for debug
			
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

void setPerFrameUniformsDepth(Shader* depthShader, std::vector<DirectionalLight> dirLights) {

	depthShader->use();
	
	for (int i = 0; i < dirLights.size(); i++) {
		DirectionalLight& dirL = dirLights[i];
		depthShader->setUniform("lightSpaceMatrix", dirL._lightSpaceMatrix);
	}
}


void setPerFrameUniformsLight(Shader* shader, std::shared_ptr<PointLight> pointL)
{
	shader->use();

	shader->setUniform("viewProjMatrix", _player.getProjectionViewMatrix());
	shader->setUniform("camera_world", _player.getPosition());
	shader->setUniform("brightness", _brightness);
	shader->setUniform("lightColor", pointL->_color);

}


void setPerFrameUniformsTexture(Shader* shader, std::vector<DirectionalLight> dirLights, std::vector<std::shared_ptr<PointLight>> pointLights)
{
	shader->use();
	shader->setUniform("viewProjMatrix", _player.getProjectionViewMatrix());
	shader->setUniform("camera_world", _player.getPosition());
	shader->setUniform("brightness", _brightness);
	shader->setUniform("lightsOn", _lightsOn);

	for (int i = 0; i < dirLights.size(); i++) {
		DirectionalLight& dirL = dirLights[i];
		shader->setUniform("dirLights[" + std::to_string(i) + "].color", dirL._color);
		shader->setUniform("dirLights[" + std::to_string(i) + "].direction", dirL._direction);
		shader->setUniform("lightSpaceMatrix", dirL._lightSpaceMatrix);
	}

	for (int i = 0; i < pointLights.size(); i++) {
		std::shared_ptr<PointLight> pointL = pointLights[i];
		shader->setUniform("pointLights[" + std::to_string(i) + "].color", pointL->_color);
		shader->setUniform("pointLights[" + std::to_string(i) + "].position", pointL->_position);
		shader->setUniform("pointLights[" + std::to_string(i) + "].attenuation", pointL->_attenuation);
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

void poll_keys(GLFWwindow* window, double dt) {
	KeyInput input;
	input.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	input.forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	input.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	input.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
	input.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
	
	if (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS) {
	 	_player.setPressed(false);
	}
	_player.inputKeys(input, dt);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	_zoom -= float(yoffset) * 0.5f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// F1 - Wireframe
	// F2 - Culling
	// F10 - Reset game
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
	case GLFW_KEY_F3:
		_hud = !_hud;
    break;
	case GLFW_KEY_F4:
		_normalToggle = !_normalToggle;
		break;
	case GLFW_KEY_F5:
		_lightsOn = !_lightsOn;
		break;
	case GLFW_KEY_F10:
		_player.moveTo(glm::vec3(0.0f, 5.0f, 0.0f));
		_start = glfwGetTime();
		_gameLost = false;
		_gameWon = false;
		break;
	case GLFW_KEY_F11:

		// glfwGetWindowMonitor(window) returns NULL if windowed
		
		if (glfwGetWindowMonitor(window) == NULL) {
			glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, window_width, window_height, _refresh_rate);
		}
		else {
			glfwSetWindowMonitor(window, NULL, 0, 0, window_width, window_height, _refresh_rate);
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
