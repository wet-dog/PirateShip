#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec2 BaseUV;
    vec4 ScreenPos;
    vec3 Eye;
    float R;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D refractionMap;
uniform sampler2D environmentMap;

float fresnel(vec3 light, vec3 normal, float R0);

void main() {
    vec3 vEye = normalize(fs_in.Eye.xyz);

    // Get bump and apply scale, then get diffuse
    vec4 vBumpTex = 2.0 * texture(normalMap, fs_in.BaseUV.xy) - 1.0;
    vec3 vBump = normalize(vBumpTex.xyz * vec3(0.2, 0.2, 1.0));
    vec4 vDiffuse = texture(diffuseMap, fs_in.BaseUV.xy);

    // Compute reflection vector
    float LdotN = dot(vBump.xyz, vEye.xyz);
    vec3 vReflect = 2.0 * LdotN * vBump.xyz - vEye;

    // Reflection vector coordinates used for environmental mapping
//    vec4 vEnvMap = texture(refractionMap, (vReflect.xy + 1.0) * 0.5);

    // Compute projected coordinates and add perturbation
    vec2 vProj = (fs_in.ScreenPos.xy/fs_in.ScreenPos.w);
    vProj.y = 1.0f - vProj.y;
    vec4 vRefrA = texture(refractionMap, vProj.xy + vBump.xy);
    vec4 vRefrB = texture(refractionMap, vProj.xy);

    // Mask occluders from refraction map
    vec4 vFinal = vRefrA * (1- vRefrA.w) + vRefrB * (vRefrA.w);

    // Compute Fresnel term
    // Change fresnel() params?
    float fresnel = fresnel(vReflect, vBump, 0.01);

    // Lerp between 1 and diffuse for glass transparency
    vDiffuse.xyz = clamp(0.1 + vDiffuse.xyz * 0.9, 0.0, 1.0);

    // Final output blends reflection and refraction using Fresnel term
//    FragColor = vDiffuse * vFinal * (1 - fresnel) + vEnvMap * fresnel;
//    FragColor = vDiffuse * vFinal * (1 - fresnel);
    
//    FragColor = texture(diffuseMap, fs_in.BaseUV.xy);
//    FragColor = texture(refractionMap, fs_in.BaseUV.xy);
//    FragColor = vRefrA;
//    FragColor = vDiffuse * vec4(vFinal.xyz, 1);
    FragColor = mix(vDiffuse * vec4(vFinal.xyz, 1), vec4(1,1,1,1), fs_in.R);

//    FragColor = vec4(vFinal.xyz, 1);
//    FragColor = vec4(vFinal.a, vFinal.a, vFinal.a, 1);

//    FragColor = texture(refractionMap, gl_FragCoord.xy);
//    FragColor.a = 1;
}

float fresnel(vec3 light, vec3 normal, float R0)
{
    float cosAngle = 1 - clamp(dot(light, normal), 0.0, 1.0);
    float result = cosAngle * cosAngle;
    result = result * result;
    result = result * cosAngle;
    result = clamp(result * (1 - clamp(R0, 0.0, 1.0)) +  R0, 0.0, 1.0);
 
    return result;
}
