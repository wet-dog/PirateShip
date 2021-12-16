#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

/* ADDED */
uniform sampler2D _CloudTex1;
uniform sampler2D _FlowTex1;
uniform sampler2D _CloudTex2;
uniform sampler2D _WaveTex;

uniform vec4 _Tiling1;
uniform vec4 _Tiling2;
uniform vec4 _TilingWave;

uniform float _CloudScale;
uniform float _CloudBias;

uniform float _Cloud2Amount;
uniform float _WaveAmount;
uniform float _WaveDistort;
uniform float _FlowSpeed;
uniform float _FlowAmount;

uniform sampler2D _ColorTex;
uniform vec4 _TilingColor;

uniform vec4 _Color;
uniform vec4 _Color2;

uniform float _CloudDensity;

uniform float _BumpOffset;
uniform float _Steps;

uniform float _CloudHeight;
uniform float _Scale;
uniform float _Speed;

uniform float _ColPow;
uniform float _ColFactor;

// Me adding specifically
uniform float _Time;

struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight dirLight;

/* ADDED */

uniform vec3 lightPos;
uniform vec3 viewPos;

vec4 SampleClouds(vec3 uv, vec3 sunTrans, float densityAdd);
float rand3(vec3 co);
vec4 newfunc();
vec4 FogColorDensitySky(vec3 worldPos);
float CalcFogFalloff( vec3 viewDir );

void main()
{
	vec3 uv = vec3(fs_in.TexCoords * _Scale / 2, 0);

	vec4 clouds = SampleClouds(uv, vec3(0.5, 0.5, 0.5), 1.0 );
	
	// return the color!
	// Fog parameters, could make them uniforms and pass them into the fragment shader
	float fog_density = 0.02f;
//	vec4  fog_colour = vec4(25.0f/255.0f, 25.0f/ 255.0f, 112.0f/ 255.0f, 1.0f);
	vec4  fog_colour = vec4(17.0f/255.0f, 17.0f/ 255.0f, 77.0f/ 255.0f, 1.0f);

	// Calculate fog
	float dist = length(.9f * fs_in.FragPos.xz);
	float fog_factor = exp(-pow(fog_density * dist, 2.0));
	fog_factor = 1.0 - clamp(fog_factor, 0.0, 1.0);

	FragColor = mix(clouds, fog_colour, fog_factor);
}

float rand3(vec3 co) {
	return fract(sin(dot(co.xyz ,vec3(17.2486,32.76149, 368.71564))) * 32168.47512);
}

vec4 SampleClouds (vec3 uv, vec3 sunTrans, float densityAdd) {

	// wave distortion
	vec3 coordsWave = vec3( uv.xy * _TilingWave.xy + ( _TilingWave.zw * _Speed * _Time ), 0.0 );
	vec3 wave = texture( _WaveTex, coordsWave.xy).xyz;

	// first cloud layer
	vec2 coords1 = uv.xy * _Tiling1.xy + ( _Tiling1.zw * _Speed * _Time ) + ( wave.xy - 0.5 ) * _WaveDistort;
	vec4 clouds = texture( _ColorTex, coords1.xy);
	
	vec3 cloudsFlow = texture( _FlowTex1, coords1.xy).xyz;

	// set up time for second clouds layer
	float speed = _FlowSpeed * _Speed * 10;
	float timeFrac1 = fract( _Time * speed );
	float timeFrac2 = fract( _Time * speed + 0.5 );
	float timeLerp  = abs( timeFrac1 * 2.0 - 1.0 );
	timeFrac1 = ( timeFrac1 - 0.5 ) * _FlowAmount;
	timeFrac2 = ( timeFrac2 - 0.5 ) * _FlowAmount;

	// second cloud layer uses flow map
	vec2 coords2 = coords1 * _Tiling2.xy + ( _Tiling2.zw * _Speed * _Time );
	vec4 clouds2 = texture( _CloudTex2, coords2.xy + ( cloudsFlow.xy - 0.5 ) * timeFrac1 );
	vec4 clouds2b = texture( _CloudTex2, coords2.xy + ( cloudsFlow.xy - 0.5 ) * timeFrac2 + 0.5 );
	clouds2 = mix( clouds2, clouds2b, timeLerp);
	clouds += ( clouds2 - 0.5 ) * _Cloud2Amount * cloudsFlow.z;

	// add wave to cloud height
	clouds.w += ( wave.z - 0.5 ) * _WaveAmount * 100.0f;

	// scale and bias clouds because we are adding lots of stuff together
	// and the values cound go outside 0-1 range
	clouds.w = clouds.w * _CloudScale + _CloudBias;

//	return clouds;

	// overhead light color
//	vec3 coords4 = vec3( uv.xy * _TilingColor.xy + ( _TilingColor.zw * _Speed * _Time ), 0.0 );
//	vec4 cloudColor = texture( _ColorTex, coords4.xy );
//
	// cloud color based on density
//	float cloudHightMask = clamp(clouds.w, 0.0, 1.0);;
//	cloudHightMask = pow( cloudHightMask, _ColPow );
//	clouds.xyz *= mix( _Color2.xyz, _Color.xyz * cloudColor.xyz * _ColFactor, cloudHightMask );

	// subtract alpha based on height
	float cloudSub = 1.0 - uv.z;
	clouds.w = clouds.w - cloudSub * cloudSub;

	// multiply density
	clouds.w = clamp(clouds.w * _CloudDensity, 0.0, 1.0);

	// add extra density
	clouds.w = clamp(clouds.w + densityAdd, 0.0, 1.0);

	// add Sunlight
//	clouds.xyz += sunTrans * cloudHightMask;

	// premultiply alpha
	clouds.xyz *= clouds.w;

    return clouds;
}
