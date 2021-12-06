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
void create_refraction_map(Model& refractiveObject, Shader& refractiveShader, glm::vec3 translate, glm::vec3 scale);
void render_glass(
	Model& refractiveObject,
	Shader& refractiveShader,
	unsigned int& maskBuffer,
	unsigned int& diffuseMap,
	unsigned int& normalMap,
	glm::vec3 translate, 
	glm::vec3 scale
);
unsigned int loadTexture(const char* path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

CharacterEntity* entity;

std::vector<std::vector<glm::vec3>> getTriangles(std::vector<Model> models, CollisionPackage &collisionPackage);

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

int main() {
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// z-buffer
	glEnable(GL_DEPTH_TEST);


	Shader lightingShader("multiple_lights.vert", "multiple_lights.frag");
	Shader lightCubeShader("light_cube.vert", "light_cube.frag");
	Shader cloudsShader("clouds.vert", "clouds.frag");
	Shader refractiveShader("refractive.vert", "refractive.frag");
	Shader refractiveMaskShader("refractive_mask.vert", "refractive_mask.frag");

	float vertices[] = {
		// positions			 // normals					// texture coords
		-0.5f, -0.5f, -0.5f,	 0.0f,  0.0f, -1.0f,		0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,	 0.0f,  0.0f, -1.0f,		1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,	 0.0f,  0.0f, -1.0f,		1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,	 0.0f,  0.0f, -1.0f,		1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,	 0.0f,  0.0f, -1.0f,		0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,	 0.0f,  0.0f, -1.0f,		0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,	 0.0f,  0.0f,  1.0f,		0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,	 0.0f,  0.0f,  1.0f,		1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,	 0.0f,  0.0f,  1.0f,		1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,	 0.0f,  0.0f,  1.0f,		1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,	 0.0f,  0.0f,  1.0f,		0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,	 0.0f,  0.0f,  1.0f,		0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,		1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,		1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,		0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,		0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,		0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,		1.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,	 1.0f,  0.0f,  0.0f,		1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,	 1.0f,  0.0f,  0.0f,		1.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,	 1.0f,  0.0f,  0.0f,		0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,	 1.0f,  0.0f,  0.0f,		0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,	 1.0f,  0.0f,  0.0f,		0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,	 1.0f,  0.0f,  0.0f,		1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,	 0.0f, -1.0f,  0.0f,		0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,	 0.0f, -1.0f,  0.0f,		1.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,	 0.0f, -1.0f,  0.0f,		1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,	 0.0f, -1.0f,  0.0f,		1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,	 0.0f, -1.0f,  0.0f,		0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,	 0.0f, -1.0f,  0.0f,		0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,	 0.0f,  1.0f,  0.0f,		0.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,	 0.0f,  1.0f,  0.0f,		1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,	 0.0f,  1.0f,  0.0f,		1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,	 0.0f,  1.0f,  0.0f,		1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,	 0.0f,  1.0f,  0.0f,		0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,	 0.0f,  1.0f,  0.0f,		0.0f,  1.0f
	};

	glm::vec3 cubePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f,  2.0f, -2.5f),
		glm::vec3(1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	};


	// Screen shader
	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
	};
	
	Shader screenShader("5.1.framebuffers_screen.vs", "5.1.framebuffers_screen.fs");
	screenShader.use();
	screenShader.setInt("screenTexture", 0);

	// screen quad VAO
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

	// cube's VAO
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// light's VAO
	// uses same VBO as cube
	unsigned int lightCubeVAO;
	glGenVertexArrays(1, &lightCubeVAO);
	glBindVertexArray(lightCubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// unbind VAO
	//glBindVertexArray(0);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	int width, height, nrChannels;

	// flip y-axis of image
	stbi_set_flip_vertically_on_load(true);

	unsigned int diffuseMap = loadTexture("resources/textures/container2.png");
	unsigned int specularMap = loadTexture("resources/textures/container2_specular.png");

	lightingShader.use();
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);


	// hide cursor and capture it
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(window, mouse_callback);

	Model ourModel("resources/models/backpack.obj");
	Model ourPlane("resources/plane/plane.obj");
	Model ourPlane2("resources/plane/plane.obj");
	Model ourCube("resources/cube/cube.obj");

	//std::vector<std::vector<glm::vec3>> triangles = getTriangles(ourCube);

	Model ourDome("resources/dome/dome.obj");
	//Model ourPirateShip("resources/pirate_ship/pirateship.obj");
	Model ourPirateShip("resources/pirate_ship/lowpoly.fbx");
	//Model ourHitBox("resources/hitbox/hitbox.fbx");
	Model ourHitBox("resources/hitbox/hitbox.obj");
	Model ourBottle("resources/bottle/ship_in_a_bottle_modified4.obj");
	Model ourSupport("resources/support/support.obj");

	//std::vector<Model> models = { ourPlane2, ourCube };
	//std::vector<Model> models = { ourPlane2 };
	std::vector<Model> models = { ourHitBox };

	unsigned int _CloudTex1 = loadTexture("resources/plane/Clouds_01.jpg");
	unsigned int _FlowTex1 = loadTexture("resources/plane/Clouds_01_Flow.jpg");
	//unsigned int _FlowTex1 = loadTexture("resources/plane/flowmap_thumb2.png");
	unsigned int _CloudTex2 = loadTexture("resources/plane/Clouds_02.jpg");
	unsigned int _WaveTex = loadTexture("resources/plane/Wave_Dist 1.jpg");
	unsigned int _ColorTex = loadTexture("resources/plane/UpperColor.jpg");

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

	// Glass shader
	unsigned int _normalMap = loadTexture("resources/bottle/bottleexport_baked_material_normal.jpeg");
	unsigned int _diffuseMap = loadTexture("resources/bottle/bottleexport_baked_material_diffuse.jpg");

	refractiveShader.use();
	refractiveShader.setInt("diffuseMap", 0);
	refractiveShader.setInt("normalMap", 1);
	refractiveShader.setInt("refractionMap", 2);
	refractiveShader.setInt("environmentMap", 3);
	
	// size of collision ellipse, experiment with this to change fidelity of detection
	//static glm::vec3 boundingEllipse = { 0.5f, 1.0f, 0.5f };

	entity = new CharacterEntity();

	// initialize player infront of model
	entity->position[1] = 7.0f;
	entity->position[2] = 4.0f;

	camera.setEntity(entity);

	entity->triangles = getTriangles(models, *entity->collisionPackage);

	glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	// framebuffer configuration
	// -------------------------
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
	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
	//unsigned int rbo2;
	//glGenRenderbuffers(1, &rbo2);
	//glBindRenderbuffer(GL_RENDERBUFFER, rbo2);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo2); // now actually attach it
	//// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	//	cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;

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

		entity->update();
		camera.Position = entity->position;
		entity->velocity = entity->velocity * .3f;

		// rendering commands here
		glClearColor(25.0f/255.0f, 25.0f/ 255.0f, 112.0f/ 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

		// use camera class to get projection and view matrices
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);

		cloudsShader.use();
		cloudsShader.setMat4("projection", projection);
		cloudsShader.setMat4("view", view);
		cloudsShader.setVec3("viewPos", camera.Position);

		glDepthMask(GL_FALSE);

		// render the plane
		model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(0.0f, 30.0f, 0.0f)); // translate it down so it's at the center of the scene
		//model = glm::scale(model, glm::vec3(10000.0f, 10000.0f, 10000.0f));	// it's a bit too big for our scene, so scale it down
		model = glm::translate(model, glm::vec3(0.0f, -25.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(100.0f, 100.0f, 100.0f));	// it's a bit too big for our scene, so scale it down

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
		lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
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
		// spotLight
		//lightingShader.setVec3("spotLight.position", camera.Position);
		//lightingShader.setVec3("spotLight.direction", camera.Front);
		//lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		//lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		//lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		//lightingShader.setFloat("spotLight.constant", 1.0f);
		//lightingShader.setFloat("spotLight.linear", 0.09);
		//lightingShader.setFloat("spotLight.quadratic", 0.032);
		//lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		//lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);


		// pass transformation matrices to the shader
		model = glm::mat4(1.0f);
		lightingShader.setMat4("projection", projection); // note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
		lightingShader.setMat4("view", view);
		lightingShader.setMat4("model", model);

		// render the plane
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(10000.0f, 10000.0f, 10000.0f));	// it's a bit too big for our scene, so scale it down
		lightingShader.setMat4("model", model);
		//ourPlane2.Draw2(lightingShader);

		// render the hitbox
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f)); // translate it down so it's at the center of the scene
		//model = glm::scale(model, glm::vec3(1.0, 1.0, 1.0));	// it's a bit too big for our scene, so scale it down
		model = glm::scale(model, glm::vec3(2, 2, 2));	// it's a bit too big for our scene, so scale it down
		//model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));

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

		// render the plane
		// TESTING: COLLISION
		//model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
		//model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
		//lightingShader.setMat4("model", model);
		//ourPlane2.Draw2(lightingShader);

		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);

		// render the ship
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f)); // translate it down so it's at the center of the scene
		//model = glm::scale(model, glm::vec3(1.0, 1.0, 1.0));	// it's a bit too big for our scene, so scale it down
		model = glm::scale(model, glm::vec3(0.02, 0.02, 0.02));	// it's a bit too big for our scene, so scale it down

		//entity->triangles = getTriangles(models, *entity->collisionPackage);

		//for (int i = 0; i < entity->triangles.size(); i++) {
		//	std::vector<glm::vec3> triangle = entity->triangles[i];
		//	glm::vec4 first = glm::vec4(triangle[0], 1);
		//	glm::vec4 second = glm::vec4(triangle[1], 1);
		//	glm::vec4 third = glm::vec4(triangle[2], 1);
		//	first = model * first;
		//	second = model * second;
		//	third = model * third;
		//	triangle = { glm::vec3(first), glm::vec3(second), glm::vec3(third) };
		//	entity->triangles[i] = triangle;
		//}

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

		// also draw the lamp object(s)
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", view);

		// we now draw as many light bulbs as we have point lights.
		glBindVertexArray(lightCubeVAO);
		for (unsigned int i = 0; i < 4; i++)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
			lightCubeShader.setMat4("model", model);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		create_refraction_map(ourBottle, refractiveMaskShader, bottle_translate, bottle_scale);

		// Swap framebuffer textures
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
		
		// Clear the buffer for drawing the bottle
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		render_glass(ourBottle, refractiveShader, maskBuffer, _diffuseMap, _normalMap, bottle_translate, bottle_scale);

		// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		// clear all relevant buffers
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
		glClear(GL_COLOR_BUFFER_BIT);

		screenShader.use();
		glBindVertexArray(quadVAO);
		glBindTexture(GL_TEXTURE_2D, maskBuffer);	// use the color attachment texture as the texture of the quad plane
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTexture(GL_TEXTURE_2D, colorTexture);	// use the color attachment texture as the texture of the quad plane
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// de-allocate all resources
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightCubeVAO);
	glDeleteBuffers(1, &VBO);

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

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
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

std::vector<std::vector<glm::vec3>> getTriangles(std::vector<Model> models, CollisionPackage &collisionPackage) {
	std::vector<std::vector<glm::vec3>> triangles;

	for (Model model : models) {
		for (Mesh mesh : model.meshes) {
			for (int i = 0; i < mesh.indices.size(); i += 3) {
				int index1 = mesh.indices[i];
				int index2 = mesh.indices[i + 1];
				int index3 = mesh.indices[i + 2];

				glm::vec3 first = mesh.vertices[index1].Position / collisionPackage.eRadius;
				glm::vec3 second = mesh.vertices[index2].Position / collisionPackage.eRadius;
				glm::vec3 third = mesh.vertices[index3].Position / collisionPackage.eRadius;

				std::vector<glm::vec3> triangle = { first, second, third };
			
				triangles.push_back(triangle);
			}
		}
	}

	return triangles;
}

void create_refraction_map(Model& refractiveObject, Shader& refractiveShader, glm::vec3 translate, glm::vec3 scale) {
	// Write to the alpha channel
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	// Disable writing to depth buffer
	glDepthMask(GL_FALSE);

	// Set up shader
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
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

	// Reset alpha values of framebuffer
	//glClearColor(0, 0, 0, 1);

	// Reset frame buffer write mode
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Reset writing to depth buffer
	glDepthMask(GL_TRUE);
}

void render_glass(
	Model& refractiveObject, 
	Shader& refractiveShader, 
	unsigned int& maskBuffer,
	unsigned int& diffuseMap,
	unsigned int& normalMap,
	glm::vec3 translate, 
	glm::vec3 scale
) {
	// Render refractive geometry
	refractiveShader.use();

	// Set up shader
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
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

	// What's around the object
	// Environment map

	// Set diffuse map
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(glGetUniformLocation(refractiveShader.ID, "diffuseMap"), 1);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);

	// Set normal map
	glActiveTexture(GL_TEXTURE2);
	glUniform1i(glGetUniformLocation(refractiveShader.ID, "normalMap"), 2);
	glBindTexture(GL_TEXTURE_2D, normalMap);

	// Draw refractive object
	refractiveObject.Draw2(refractiveShader);
}
