#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <memory>

#include <PirateShip/shader_m.h>
#include <PirateShip/camera.h>
#include <PirateShip/model.h>
#include <PirateShip/texture.h>
#include <PirateShip/water_shader.h>
#include <PirateShip/clouds_shader.h>
#include <PirateShip/lighting_shader.h>

#include <stb/stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void create_refraction_mask(Model& refractiveObject, Shader& refractiveShader, glm::vec3 translate, glm::vec3 scale);
unsigned int loadTexture(const char* path);

void render_glass(
	Model& refractiveObject,
	Shader& refractiveShader,
	unsigned int& maskBuffer,
	unsigned int& diffuseMap,
	unsigned int& normalMap,
	unsigned int& specularMap,
	glm::vec3 translate, 
	glm::vec3 scale
);

std::vector<std::vector<glm::vec3>> getTriangles(const std::vector<Model>& hitboxes, const CollisionPackage& collisionPackage);


const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Frame times
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Player
std::shared_ptr<CharacterEntity> entity;
bool gravity = true;


int main() {
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// z-buffer
	glEnable(GL_DEPTH_TEST);

	// Load shaders
	Shader lightingShader("shaders/multiple_lights.vert", "shaders/multiple_lights.frag");
	Shader lightCubeShader("shaders/light_cube.vert", "shaders/light_cube.frag");
	Shader cloudsShader("shaders/clouds.vert", "shaders/clouds.frag");
	Shader waterShader("shaders/water.vert", "shaders/water.frag");
	Shader refractiveShader("shaders/refractive.vert", "shaders/refractive.frag");
	Shader refractiveMaskShader("shaders/refractive_mask.vert", "shaders/refractive_mask.frag");
	Shader screenShader("shaders/framebuffers.vert", "shaders/framebuffers.frag");

	CloudsShader cloudsSettings = CloudsShader();
	WaterShader waterSettings = WaterShader();
	LightingShader lightingSettings = LightingShader();

	// Quad that fills the entire screen for the screen shader
	float quadVertices[] = {
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
	};

	// Screen quad VAO
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// Set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Load and generate the texture
	int width, height, nrChannels;

	// Flip y-axis of image
	stbi_set_flip_vertically_on_load(true);

	// Hide cursor and capture it
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(window, mouse_callback);

	Model ourPlane("resources/plane/plane.obj");
	Model ourCube("resources/cube/cube.obj");
	Model ourDome("resources/dome/dome.obj");
	Model ourPirateShip("resources/pirate_ship/pirateship.obj");
	Model ourHitBox("resources/hitbox/hitbox.obj");
	Model ourBottle("resources/bottle/bottle.obj");
	Model ourSupport("resources/support/support.obj");

	std::vector<Model> hitboxes = { ourHitBox };

	// Flow shaders set up
	unsigned int _CloudTex1 = loadTexture("resources/plane/Clouds_01.jpg");
	unsigned int _CloudTex2 = loadTexture("resources/plane/Clouds_02.jpg");
	unsigned int _FlowTex1 = loadTexture("resources/plane/Clouds_01_Flow.jpg");
	unsigned int _WaveTex = loadTexture("resources/plane/Wave_Dist_1.jpg");
	unsigned int _ColorTex = loadTexture("resources/plane/UpperColor.jpg");
	unsigned int _ColorWaveTex = loadTexture("resources/plane/Waves_Color.jpg");
	unsigned int _WaveTex2 = loadTexture("resources/plane/Waves.png");

	// Glass shader set up
	unsigned int _diffuseMap = loadTexture("resources/bottle/bottle_DIFF.jpg");
	unsigned int _normalMap = loadTexture("resources/bottle/bottle_NORM.jpeg");
	unsigned int _specularMap = loadTexture("resources/bottle/bottle_SPEC.png");

	refractiveShader.use();
	refractiveShader.setInt("diffuseMap", 0);
	refractiveShader.setInt("normalMap", 1);
	refractiveShader.setInt("refractionMap", 2);
	refractiveShader.setInt("environmentMap", 3);
	refractiveShader.setInt("specularMap", 4);

	// Initialize player
	entity = std::make_shared<CharacterEntity>();
	entity->position[1] = 7.0f;
	entity->position[2] = 4.0f;
	entity->triangles = getTriangles(hitboxes, *entity->collisionPackage);
	camera.setEntity(entity);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	// framebuffer configuration
	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	// create a color attachment texture for the mask
	unsigned int maskBuffer;
	glGenTextures(1, &maskBuffer);
	glBindTexture(GL_TEXTURE_2D, maskBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maskBuffer, 0);
	// create a renderbuffer object for depth and stencil attachment
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
	// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;

	// create a color attachment texture
	unsigned int colorTexture;
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	waterSettings.setWaterShader(waterShader);

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glEnable(GL_DEPTH_TEST);

		// Swap back to refractionMask framebuffer texture
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maskBuffer, 0);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// Update player and camera
		entity->update(gravity);
		camera.Position = entity->position;
		entity->velocity = entity->velocity * .05f;

		// Clear buffers
		glClearColor(25.0f/255.0f, 25.0f/ 255.0f, 112.0f/ 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// Ignore depth buffer when rendering skybox
		glDepthMask(GL_FALSE);

		// Render clouds
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -125.0f, 0.0f));
		model = glm::scale(model, glm::vec3(200.0f, 200.0f, 200.0f));
		
		cloudsShader.use();
		cloudsShader.setMat4("projection", projection);
		cloudsShader.setMat4("view", view);
		cloudsShader.setMat4("model", model);
		cloudsShader.setVec3("viewPos", camera.Position);
		cloudsShader.setFloat("_Time", glfwGetTime());

		cloudsSettings.setCloudsShader(cloudsShader);
		cloudsSettings.bindCloudsTextures(cloudsShader, _CloudTex1, _FlowTex1, 
										  _CloudTex2, _WaveTex, _ColorTex);

		ourDome.Draw2(cloudsShader);

		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);

		// Render water
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -10.0f, 0.0f));
		model = glm::scale(model, glm::vec3(10000.0f, 10000.0f, 10000.0f));
		
		waterShader.use();
		waterShader.setMat4("projection", projection);
		waterShader.setMat4("view", view);
		waterShader.setMat4("model", model);
		waterShader.setVec3("viewPos", camera.Position);
		waterShader.setFloat("_Time", glfwGetTime());

		waterSettings.setWaterShader(waterShader);
		waterSettings.bindWaterTextures(waterShader);

		ourPlane.Draw2(waterShader);

		// Render objects with general lighting shader
		lightingShader.use();
		lightingSettings.setLightingShader(lightingShader);

		// Render ship
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02, 0.02, 0.02));
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);
		lightingShader.setMat4("model", model);
		lightingShader.setMat4("model", model);
		lightingShader.setVec3("viewPos", camera.Position);

		ourPirateShip.Draw(lightingShader);

		// Change hitbox size
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
		model = glm::scale(model, glm::vec3(200, 200, 200));

		// Render the hitbox for debugging
		//lightingShader.setMat4("model", model);
		//ourHitBox.Draw(lightingShader);

		// Adjust physical hitbox coordinates based on render coordinates
		entity->triangles = getTriangles(hitboxes, *entity->collisionPackage);
		for (auto& triangle : entity->triangles) {
			glm::vec4 first = glm::vec4(triangle[0], 1);
			glm::vec4 second = glm::vec4(triangle[1], 1);
			glm::vec4 third = glm::vec4(triangle[2], 1);
			first = model * first;
			second = model * second;
			third = model * third;
			triangle = { glm::vec3(first), glm::vec3(second), glm::vec3(third) };
		}

		// Bottle scale and translation
		glm::vec3 bottle_translate = glm::vec3(0.0f, 25.5f, 0.0f);
		glm::vec3 bottle_scale = glm::vec3(15.0f, 15.0f, 15.0f);

		// Render wood bottle support
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -3.95f, 5.0f));
		model = glm::scale(model, bottle_scale);

		lightingShader.setMat4("model", model);
		ourSupport.Draw(lightingShader);

		// Render glass bottle
		create_refraction_mask(ourBottle, refractiveMaskShader, bottle_translate, bottle_scale);

		// Swap framebuffer textures
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
		
		// Clear the buffer for drawing the bottle
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		render_glass(ourBottle, refractiveShader, maskBuffer, _diffuseMap, 
					 _normalMap, _specularMap, bottle_translate, bottle_scale);

		// Bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// Disable depth test so screen-space quad isn't discarded due to depth test
		glDisable(GL_DEPTH_TEST); 
		// Clear buffers
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		screenShader.use();
		glBindVertexArray(quadVAO);

		// Use the framebuffer color texture as the texture of the quad plane
		// Draw everything that was rendered before the refractive object
		glBindTexture(GL_TEXTURE_2D, maskBuffer);	
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Draw the refractive object
		glBindTexture(GL_TEXTURE_2D, colorTexture);	
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		gravity = !gravity;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}


// Update mouse position and process mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}


// Convert all the triangles of a hitbox into ellipsoid space and put them in an array for collision
std::vector<std::vector<glm::vec3>> getTriangles(const std::vector<Model>& hitboxes, const CollisionPackage& collisionPackage) {
	std::vector<std::vector<glm::vec3>> triangles;

	for (const auto& hitbox : hitboxes) {
		for (const auto& mesh : hitbox.meshes) {
			for (int i = 0; i < mesh.indices.size();) {
				// Get indices of a triangle's vertices
				int a = mesh.indices[i++];
				int b = mesh.indices[i++];
				int c = mesh.indices[i++];

				// Three coordinates of a triangle in ellipsoid space
				glm::vec3 first = mesh.vertices[a].Position / collisionPackage.eRadius;
				glm::vec3 second = mesh.vertices[b].Position / collisionPackage.eRadius;
				glm::vec3 third = mesh.vertices[c].Position / collisionPackage.eRadius;

				std::vector<glm::vec3> triangle = { first, second, third };
			
				triangles.push_back(triangle);
			}
		}
	}

	return triangles;
}


// Create a mask by rendering the refractive object to the framebuffer's alpha channel
// Other objects in front will cut away from the mask and won't show up in refraction
void create_refraction_mask(Model& refractiveObject, Shader& refractiveShader, glm::vec3 translate, glm::vec3 scale) {
	// Write to the alpha channel
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	// Disable writing to depth buffer
	glDepthMask(GL_FALSE);

	// Set up shader
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, translate);
	model = glm::scale(model, scale);

	refractiveShader.use();
	refractiveShader.setMat4("projection", projection);
	refractiveShader.setMat4("view", view);
	refractiveShader.setMat4("model", model);

	// Draw refractive object
	refractiveObject.Draw2(refractiveShader);

	// Reset frame buffer write mode
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Reset writing to depth buffer
	glDepthMask(GL_TRUE);
}


// Render a refractive object
void render_glass(
	Model& refractiveObject, 
	Shader& refractiveShader, 
	unsigned int& maskBuffer,
	unsigned int& diffuseMap,
	unsigned int& normalMap,
	unsigned int& specularMap,
	glm::vec3 translate, 
	glm::vec3 scale
) {
	// Render refractive geometry
	refractiveShader.use();

	// Set up shader
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, translate);
	model = glm::scale(model, scale);

	refractiveShader.use();
	refractiveShader.setMat4("projection", projection);
	refractiveShader.setMat4("view", view);
	refractiveShader.setMat4("model", model);
	refractiveShader.setVec3("vCameraPos", camera.Position);

	// Set background texture
	// What's behind the object
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(refractiveShader.ID, "refractionMap"), 0);
	glBindTexture(GL_TEXTURE_2D, maskBuffer);

	// Set diffuse map
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(glGetUniformLocation(refractiveShader.ID, "diffuseMap"), 1);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);

	// Set normal map
	glActiveTexture(GL_TEXTURE2);
	glUniform1i(glGetUniformLocation(refractiveShader.ID, "normalMap"), 2);
	glBindTexture(GL_TEXTURE_2D, normalMap);

	// Set specular map
	glActiveTexture(GL_TEXTURE3);
	glUniform1i(glGetUniformLocation(refractiveShader.ID, "specularMap"), 3);
	glBindTexture(GL_TEXTURE_2D, specularMap);

	// Directional light
	refractiveShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	refractiveShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

	// Draw refractive object
	refractiveObject.Draw2(refractiveShader);
}
