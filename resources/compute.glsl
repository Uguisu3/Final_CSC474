#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
layout(local_size_x = 1, local_size_y = 1) in;	
//local group of shaders
#define STARCOUNT 256
layout (std430, binding=0) volatile buffer shader_data
{ 
  vec4 star_position[STARCOUNT];
  vec4 star_speed[STARCOUNT];
  vec4 star_start_speed[STARCOUNT];
  vec4 star_height[STARCOUNT];
  vec4 star_start_pos[STARCOUNT];
  vec4 star_scale[STARCOUNT];
};

void main() 
{
	uint index = gl_GlobalInvocationID.x;
	vec3 pos = star_position[index].xyz;
	pos += star_speed[index].xyz;
	star_scale[index][0]= star_height[index][0] - pos[1];
	if(pos[1] > star_height[index][0])
	{
	    pos = star_start_pos[index].xyz;
	    star_speed[index].xyz = star_start_speed[index].xyz;
	}
	star_position[index] = vec4(pos,0);

}