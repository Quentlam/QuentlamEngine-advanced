#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in float a_TexIndex;
layout(location = 5) in float a_TilingFactor;
layout(location = 6) in int a_EntityID;
layout(location = 7) in float a_AmbientStrength;
layout(location = 8) in float a_DiffuseStrength;
layout(location = 9) in float a_SpecularStrength;
layout(location = 10) in float a_Shininess;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;
flat out int v_EntityID;

out float v_AmbientStrength;
out float v_DiffuseStrength;
out float v_SpecularStrength;
out float v_Shininess;

uniform mat4 u_ViewProjection;

void main()
{
	v_FragPos = a_Position; 
	v_Normal = a_Normal;
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_TilingFactor = a_TilingFactor;
	v_EntityID = a_EntityID;
	
	v_AmbientStrength = a_AmbientStrength;
	v_DiffuseStrength = a_DiffuseStrength;
	v_SpecularStrength = a_SpecularStrength;
	v_Shininess = a_Shininess;
	
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;		
layout(location = 1) out int color2;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;
flat in int v_EntityID;

in float v_AmbientStrength;
in float v_DiffuseStrength;
in float v_SpecularStrength;
in float v_Shininess;

uniform sampler2D u_Textures[32];

uniform vec3 u_ViewPos;
uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

void main()
{
	vec4 texColor = v_Color;
	switch(int(v_TexIndex))
	{
	case 0:texColor *= texture(u_Textures [0 ],v_TexCoord * v_TilingFactor); break;
	case 1:texColor *= texture(u_Textures [1 ],v_TexCoord * v_TilingFactor); break;
	case 2:texColor *= texture(u_Textures [2 ],v_TexCoord * v_TilingFactor); break;
	case 3:texColor *= texture(u_Textures [3 ],v_TexCoord * v_TilingFactor); break;
	case 4:texColor *= texture(u_Textures [4 ],v_TexCoord * v_TilingFactor); break;
	case 5:texColor *= texture(u_Textures [5 ],v_TexCoord * v_TilingFactor); break;
	case 6:texColor *= texture(u_Textures [6 ],v_TexCoord * v_TilingFactor); break;
	case 7:texColor *= texture(u_Textures [7 ],v_TexCoord * v_TilingFactor); break;
	case 8:texColor *= texture(u_Textures [8 ],v_TexCoord * v_TilingFactor); break;
	case 9:texColor *= texture(u_Textures [9 ],v_TexCoord * v_TilingFactor); break;
	case 10:texColor *= texture(u_Textures[10],v_TexCoord * v_TilingFactor); break;
	case 11:texColor *= texture(u_Textures[11],v_TexCoord * v_TilingFactor); break;
	case 12:texColor *= texture(u_Textures[12],v_TexCoord * v_TilingFactor); break;
	case 13:texColor *= texture(u_Textures[13],v_TexCoord * v_TilingFactor); break;
	case 14:texColor *= texture(u_Textures[14],v_TexCoord * v_TilingFactor); break;
	case 15:texColor *= texture(u_Textures[15],v_TexCoord * v_TilingFactor); break;
	case 16:texColor *= texture(u_Textures[16],v_TexCoord * v_TilingFactor); break;
	case 17:texColor *= texture(u_Textures[17],v_TexCoord * v_TilingFactor); break;
	case 18:texColor *= texture(u_Textures[18],v_TexCoord * v_TilingFactor); break;
	case 19:texColor *= texture(u_Textures[19],v_TexCoord * v_TilingFactor); break;
	case 20:texColor *= texture(u_Textures[20],v_TexCoord * v_TilingFactor); break;
	case 21:texColor *= texture(u_Textures[21],v_TexCoord * v_TilingFactor); break;
	case 22:texColor *= texture(u_Textures[22],v_TexCoord * v_TilingFactor); break;
	case 23:texColor *= texture(u_Textures[23],v_TexCoord * v_TilingFactor); break;
	case 24:texColor *= texture(u_Textures[24],v_TexCoord * v_TilingFactor); break;
	case 25:texColor *= texture(u_Textures[25],v_TexCoord * v_TilingFactor); break;
	case 26:texColor *= texture(u_Textures[26],v_TexCoord * v_TilingFactor); break;
	case 27:texColor *= texture(u_Textures[27],v_TexCoord * v_TilingFactor); break;
	case 28:texColor *= texture(u_Textures[28],v_TexCoord * v_TilingFactor); break;
	case 29:texColor *= texture(u_Textures[29],v_TexCoord * v_TilingFactor); break;
	case 30:texColor *= texture(u_Textures[30],v_TexCoord * v_TilingFactor); break;
	case 31:texColor *= texture(u_Textures[31],v_TexCoord * v_TilingFactor); break;
	}

	if (texColor.a < 0.1) discard;

	vec3 lightDir = normalize(-u_LightDirection);
	vec3 lightColor = u_LightColor * u_LightIntensity;
	vec3 norm = normalize(v_Normal);

	// Ambient
	vec3 ambient = v_AmbientStrength * lightColor;

	// Diffuse
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = v_DiffuseStrength * diff * lightColor;

	// Specular
	vec3 viewDir = length(u_ViewPos) > 0.01 ? normalize(u_ViewPos - v_FragPos) : vec3(0.0, 0.0, 1.0);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), v_Shininess);
	vec3 specular = v_SpecularStrength * spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * texColor.rgb;

	color = vec4(result, texColor.a);
	color2 = v_EntityID;
}
