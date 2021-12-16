#pragma once
#ifndef WATERSHADER_H
#define WATERSHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <PirateShip/shader_m.h>
#include <PirateShip/texture.h>

class WaterShader
{
public:
	unsigned int _CloudTex1 = loadTexture("resources/plane/Clouds_01.jpg");
	unsigned int _CloudTex2 = loadTexture("resources/plane/Waves.png");
	unsigned int _FlowTex1 = loadTexture("resources/plane/Clouds_01_Flow.jpg");
	unsigned int _ColorWaveTex = loadTexture("resources/plane/Waves_Color.jpg");

	void setWaterShader(Shader& waterShader) 
	{
		waterShader.use();

		// Textures
		waterShader.setInt("_CloudTex1", 0);
		waterShader.setInt("_FlowTex1", 1);
		waterShader.setInt("_CloudTex2", 2);
		waterShader.setInt("_WaveTex", 3);
		waterShader.setInt("_ColorTex", 4);

		// Tiling scales and speeds
		waterShader.setVec4("_Tiling1", glm::vec4(0.1, 0.1, 0, 1));
		waterShader.setVec4("_Tiling2", glm::vec4(4, 4, 0, 0));
		waterShader.setVec4("_TilingWave", glm::vec4(0.1, 0.1, 0, 5));

		// Cloud height scale and offset
		waterShader.setFloat("_CloudScale", 1.0f);
		waterShader.setFloat("_CloudBias", 0.0f);

		// How much the Cloud 2 texture is mixed in
		waterShader.setFloat("_Cloud2Amount", 2.0f);

		// Wave distort height
		waterShader.setFloat("_WaveAmount", 0.6f);
		// Wave distort intensity
		waterShader.setFloat("_WaveDistort", 0.05f);

		// Flow settings
		waterShader.setFloat("_FlowSpeed", -3.0f);
		waterShader.setFloat("_FlowAmount", 1.0f);

		// Colors
		waterShader.setVec4("_TilingColor", glm::vec4(0.05f, 0.05f, 0.0f, 1.0f));
		waterShader.setVec4("_Color", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

		// Scale cloud height with density factor
		waterShader.setFloat("_CloudDensity", 7.0f);

		// How low down clouds are in sky
		waterShader.setFloat("_CloudHeight", 500.0f);
		// Cloud size
		waterShader.setFloat("_Scale", 10000.0f);
		// Cloud movement speed
		waterShader.setFloat("_Speed", 0.01f);

		// Color factors
		waterShader.setFloat("_ColPow", 5.0f);
		waterShader.setFloat("_ColFactor", 20.0f);
	}
	
	void bindWaterTextures(Shader& waterShader) {
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(waterShader.ID, "_CloudTex1"), 0);
		glBindTexture(GL_TEXTURE_2D, _CloudTex1);

		glActiveTexture(GL_TEXTURE1);
		glUniform1i(glGetUniformLocation(waterShader.ID, "_FlowTex1"), 1);
		glBindTexture(GL_TEXTURE_2D, _FlowTex1);

		glActiveTexture(GL_TEXTURE2);
		glUniform1i(glGetUniformLocation(waterShader.ID, "_CloudTex2"), 2);
		glBindTexture(GL_TEXTURE_2D, _CloudTex2);

		glActiveTexture(GL_TEXTURE4);
		glUniform1i(glGetUniformLocation(waterShader.ID, "_ColorTex"), 4);
		glBindTexture(GL_TEXTURE_2D, _ColorWaveTex);
	}
};
#endif
