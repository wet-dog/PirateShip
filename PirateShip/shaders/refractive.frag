#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec2 BaseUV;
    vec4 ScreenPos;
    vec3 Eye;
    float R;
} fs_in;

struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

float fresnel(vec3 light, vec3 normal, float R0);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

in vec3 aNormal;
in vec3 CameraPos;
in vec3 FragPos;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D refractionMap;
uniform sampler2D specularMap;

uniform DirLight dirLight;

void main() {
    vec3 vEye = normalize(fs_in.Eye.xyz);

    // Get bump and apply scale, then get diffuse
    vec4 vBumpTex = 2.0 * texture(normalMap, fs_in.BaseUV.xy) - 1.0;
    vec3 vBump = normalize(vBumpTex.xyz * vec3(0.2, 0.2, 1.0));
    vec4 vDiffuse = texture(diffuseMap, fs_in.BaseUV.xy);

    // Compute reflection vector
    float LdotN = dot(vBump.xyz, vEye.xyz);
    vec3 vReflect = 2.0 * LdotN * vBump.xyz - vEye;

    // Compute projected coordinates and add perturbation
    vec2 vProj = fs_in.ScreenPos.xy / fs_in.ScreenPos.w;
    vProj.y = 1.0f - vProj.y;
    vec4 vRefrA = texture(refractionMap, vProj.xy + vBump.xy);
    vec4 vRefrB = texture(refractionMap, vProj.xy);

    // Mask occluders from refraction map
    vec4 vFinal = vRefrA * (1- vRefrA.w) + vRefrB * (vRefrA.w);

    // Lerp between 1 and diffuse for glass transparency
    vDiffuse.xyz = clamp(0.1 + vDiffuse.xyz * 0.9, 0.0, 1.0);

    // Get specular highlights
    vec3 norm = normalize(aNormal);
    vec3 viewDir = normalize(CameraPos - FragPos);
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // Mix between diffuse and refraction texture based on Fresnel term
    FragColor = mix(vDiffuse * vec4(vFinal.xyz, 1), vec4(1), fs_in.R) + vec4(result, 1);
}

// Calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    float shininess = 50.0f;
    vec3 lightDir = normalize(-light.direction);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * vec3(texture(specularMap, fs_in.BaseUV));
    return specular;
}
