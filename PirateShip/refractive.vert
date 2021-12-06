#version 330 core
layout (location = 0) in vec3 Pos;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;
layout (location = 3) in vec3 Tangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec4 vTranslation = vec4(0);
uniform vec3 vCameraPos;

out VS_OUT {
    vec2 BaseUV;
    vec4 ScreenPos;
    vec3 Eye;
    float R;
} vs_out;

mat3 GetTangentSpaceBasis(vec3 T, vec3 N)
{
   mat3 objToTangentSpace;
   
   objToTangentSpace[0] = T;           // tangent
   objToTangentSpace[1] = cross(T, N); // binormal
   objToTangentSpace[2] = N;           // normal  
   
   return objToTangentSpace;
}

void main() {
    mat4 MVP = projection * view * model;
    
    vec4 vPos = vec4(Pos, 1) + vTranslation;
    vs_out.BaseUV.xy = TexCoords.xy;
          
    // perspective corrected projection      
    vec4 vHPos = MVP * vPos;         	
    		  
	vHPos.y = -vHPos.y;
    vs_out.ScreenPos.xy = (vHPos.xy + vHPos.w)*0.5;    
    vs_out.ScreenPos.zw = vec2(1, vHPos.w);      
        
    // get tangent space basis    
    mat3 objToTangentSpace = GetTangentSpaceBasis(Tangent.xyz, Normal.xyz);
            
    vec3 EyeVec = vCameraPos.xyz - vPos.xyz;
    vec3 LightVec = vCameraPos.xyz - vPos.xyz;
    
    vs_out.Eye.xyz = objToTangentSpace * EyeVec;

    // Fresnel R value
    vec3 posWorld = (model *  vHPos).xyz;
	vec3 normWorld = normalize((model * vec4(Normal, 1)).xyz);

	vec3 I = normalize(posWorld - vCameraPos);
    float _Bias = 0.1;
    float _Scale = 0.01;
    float _Power = 0.05;
	vs_out.R = _Bias + _Scale * pow(1.0 + dot(I, normWorld), _Power);

//    gl_Position = vec4(vs_out.Eye.xyz, 0);
    gl_Position = MVP * vec4(Pos, 1);
}
