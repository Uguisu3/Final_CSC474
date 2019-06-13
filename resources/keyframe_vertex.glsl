#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in int vertimat;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 vertex_pos;
uniform mat4 Manim[200];
void main()
{
	mat4 Ma = Manim[vertimat];
	vec4 pos = vec4(Ma[3][0],Ma[3][1],Ma[3][2],1);
	gl_Position = P * V * M * pos;

}
