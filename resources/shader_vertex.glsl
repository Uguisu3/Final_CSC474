#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout (location = 3) in vec4 InstancePos;
//layout (location = 4) in vec4 InstanceScale;


uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 color;
out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;
void main()
{
	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;
	mat4 S = mat4(1);
	S[0][0]= InstancePos[3];
	S[1][1]= InstancePos[3];
	S[2][2]= InstancePos[3];
	vec4 inst = InstancePos;
	inst[3] = 0;
	vec4 pos = M * S * (vec4(vertPos,1.0)) + inst;
	gl_Position = P * V * pos;
	vertex_tex = vertTex;	
}
