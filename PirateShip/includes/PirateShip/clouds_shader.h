#pragma once
#ifndef CLOUDSSHADER_H
#define CLOUDSSHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <PirateShip/shader_m.h>
#include <PirateShip/texture.h>

class CloudsShader 
{
public:
	unsigned int _CloudTex1 = loadTexture("resources/plane/Clouds_01.jpg");
	unsigned int _FlowTex1 = loadTexture("resources/plane/Clouds_01_Flow.jpg");
	unsigned int _CloudTex2 = loadTexture("resources/plane/Clouds_02.jpg");
	unsigned int _WaveTex = loadTexture("resources/plane/Wave_Dist691.jpg");
	unsigned int _ColorTex = loadTexture("resources/plane/UpperColor.jpg");

	void setCloudsShader(Shader& cloudsShader)
	{
		cloudsShader.use();

		// Textures
		cloudsShader.setInt("_CloudTex1", 0);
		cloudsShader.setInt("_FlowTex1", 1);
		cloudsShader.setInt("_CloudTex2", 2);
		cloudsShader.setInt("_WaveTex", 3);
		cloudsShader.setInt("_ColorTex", 4);

		// Tiling scales and speeds
		cloudsShader.setVec4("_Tiling1", glm::vec4(0.1, 0.1, 0, 1));
		cloudsShader.setVec4("_Tiling2", glm::vec4(4, 4, 0, 0));
		cloudsShader.setVec4("_TilingWave", glm::vec4(0.1, 0.1, 0, 5));

		// Cloud height scale and offset
		cloudsShader.setFloat("_CloudScale", 1.0f);
		cloudsShader.setFloat("_CloudBias", 0.0f);

		// How much the Cloud 2 texture is mixed in
		cloudsShader.setFloat("_Cloud2Amount", 2.0f);

		// Wave distort height
		cloudsShader.setFloat("_WaveAmount", 0.6f);
		// Wave distort intensity
		cloudsShader.setFloat("_WaveDistort", 0.05f);

		// Flow settings
		cloudsShader.setFloat("_FlowSpeed", -3.0f);
		cloudsShader.setFloat("_FlowAmount", 1.0f);

		// Colors
		cloudsShader.setVec4("_TilingColor", glm::vec4(0.05f, 0.05f, 0.0f, 1.0f));
		cloudsShader.setVec4("_Color", glm::vec4(0.9495942f, 0.4779412f, 1.0f, 1.0f));
		cloudsShader.setVec4("_Color2", glm::vec4(0.3868124f, 0.3822448f, 0.5147059f, 1.0f));

		// Scale cloud height with density factor
		cloudsShader.setFloat("_CloudDensity", 7.0f);

		// How low down clouds are in sky
		cloudsShader.setFloat("_CloudHeight", 500.0f);
		// Cloud size
		cloudsShader.setFloat("_Scale", 0.5f);
		// Cloud movement speed
		cloudsShader.setFloat("_Speed", 0.01f);

		// Color factors
		cloudsShader.setFloat("_ColPow", 5.0f);
		cloudsShader.setFloat("_ColFactor", 20.0f);

		cloudsShader.setFloat("_BumpOffset", 1.0f);
		cloudsShader.setFloat("_Steps", 70.0f);
	}

	void bindCloudsTextures(Shader& cloudsShader)
	{
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
	}
};
#endif
