#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include <PirateShip/shader_m.h>
#include <PirateShip/camera.h>
#include <PirateShip/model.h>

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

void setWaterShader(Shader& waterShader);
void bindWaterTextures(
	Shader& waterShader,
	unsigned int _CloudTex1,
	unsigned int _FlowTex1,
	unsigned int _CloudTex2,
	unsigned int _WaveTex,
	unsigned int _ColorTex
);

std::vector<std::vector<glm::vec3>> getTriangles(std::vector<Model> models, CollisionPackage &collisionPackage);


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
CharacterEntity* entity;
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
	Shader screenShader("shaders/framebuffers_screen.vert", "shaders/framebuffers_screen.frag");

	glm::vec3 pointLightPositions[] = {
		glm::vec3(16.9275f, 23.8319f, 43.2494f),
		glm::vec3(-20.9641f, 35.7645f, 10.6067f),
		glm::vec3(-14.3825f, 25.638f, - 31.6371f),
		glm::vec3(21.0417f, 13.0547f, 9.31641f)
	};

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

	std::vector<Model> models = { ourHitBox };

	unsigned int _CloudTex1 = loadTexture("resources/plane/Clouds_01.jpg");
	unsigned int _CloudTex2 = loadTexture("resources/plane/Clouds_02.jpg");
	unsigned int _FlowTex1 = loadTexture("resources/plane/Clouds_01_Flow.jpg");
	unsigned int _WaveTex = loadTexture("resources/plane/Wave_Dist_1.jpg");
	unsigned int _ColorTex = loadTexture("resources/plane/UpperColor.jpg");
	unsigned int _ColorWaveTex = loadTexture("resources/plane/Waves_Color.jpg");
	unsigned int _WaveTex2 = loadTexture("resources/plane/Waves.png");

	cloudsShader.use();
	cloudsShader.setInt("_CloudTex1", 0);
	cloudsShader.setInt("_FlowTex1", 1);
	cloudsShader.setInt("_CloudTex2", 2);
	cloudsShader.setInt("_WaveTex", 3);
	cloudsShader.setInt("_ColorTex", 4);

	cloudsShader.setVec4("_Tiling1", glm::vec4(0.1, 0.1, 0, 1));
	cloudsShader.setVec4("_Tiling2", glm::vec4(4, 4, 0, 0));
	cloudsShader.setVec4("_TilingWave", glm::vec4(0.1, 0.1, 0, 5));
	
	cloudsShader.setFloat("_CloudScale", 1.0f);
	cloudsShader.setFloat("_CloudBias", 0.0f);
	
	cloudsShader.setFloat("_Cloud2Amount", 2.0f);
	cloudsShader.setFloat("_WaveAmount", 0.6f);
	cloudsShader.setFloat("_WaveDistort", 0.05f);
	cloudsShader.setFloat("_FlowSpeed", -3.0f);
	cloudsShader.setFloat("_FlowAmount", 1.0f);

	cloudsShader.setVec4("_TilingColor", glm::vec4(0.05f, 0.05f, 0.0f, 1.0f));

	cloudsShader.setVec4("_Color", glm::vec4(0.9495942f, 0.4779412f, 1.0f, 1.0f));
	cloudsShader.setVec4("_Color2", glm::vec4(0.3868124f, 0.3822448f, 0.5147059f, 1.0f));

	cloudsShader.setFloat("_CloudDensity", 7.0f);

	cloudsShader.setFloat("_BumpOffset", 1.0f);
	cloudsShader.setFloat("_Steps", 70.0f);

	cloudsShader.setFloat("_CloudHeight", 500.0f);
	cloudsShader.setFloat("_Scale", 0.5f);
	cloudsShader.setFloat("_Speed", 0.01f);

	cloudsShader.setVec4("_LightSpread", glm::vec4(5.0, 10.0, 40.0, 100.0));

	cloudsShader.setFloat("_ColPow", 5.0f);
	cloudsShader.setFloat("_ColFactor", 20.0f);

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

	entity = new CharacterEntity();

	// initialize player in front of model
	entity->position[1] = 7.0f;
	entity->position[2] = 4.0f;

	camera.setEntity(entity);

	entity->triangles = getTriangles(models, *entity->collisionPackage);

	glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	// framebuffer configuration
	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	// create a color attachment texture
	unsigned int maskBuffer;
	glGenTextures(1, &maskBuffer);
	glBindTexture(GL_TEXTURE_2D, maskBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maskBuffer, 0);
	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
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

	setWaterShader(waterShader);

	// render loop
	while (!glfwWindowShouldClose(window))
	{

		std::cout << entity->position.x << " " << entity->position.y << " " << entity->position.z << "\n";

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glEnable(GL_DEPTH_TEST);

		// Swap back to refractionMask framebuffer texture
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maskBuffer, 0);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		entity->update(gravity);
		camera.Position = entity->position;
		entity->velocity = entity->velocity * .05f;

		// rendering commands here
		glClearColor(25.0f/255.0f, 25.0f/ 255.0f, 112.0f/ 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

		// use camera class to get projection and view matrices
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);

		cloudsShader.use();
		cloudsShader.setMat4("projection", projection);
		cloudsShader.setMat4("view", view);
		cloudsShader.setVec3("viewPos", camera.Position);

		glDepthMask(GL_FALSE);

		// Render the clouds
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -125.0f, 0.0f));
		model = glm::scale(model, glm::vec3(200.0f, 200.0f, 200.0f));

		cloudsShader.setMat4("model", model);

		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(cloudsShader.ID, "_CloudTex1"), 0);
		glBindTexture(GL_TEXTURE_2D, _CloudTex1);

		glActiveTexture(GL_TEXTURE1);
		glUniform1i(glGetUniformLocation(cloudsShader.ID, "_FlowTex1"), 1);
		glBindTexture(GL_TEXTURE_2D, _FlowTex1);

		glActiveTexture(GL_TEXTURE2);
		glUniform1i(glGetUniformLocation(cloudsShader.ID, "_CloudTex2"), 2);
		glBindTexture(GL_TEXTURE_2D, _CloudTex2);

		glActiveTexture(GL_TEXTURE3);
		glUniform1i(glGetUniformLocation(cloudsShader.ID, "_WaveTex"), 3);
		glBindTexture(GL_TEXTURE_2D, _WaveTex);

		glActiveTexture(GL_TEXTURE4);
		glUniform1i(glGetUniformLocation(cloudsShader.ID, "_ColorTex"), 4);
		glBindTexture(GL_TEXTURE_2D, _ColorTex);

		cloudsShader.setFloat("_Time", glfwGetTime());

		ourDome.Draw2(cloudsShader);

		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);

		// draw objects
		lightingShader.use();
		lightingShader.setVec3("viewPos", camera.Position);
		lightingShader.setFloat("material.shininess", 32.0f);

		// directional light
		lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.25f);
		lightingShader.setVec3("dirLight.diffuse", 0.5f, 0.6f, 0.5f);
		lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

		// point light 1
		lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
		lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[0].constant", 1.0f);
		lightingShader.setFloat("pointLights[0].linear", 0.09);
		lightingShader.setFloat("pointLights[0].quadratic", 0.032);
		// point light 2
		lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
		lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[1].constant", 1.0f);
		lightingShader.setFloat("pointLights[1].linear", 0.09);
		lightingShader.setFloat("pointLights[1].quadratic", 0.032);
		// point light 3
		lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
		lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[2].constant", 1.0f);
		lightingShader.setFloat("pointLights[2].linear", 0.09);
		lightingShader.setFloat("pointLights[2].quadratic", 0.032);
		// point light 4
		lightingShader.setVec3("pointLights[3].position", pointLightPositions[3]);
		lightingShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[3].constant", 1.0f);
		lightingShader.setFloat("pointLights[3].linear", 0.09);
		lightingShader.setFloat("pointLights[3].quadratic", 0.032);

		// pass transformation matrices to the shader
		model = glm::mat4(1.0f);
		lightingShader.setMat4("projection", projection); // note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
		lightingShader.setMat4("view", view);
		lightingShader.setMat4("model", model);

		// render the water
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -10.0f, 0.0f));
		model = glm::scale(model, glm::vec3(10000.0f, 10000.0f, 10000.0f));
		
		waterShader.use();
		waterShader.setMat4("projection", projection);
		waterShader.setMat4("view", view);
		waterShader.setVec3("viewPos", camera.Position);
		waterShader.setMat4("model", model);

		waterShader.setFloat("_Time", glfwGetTime());

		waterShader.setFloat("_Scale", 10000.0f);

		waterShader.setVec4("_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		waterShader.setVec4("_Color2", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		
		bindWaterTextures(waterShader, _CloudTex1, _FlowTex1, _WaveTex2, _WaveTex, _ColorWaveTex);

		ourPlane.Draw2(waterShader);

		lightingShader.use();

		// render the hitbox
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
		model = glm::scale(model, glm::vec3(200, 200, 200));

		lightingShader.setMat4("model", model);
		//ourHitBox.Draw(lightingShader);

		entity->triangles = getTriangles(models, *entity->collisionPackage);

		for (int i = 0; i < entity->triangles.size(); i++) {
			std::vector<glm::vec3> triangle = entity->triangles[i];
			glm::vec4 first = glm::vec4(triangle[0], 1);
			glm::vec4 second = glm::vec4(triangle[1], 1);
			glm::vec4 third = glm::vec4(triangle[2], 1);
			first = model * first;
			second = model * second;
			third = model * third;
			triangle = { glm::vec3(first), glm::vec3(second), glm::vec3(third) };
			entity->triangles[i] = triangle;
		}

		// render the ship
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02, 0.02, 0.02));

		lightingShader.setMat4("model", model);
		ourPirateShip.Draw(lightingShader);

		// Bottle scale and translation
		glm::vec3 bottle_translate = glm::vec3(0.0f, 25.5f, 0.0f);
		glm::vec3 bottle_scale = glm::vec3(15.0f, 15.0f, 15.0f);

		// render the support
		model = glm::mat4(1.0f);
		// bottle_scale.y * 2.1f, bottle_scale.z * .3f
		model = glm::translate(model, glm::vec3(0.0f, -3.95f, 5.0f));
		model = glm::scale(model, bottle_scale);

		lightingShader.setMat4("model", model);
		ourSupport.Draw(lightingShader);

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

		// Use the framebuffer color texture as the texture of the quad plane
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


// Utility function for loading a 2D texture from file
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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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


// Convert all the triangles in a mesh into ellipsoid space and put them in an array for collision
std::vector<std::vector<glm::vec3>> getTriangles(std::vector<Model> models, CollisionPackage &collisionPackage) {
	std::vector<std::vector<glm::vec3>> triangles;

	for (Model model : models) {
		for (Mesh mesh : model.meshes) {
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


void setWaterShader(Shader& waterShader) {
	unsigned int _CloudTex1 = loadTexture("resources/plane/Clouds_01.jpg");
	unsigned int _FlowTex1 = loadTexture("resources/plane/Clouds_01_Flow.jpg");
	unsigned int _CloudTex2 = loadTexture("resources/plane/Clouds_02.jpg");
	unsigned int _WaveTex = loadTexture("resources/plane/Wave_Dist_1.jpg");
	unsigned int _ColorWaveTex = loadTexture("resources/plane/Waves_Color.jpg");

	waterShader.use();
	waterShader.setInt("_CloudTex1", 0);
	waterShader.setInt("_FlowTex1", 1);
	waterShader.setInt("_CloudTex2", 2);
	waterShader.setInt("_WaveTex", 3);
	waterShader.setInt("_ColorTex", 4);

	waterShader.setVec4("_Tiling1", glm::vec4(0.1, 0.1, 0, 1));
	waterShader.setVec4("_Tiling2", glm::vec4(4, 4, 0, 0));
	waterShader.setVec4("_TilingWave", glm::vec4(0.1, 0.1, 0, 5));

	waterShader.setFloat("_CloudScale", 1.0f);
	waterShader.setFloat("_CloudBias", 0.0f);

	waterShader.setFloat("_Cloud2Amount", 2.0f);
	waterShader.setFloat("_WaveAmount", 0.6f);
	waterShader.setFloat("_WaveDistort", 0.05f);
	waterShader.setFloat("_FlowSpeed", -3.0f);
	waterShader.setFloat("_FlowAmount", 1.0f);

	waterShader.setVec4("_TilingColor", glm::vec4(0.05f, 0.05f, 0.0f, 1.0f));

	waterShader.setVec4("_Color", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

	waterShader.setFloat("_CloudDensity", 7.0f);

	waterShader.setFloat("_BumpOffset", 1.0f);
	waterShader.setFloat("_Steps", 70.0f);

	waterShader.setFloat("_CloudHeight", 500.0f);
	waterShader.setFloat("_Scale", 0.01f);
	waterShader.setFloat("_Speed", 0.01f);

	waterShader.setVec4("_LightSpread", glm::vec4(5.0, 10.0, 40.0, 100.0));

	waterShader.setFloat("_ColPow", 5.0f);
	waterShader.setFloat("_ColFactor", 20.0f);
}


void bindWaterTextures(Shader& waterShader, unsigned int _CloudTex1, unsigned int _FlowTex1, unsigned int _CloudTex2, unsigned int _WaveTex, unsigned int _ColorTex) {
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(waterShader.ID, "_CloudTex1"), 0);
	glBindTexture(GL_TEXTURE_2D, _CloudTex1);

	glActiveTexture(GL_TEXTURE1);
	glUniform1i(glGetUniformLocation(waterShader.ID, "_FlowTex1"), 1);
	glBindTexture(GL_TEXTURE_2D, _FlowTex1);

	glActiveTexture(GL_TEXTURE2);
	glUniform1i(glGetUniformLocation(waterShader.ID, "_CloudTex2"), 2);
	glBindTexture(GL_TEXTURE_2D, _CloudTex2);

	glActiveTexture(GL_TEXTURE3);
	glUniform1i(glGetUniformLocation(waterShader.ID, "_WaveTex"), 3);
	glBindTexture(GL_TEXTURE_2D, _WaveTex);

	glActiveTexture(GL_TEXTURE4);
	glUniform1i(glGetUniformLocation(waterShader.ID, "_ColorTex"), 4);
	glBindTexture(GL_TEXTURE_2D, _ColorTex);
}
