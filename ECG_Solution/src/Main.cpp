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

void setPerFrameUniformsTexture(Shader* shader,std::vector<DirectionalLight> dirLights, std::vector<PointLight> pointLights);

void renderQuad();
void renderQuad(unsigned int textureColorbuffer);
void setPerFrameUniformsDepth(Shader* depthShader, std::vector<DirectionalLight> dirLights);
void setPerFrameUniformsLight(Shader* shader, PointLight& pointL, std::shared_ptr<Material> lightMaterial);

glm::mat4 lookAtView(glm::vec3 eye, glm::vec3 at, glm::vec3 up);

/* --------------------------------------------- */
// Global variables
/* --------------------------------------------- */

static bool _wireframe = false;
static bool _culling = true;
static bool _dragging = false;
static bool _strafing = false;
static float _zoom = 6.0f;
static CameraPlayer _player(glm::vec3(0.0f, 5.0f, 0.0f));

int _timer = 300;
boolean _gameLost = false;

int window_width, window_height, _refresh_rate;
GLFWmonitor* monitor;
bool fullscreen;
float _brightness;
float exposure = 1.0f;

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
	float farZ = float(reader.GetReal("camera", "far", 1000.0f));
	string _fontpath = "assets/fonts/Roboto-Regular.ttf";

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

		// -----------------------
		
		// Create textures
		std::shared_ptr<ShadowMapTexture> shadowMapTexture = std::make_shared<ShadowMapTexture>(window_width, window_height);

		std::shared_ptr<Texture> woodTexture = std::make_shared<Texture>("assets/textures/wood_texture.dds", shadowMapTexture->getHandle());
		std::shared_ptr<Texture> tileTexture = std::make_shared<Texture>("assets/textures/tiles_diffuse.dds", shadowMapTexture->getHandle());

		// Create materials
		std::shared_ptr<Material> woodTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, woodTexture);
		std::shared_ptr<Material> tileTextureMaterial = std::make_shared<TextureMaterial>(textureShader, glm::vec3(0.1f, 0.5f, 0.1f), 2.0f, tileTexture);

		std::shared_ptr<Material> catModelMaterial = std::make_shared<TextureMaterial>(textureShader);
		std::shared_ptr<Material> depthMaterial = std::make_shared<TextureMaterial>(depthShader);
		std::shared_ptr<Material> lightMaterial = std::make_shared<TextureMaterial>(lightShader);

		// Create geometry
		Geometry mainBox(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f)), Geometry::createCubeGeometry(0.5f, 0.5f, 0.5f), woodTextureMaterial);
		Geometry mainBox2(glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f)), Geometry::createCubeGeometry(1.5f, 0.5f, 1.5f), woodTextureMaterial);
		Geometry mainBox3(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 4.0f)), Geometry::createCubeGeometry(1.5f, 1.5f, 0.5f), woodTextureMaterial);

		glm::mat4 catModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));
		ModelLoader cat("assets/objects/cat/cat.obj", catModel, catModelMaterial);
		BulletBody btCat(btTag, Geometry::createCubeGeometry(0.4f, 0.5f, 0.2f), 1.0f, true, glm::vec3(0.0f, 10.0f, 0.0f), bulletWorld._world);

		glm::mat4 sceneModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		ModelLoader scene("assets/objects/scene.obj", sceneModel, catModelMaterial);
		
		for (const auto& mesh : scene.getMeshes()) {
			// store bullet body in a list or another data structure 
			std::cout << mesh._aiMesh->mName.C_Str() << std::endl;
			std::cout << mesh._aiMesh->mNumVertices << std::endl;
			string name(mesh._aiMesh->mName.C_Str());
			if (!(name.compare("hull"))) {
				//std::cout << "this is hull" << std::endl;
				BulletBody btScene(btTag, mesh._aiMesh, mesh._transformationMatrix, 0.0f, false, bulletWorld._world);
			}
			else {
				BulletBody btScene(btTag, mesh._aiMesh, mesh._transformationMatrix, 0.0f, true, bulletWorld._world);
			}
		}
		
		// Initialize camera
		// Camera camera(fov, float(window_width) / float(window_height), nearZ, farZ);

		// Initialize user interface/HUD
		_ui = std::make_shared<UserInterface>("userinterface.vert", "userinterface.frag", window_width, window_height, _brightness, _fontpath);

		// Initialize lights and put them into vector
		// NOTE: to set up number of lights "#define NR_DIR_LIGHTS" and "#define NR_POINT_LIGHTS" in "texture.frag" has to be updated!
		#pragma region directional lights

		//white
		DirectionalLight dirL1(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		dirLights.push_back(dirL1);
		// orangey
		DirectionalLight dirL2(glm::vec3(0.3f, 0.2f, 0.1f), glm::vec3(0.0f, -1.0f, -1.0f));
		dirLights.push_back(dirL2);
		// pinkish
		DirectionalLight dirL3(glm::vec3(0.4f, 0.2f, 0.3f), glm::vec3(0.0f, -1.0f, 1.0f));
		dirLights.push_back(dirL3);
		#pragma endregion

		#pragma region point lights
		// turqoise
		PointLight pointL1(glm::vec3(0.0f, 3.0f, 3.0f), glm::vec3(1.0f, 1.0f, -2.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL1);
		// yellow
		PointLight pointL2(glm::vec3(5.0f, 5.0f, 0.0f), glm::vec3(1.0f, 1.2f, 1.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL2);
		// green
		PointLight pointL3(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(-2.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL3);
		// white 
		PointLight pointL4(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(-2.0f, 1.0f, -2.0f), glm::vec3(1.0f, 0.7f, 1.8f));
		pointLights.push_back(pointL4);

		#pragma endregion

		// Render loop
		float lastT = float(glfwGetTime());
		float dt = 0.0f;
		float t_sum = 0.0f;
		double mouse_x, mouse_y;
		int fpsCounter = 0;
		double lastTime = glfwGetTime();
		int fps = 0;

		double last_mouse_x, last_mouse_y;
		glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);



		// --------------------------
		// framebuffer configuration
		unsigned int framebuffer;
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		// create a color attachment texture
		unsigned int textureColorbuffer[2]; // here

		glGenTextures(2, textureColorbuffer);

		for (unsigned int i = 0; i < 2; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textureColorbuffer[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// attach texture to framebuffer
			//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textureColorbuffer[i], 0
			);
		}

		// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
		unsigned int rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it

		// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
		unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		 

		// ----------------------
		// ping-pong-framebuffer for blurring
		unsigned int pingpongFBO[2];
		unsigned int pingpongColorbuffers[2];
		glGenFramebuffers(2, pingpongFBO);
		glGenTextures(2, pingpongColorbuffers);
		for (unsigned int i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
			// also check if framebuffers are complete (no need for depth buffer)
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Framebuffer not complete!" << std::endl;
		}



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

			// shadowmapping
			// 1. render depth of scene to texture (from light's perspective) (is done in dirLight constructor)
			setPerFrameUniformsDepth(depthShader.get(), dirLights);
			shadowMapTexture->activate();

			mainBox.drawShader(depthShader.get());
			mainBox2.drawShader(depthShader.get());
			mainBox3.drawShader(depthShader.get());
			cat.SetModelMatrix(glm::translate(glm::mat4(1.0f), btCat.getPosition()));
			cat.DrawShader(depthShader.get());
			scene.DrawShader(depthShader.get());

			shadowMapTexture->resetViewPort();

			// 2. render scene as normal using the generated depth/shadow map 
			
			// start initial framebuffer 
			// first pass			
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
			glEnable(GL_DEPTH_TEST);
			// end initial framebuffer 

			setPerFrameUniformsTexture(textureShader.get(), dirLights, pointLights);

			// render
			mainBox.draw();
			mainBox2.draw();
			mainBox3.draw();
			cat.SetModelMatrix(glm::translate(glm::mat4(1.0f), btCat.getPosition()));
			cat.Draw();
			scene.Draw();

			// light cubes
			for (int i = 0; i < pointLights.size(); i++) {
				PointLight& pointL = pointLights[i];
				Geometry lightbox(glm::translate(glm::mat4(1.0f), pointL._position), Geometry::createCubeGeometry(0.5f, 0.5f, 0.5f), lightMaterial);

				setPerFrameUniformsLight(lightShader.get(), pointL, lightMaterial);
				lightbox.drawShader(lightShader.get());
			}
			
			// 2. blur bright fragments with two-pass Gaussian Blur 
			// --------------------------------------------------
			bool horizontal = true, first_iteration = true;
			unsigned int amount = 10;
			blurShader->use();
			for (unsigned int i = 0; i < amount; i++)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
				blurShader -> setUniform("horizontal", horizontal);
				glBindTexture(GL_TEXTURE_2D, first_iteration ? textureColorbuffer[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
				renderQuad();
				horizontal = !horizontal;
				if (first_iteration)
					first_iteration = false;
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			// 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
			// --------------------------------------------------------------------------------------------------------------------------
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			bloomResultShader -> use();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureColorbuffer[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);

			bloomResultShader->setUniform("exposure", exposure);
			renderQuad();


			// start initial framebuffer 
			// second pass			
			//glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
			//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			//glClear(GL_COLOR_BUFFER_BIT);

			//quadShader->use();
			//renderQuad(textureColorbuffer[0]);
			// end initial framebuffer 
			
			double t = glfwGetTime();
			double dt = t - lastT;
			if ((int)floor(lastT) != (int)floor(t)) {
				fps = fpsCounter;
				fpsCounter = 0;
			}
			fpsCounter++;

			lastT = t;

			// draw user interface
			_ui->updateUI(fps, false, false, glm::vec3(0, 0, 0));

			// bullet
			bulletWorld.stepSimulation(
				dt, // btScalar timeStep: seconds, not milliseconds, passed since the last call 
				1, // maxSubSteps: should generally stay at one so Bullet interpolates current values on its own
				btScalar(1.) / btScalar(60.)); // fixedTimeStep: inversely proportional to the simulation's resolution

			// render depth map to quad for visual debugging
			quadShader->use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, shadowMapTexture->getHandle());
			//renderQuad();

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


void setPerFrameUniformsLight(Shader* shader, PointLight& pointL, std::shared_ptr<Material> lightMaterial)
{
	shader->use();

	shader->setUniform("viewProjMatrix", _player.getProjectionViewMatrix());
	shader->setUniform("camera_world", _player.getPosition());
	shader->setUniform("brightness", _brightness);
	shader->setUniform("lightColor", pointL._color);

}


void setPerFrameUniformsTexture(Shader* shader, std::vector<DirectionalLight> dirLights, std::vector<PointLight> pointLights)
{
	shader->use();
	//shader->setUniform("viewProjMatrix", camera.getViewProjectionMatrix());
	//shader->setUniform("camera_world", camera.getPosition());
	shader->setUniform("viewProjMatrix", _player.getProjectionViewMatrix());
	shader->setUniform("camera_world", _player.getPosition());
	shader->setUniform("brightness", _brightness);

	for (int i = 0; i < dirLights.size(); i++) {
		DirectionalLight& dirL = dirLights[i];
		shader->setUniform("dirLights[" + std::to_string(i) + "].color", dirL._color);
		shader->setUniform("dirLights[" + std::to_string(i) + "].direction", dirL._direction);
		shader->setUniform("lightSpaceMatrix", dirL._lightSpaceMatrix);
	}
	for (int i = 0; i < pointLights.size(); i++) {
		PointLight& pointL = pointLights[i];
		shader->setUniform("pointLights[" + std::to_string(i) + "].color", pointL._color);
		shader->setUniform("pointLights[" + std::to_string(i) + "].position", pointL._position);
		shader->setUniform("pointLights[" + std::to_string(i) + "].attenuation", pointL._attenuation);
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
	input.right = glfwGetKey(window, GLFW_KEY_D);
	input.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
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

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad(unsigned int textureColorbuffer)
{
	if (quadVAO == 0)
	{
		float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}

	glBindVertexArray(quadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

// renderQuad() renders a 1x1 XY quad in NDC
// for debugging shadow!
// -----------------------------------------
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