/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include <time.h>
#include "WindowManager.h"
#include "Shape.h"
#include "bone.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "line.h"
using namespace std;
using namespace glm;
shared_ptr<Shape> shape, room, chair,table,needle,pic_frame,pic;
int loc = 0;
vector<vec3> ball_roll,smooth;
Line yarn_path;

#define STARCOUNT 256
class ssbo_data
	{
	public:
		vec4 position[STARCOUNT];
		vec4 speed[STARCOUNT];
		vec4 start_speed[STARCOUNT];
		vec4 height[STARCOUNT];
		vec4 start_pos[STARCOUNT];
		vec4 scale[STARCOUNT];
	};
float frand()
	{
	return (float)rand() / (float)RAND_MAX;
	}

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

class phys_obj
{
	public:
		vec3 pos;
		vec3 speed;
		float mass;
		float dampening = .5;
		bool first = true;
	void update(vec3 force, float dt)
	{
		vec3 a = force/mass;
		speed += a*dt;
		pos += speed*dt;

		if(pos[1] <= -.35 && pos[0] >= -1.2 && pos[2] >= -9.9 && pos[0] <= -.335 && pos[2] <= -9.07)
		{
			pos[1] = -.35;
			speed[1] = abs(speed[1]);
			speed[1] *=dampening;
			speed[0] *=.996;
			speed[2] *=.996;
		}
		else if (first)
		{
			ball_roll.push_back(vec3(pos[0],-.6,pos[2]));
			yarn_path.re_init_line(ball_roll);
			first = false;
		}
		if(pos[1] <= -1.32)
		{
			pos[1] = -1.32;
			speed[1] = abs(speed[1]);
			speed[1] *=dampening;
			speed[0] *=.996;
			speed[2] *=.996;
			ball_roll.push_back(vec3(pos[0],-2.5,pos[2]));
			yarn_path.re_init_line(ball_roll);

		}

	}
};

class cloth_points
{
	public:
		int number;
		vec3 pos;
		vec3 speed;
		float mass = 1;
		float dampening = .85;
		vector<cloth_points*> neighbors;
	void update(vec3 force, float dt)
	{
		for(int i = 0; i< neighbors.size(); i++)
		{
		    force += -(float)pow(distance(pos,neighbors[i]->pos)*8,1) * normalize(pos - neighbors[i]->pos);
		}
		vec3 a = force/mass;
		speed += a*dt;
		pos += speed*dt;
		speed *=dampening;

	}
	void init()
    {
        for(int i = 0; i< neighbors.size(); i++)
        {
            if(neighbors[i]->number == number-40)
            {
                pos = neighbors[i]->pos;
                pos[1] = neighbors[i]->pos[1] - .008;
            }
        }
    }
};
class camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		if (w == 1)
		{
			speed = 10*ftime;
		}
		else if (s == 1)
		{
			speed = -10*ftime;
		}
		float yangle=0;
		if (a == 1)
			yangle = -3*ftime;
		else if(d==1)
			yangle = 3*ftime;
		rot.y += yangle;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::vec4 dir = glm::vec4(0, 0, speed,1);
		dir = dir*R;
		pos += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R*T;
	}
};

camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog,psky,pplane,key;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexNormDBox, VertexTexBox, IndexBufferIDBox, InstanceBuffer,InstanceBuffer2,
			VertexBufferID2, VertexArrayID2,VertexBufferIDimat;

    Line linerender;
    Line smoothrender;
    Line draw_between;

    vector<vec3> card;


    vector<vec3> line;
	vector<vec3> between;
    vector<float> ang;
    int lod = 50;

	//texture data
	GLuint Texture;
	GLuint Texture2;

	ssbo_data stars;
	GLuint ssbo_stars;
	GLuint computeProgram;

	mat4 animmat[200];
	int animmatsize=0;

	bone *root = NULL;
	int size_stick = 0;
	all_animations all_animation;
	phys_obj yarn;
	cloth_points points[200];

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
		if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		{
			loc ++;
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		{
			loc --;
		}

	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

			//change this to be the points converted to WORLD
			//THIS IS BROKEN< YOU GET TO FIX IT - yay!
			newPt[0] = 0;
			newPt[1] = 0;

			std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
			//update the vertex array with the updated points
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}



	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		for (int ii = 1; ii < STARCOUNT; ii++)
		{
			stars.position[ii] = vec4(frand()*.8,frand()*.5,frand()*.8,1);
			stars.speed[ii] = vec4(frand()*.001,frand()*.03+.01,frand()*.001,1);
			stars.start_speed[ii] = stars.speed[ii];
			stars.height[ii] = vec4(frand()+1,0,0,0);
			stars.start_pos[ii] = stars.position[ii];
			stars.scale[ii] = vec4(1,1,1,1);
		}



		vector<vec3> meshpos;
		vector<unsigned int> meshindices;
		vector<ivec4> meshanimindices;
		vector<vec4> meshanimweights;

		for (int ii = 0; ii < 200; ii++)
			animmat[ii] = mat4(1);

		readtobone("../resources/Knitting_character.fbx",&all_animation,&root, &meshpos, &meshindices,&meshanimindices,&meshanimweights);
		root->set_animations(&all_animation,animmat,animmatsize);


		string resourceDirectory = "../resources";
        string mtl = resourceDirectory + "/";
		// Initialize mesh.
		shape = make_shared<Shape>();
		//shape->loadMesh(resourceDirectory + "/t800.obj");
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize();
		shape->init();

        room = make_shared<Shape>();
        room->loadMesh(resourceDirectory + "/room.obj",&mtl,stbi_load);
        room->resize();
        room->init();

        chair = make_shared<Shape>();
        chair->loadMesh(resourceDirectory + "/chair.obj",&mtl,stbi_load);
        chair->resize();
        chair->init();


		table = make_shared<Shape>();
		table->loadMesh(resourceDirectory + "/table.obj",&mtl,stbi_load);
		table->resize();
		table->init();

		needle = make_shared<Shape>();
		needle->loadMesh(resourceDirectory + "/knittingneedle.obj",&mtl,stbi_load);
		needle->resize();
		needle->init();

        pic_frame = make_shared<Shape>();
        pic_frame->loadMesh(resourceDirectory + "/picture_frame.obj",&mtl,stbi_load);
        pic_frame->resize();
        pic_frame->init();

        pic = make_shared<Shape>();
        pic->loadMesh(resourceDirectory + "/painting.obj",&mtl,stbi_load);
        pic->resize();
        pic->init();
		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);

		GLfloat cube_vertices[] = {
			// front
			-1.0, -1.0,  1.0,//LD
			1.0, -1.0,  1.0,//RD
			1.0,  1.0,  1.0,//RU
			-1.0,  1.0,  1.0,//LU
		};
		//make it a bit smaller
		for (int i = 0; i < 12; i++)
			cube_vertices[i] *= 0.5;
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_DYNAMIC_DRAW);

		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//color
		GLfloat cube_norm[] = {
			// front colors
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,

		};
		glGenBuffers(1, &VertexNormDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexNormDBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_norm), cube_norm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//color
		glm::vec2 cube_tex[] = {
			// front colors
			glm::vec2(0.0, 1.0),
			glm::vec2(1.0, 1.0),
			glm::vec2(1.0, 0.0),
			glm::vec2(0.0, 0.0),

		};
		glGenBuffers(1, &VertexTexBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexTexBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLushort cube_elements[] = {

			// front
			0, 1, 2,
			2, 3, 0,
		};
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);


		//generate vertex buffer to hand off to OGL ###########################
		glGenBuffers(1, &InstanceBuffer);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);
		glm::vec4 *positions = new glm::vec4[STARCOUNT];
		for (int i = 0; i<STARCOUNT; i++)
			positions[i] = stars.position[i];
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, STARCOUNT * sizeof(glm::vec4), positions, GL_STATIC_DRAW);
		int position_loc = glGetAttribLocation(prog->pid, "InstancePos");
		for (int i = 0; i < STARCOUNT; i++)
		{
			// Set up the vertex attribute
			glVertexAttribPointer(position_loc + i,              // Location
				4, GL_FLOAT, GL_FALSE,       // vec4
				sizeof(vec4),                // Stride
				(void *)(sizeof(vec4) * i)); // Start offset
											 // Enable it
			glEnableVertexAttribArray(position_loc + i);
			// Make it instanced
			glVertexAttribDivisor(position_loc + i, 1);
		}

		glBindVertexArray(0);



		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID2);
		glBindVertexArray(VertexArrayID2);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferID2);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID2);

		vector<vec3> pos;
		vector<unsigned int> imat;
		root->write_to_VBOs(vec3(0, 0, 0), pos, imat);
		size_stick = pos.size();
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*pos.size(), pos.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//indices of matrix:
		glGenBuffers(1, &VertexBufferIDimat);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDimat);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uint)*imat.size(), imat.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 0, (void*)0);
		glBindVertexArray(0);





		int width, height, channels;
		char filepath[1000];

		//texture 1
		string str = resourceDirectory + "/fire.png";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 2
		str = resourceDirectory + "/yarn.jpeg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);
		glUseProgram(pplane->pid);
		glUniform1i(Tex2Location, 0);
		//make an SSBO
	
		glGenBuffers(1, &ssbo_stars);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_stars);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &stars, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_stars);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind


        smoothrender.init();
        linerender.init();
        yarn_path.init();
        draw_between.init();
        ball_roll.push_back(vec3(0,0,0));
		ball_roll.push_back(vec3(0,0,0));
		ball_roll.push_back(vec3(0,0,0));
		between.push_back(vec3(0));
		between.push_back(vec3(0));
        line.push_back(vec3(-4,1,0));
        line.push_back(vec3(0,1,0));
        line.push_back(vec3(0,1,-5));
        line.push_back(vec3(1,1,-7));
        line.push_back(vec3(2,.5,-12.3));
		line.push_back(vec3(-2,.5,-11));
		line.push_back(vec3(-2,0,-9));
        ang.push_back(3.14159/2);
        ang.push_back(0);
        ang.push_back(-.5);
        ang.push_back(-3.14159/4.);
        ang.push_back(-3.14159/2.);
        ang.push_back(-3.14159*1.25);
		ang.push_back(-3.14159 *1.85);
        linerender.re_init_line(line);
        cardinal_curve(card,line,lod,1.0);
        smoothrender.re_init_line(card);



	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_DEPTH_TEST);
		// Initialize the GLSL program.
		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");
		prog->addAttribute("InstancePos");
		prog->addAttribute("InstanceScale");


		key = std::make_shared<Program>();
		key->setVerbose(true);
		key->setShaderNames(resourceDirectory + "/keyframe_vertex.glsl", resourceDirectory + "/keyframe_fragment.glsl");
		if (!key->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		key->addUniform("P");
		key->addUniform("V");
		key->addUniform("M");
		key->addUniform("campos");
		key->addUniform("Manim");
		key->addAttribute("vertPos");
		key->addAttribute("vertimat");

		psky = std::make_shared<Program>();
		psky->setVerbose(true);
		psky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!psky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		psky->addUniform("P");
		psky->addUniform("V");
		psky->addUniform("M");
		psky->addUniform("campos");
		psky->addAttribute("vertPos");
		psky->addAttribute("vertNor");
		psky->addAttribute("vertTex");

        pplane = std::make_shared<Program>();
        pplane->setVerbose(true);
        pplane->setShaderNames(resourceDirectory + "/plane_vertex.glsl", resourceDirectory + "/plane_frag.glsl");
        if (!pplane->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        pplane->addUniform("P");
        pplane->addUniform("V");
        pplane->addUniform("M");
        pplane->addUniform("campos");
        pplane->addAttribute("vertPos");
        pplane->addAttribute("vertNor");
        pplane->addAttribute("vertTex");


		//load the compute shader
		std::string ShaderString = readFileAsString("../resources/compute.glsl");
		const char *shader = ShaderString.c_str();
		GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(computeShader, 1, &shader, nullptr);

		GLint rc;
		CHECKED_GL_CALL(glCompileShader(computeShader));
		CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
		if (!rc)	//error compiling the shader file
			{
			GLSL::printShaderInfoLog(computeShader);
			std::cout << "Error compiling fragment shader " << std::endl;
			exit(1);
			}

		computeProgram = glCreateProgram();
		glAttachShader(computeProgram, computeShader);
		glLinkProgram(computeProgram);
		glUseProgram(computeProgram);
		
		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 2;
		glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);
		yarn.pos = vec3(-.8,-.35,-9.5);
		yarn.speed = vec3(.3,0,.3);
		yarn.dampening = .8;
		yarn.mass = 1;

		int width = 40;
	    for(int i = 0; i< 200; i++)
        {
	        points[i].pos = vec3((i%width) *.008,(i/width)*-.008,-3);
	        points[i].number = i;
        }
        for(int i = 0; i< 200; i++)
        {
            if(i%width != 0)
                points[i].neighbors.push_back(&points[i-1]);
            if(i%width != width-1)
                points[i].neighbors.push_back(&points[i+1]);
            if(i/width != 0)
                points[i].neighbors.push_back(&points[i-width]);
//            if(i/width != width-1)
//                points[i].neighbors.push_back(&points[i+width]);
            if(i%width != 0 && i/width != 0)
                points[i].neighbors.push_back(&points[i-(width+1)]);
//            if(i%width != 0 && i/width != width-1)
//                points[i].neighbors.push_back(&points[i+ (width-1)]);
            if(i%width != width-1 && i/width != 0)
                points[i].neighbors.push_back(&points[i-(width-1)]);
//            if(i%width != width-1 && i/width != width-1)
//                points[i].neighbors.push_back(&points[i+(width+1)]);
        }

	}
	void compute()
	{

		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_stars);
		glUseProgram(computeProgram);
		glDispatchCompute((GLuint)STARCOUNT, (GLuint)1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		//copy data back to instance buffer

		//stars.position[0] = vec4(3, 3, 3,3);
		//step 1 getting data back from GPU to the stars object
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_stars);
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		int siz = sizeof(ssbo_data);
		memcpy(&stars,p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		//step 2 copying to instance buffer
		glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);
		glm::vec4 *positions = new glm::vec4[STARCOUNT];
		for (int i = 0; i<STARCOUNT; i++)
		{
			positions[i] = stars.position[i];
			positions[i][3] = stars.scale[i][0]*.5;
		}


		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, STARCOUNT * sizeof(glm::vec4), positions, GL_STATIC_DRAW);

	}


	mat4 lookat(vec3 targetPosition, vec3 objectPosition)
	{
		glm::vec3 delta = targetPosition-objectPosition;
		glm::vec3 up;
		glm::vec3 direction(glm::normalize(delta));
		if(abs(direction.x)< 0.00001 && abs(direction.z) < 0.00001){
			if(direction.y > 0)
				up = glm::vec3(0.0, 0.0, -1.0); //if direction points in +y
			else
				up = glm::vec3(0.0, 0.0, 1.0); //if direction points in -y
		} else {
			up = glm::vec3(0.0, 1.0, 0.0); //y-axis is the general up
		}
		up=glm::normalize(up);
		glm::vec3 right = glm::normalize(glm::cross(up,direction));
		up= glm::normalize(glm::cross(direction, right));

		return glm::mat4(right.x, right.y, right.z, 0.0f,
						 up.x, up.y, up.z, 0.0f,
						 direction.x, direction.y, direction.z, 0.0f,
						 objectPosition.x, objectPosition.y, objectPosition.z, 1.0f);

	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render() {
		compute();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		double frametime = get_last_elapsed_time();
		static double totaltime_ms = 0;
		totaltime_ms += frametime * 1000.0;
		static double totaltime_untilframe_ms = 0;
		static bool first = true;
		totaltime_untilframe_ms += frametime * 1000.0;

		for (int ii = 0; ii < 200; ii++)
			animmat[ii] = mat4(1);

		//animation frame system
		int anim_step_width_ms = 8490 / 204;
		static int frame = 0;
		if (totaltime_untilframe_ms >= anim_step_width_ms) {
			totaltime_untilframe_ms = 0;
			frame++;
		}
		root->play_animation(frame, "john knitting_2Char00|john knitting_2Char00|john knitting_2Char");





		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float) height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		static bool move = true;

		static float counts = 1;
		if (move) {
			mycam.pos = -vec3(card[(int) counts].x * (1 - (counts - ((int) counts))) +
							  card[(int) counts + 1].x * (counts - ((int) counts)),
							  card[(int) counts].y * (1 - (counts - ((int) counts))) +
							  card[(int) counts + 1].y * (counts - ((int) counts)),
							  card[(int) counts].z * (1 - (counts - ((int) counts))) +
							  card[(int) counts + 1].z * (counts - ((int) counts)));
			mycam.rot = vec3(0, ang[(int) counts / lod] * (1 - (counts / lod - ((int) counts / lod))) +
								ang[(int) counts / lod + 1] * (counts / lod - ((int) counts / lod)), 0);
		}

		counts += frametime * 7;
		//cout << counts << endl;
		if ((int) counts >= card.size() - 1) {
			move = false;
		}
		if(counts > 40)
		{
			vec3 force = vec3(0, -9.81, 0);
			yarn.update(force, frametime);
		}
		if((int)counts%1 == 0 && ball_roll.size() > 0)
		{
			ball_roll[ball_roll.size()-1] = yarn.pos - vec3(0,.08,0);
			yarn_path.re_init_line(ball_roll);
		}


		// Create the matrix stacks - please leave these alone for now

		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = mycam.process(frametime);
		M = glm::mat4(1);

		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float) (3.14159 / 4.), (float) ((float) width / (float) height), 0.1f,
							 1000.0f); //so much type casting... GLM metods are quite funny ones


		//animation with the model matrix:
		static float w = 0.0;
		w += 1.0 * frametime;//rotation angle
		float trans = 0;// sin(t) * 2;
		glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3));
		glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));


		pplane->bind();

		glm::mat4 TransPlane = glm::translate(glm::mat4(1.0f), vec3(-.779,-0.52,-11.04));
//        cout << "up :"  << mycam.up << endl;
//        cout << "left :"  << mycam.left << endl;
		glm::mat4 SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(0.90f, 0.9f, 0.9f));
		float sangle = -3.1415926 / 2.;
		glm::mat4 RotateXPlane = glm::rotate(glm::mat4(1.0f), -.8f, vec3(0,1,0));
		M = TransPlane*RotateXPlane* SPlane;

		glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
		chair->draw(pplane, false);			//render!!!!!!!

		TransPlane = glm::translate(glm::mat4(1.0f), vec3(0,2,-6));
		SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f));
		sangle = -3.1415926 / 2.;
		RotateXPlane = glm::rotate(glm::mat4(1.0f), 0.0f, vec3(1,0,0));

		M = TransPlane*RotateXPlane* SPlane;
		glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
		room->draw(pplane, false);			//render!!!!!!!

		TransPlane = glm::translate(glm::mat4(1.0f), vec3(-.779,-0.95,-9.50));
		SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
		sangle = -3.1415926 / 2.;
		RotateXPlane = glm::rotate(glm::mat4(1.0f), 0.0f, vec3(1,0,0));

		M = TransPlane*RotateXPlane* SPlane;
		glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
		table->draw(pplane, false);			//render!!!!!!!

		TransPlane = glm::translate(glm::mat4(1.0f), yarn.pos);
		SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
		sangle = -3.1415926 / 2.;
		static float roll = 0;
		static float change = .1;
		roll+= change;
		if (change > 0)
			change -= .0001;

		RotateXPlane = glm::rotate(glm::mat4(1.0f), roll, vec3(1,0,0));
		mat4 RotY = glm::rotate(glm::mat4(1.0f), 3.14159f/4.0f, vec3(0,1,0));

		M = TransPlane*RotY * RotateXPlane* SPlane;
		glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		shape->draw(pplane, false);

        TransPlane = glm::translate(glm::mat4(1.0f), vec3(-4,2,-10));
        SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        sangle = -3.1415926 / 2.;
        RotateXPlane = glm::rotate(glm::mat4(1.0f), sangle, vec3(0,0,1));

        M = TransPlane*RotateXPlane* SPlane;
        glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
        pic_frame->draw(pplane, false);

        TransPlane = glm::translate(glm::mat4(1.0f), vec3(0,-.05,0));
        SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(0.77f, 0.77f, 0.77f));
        sangle = -3.1415926 / 2.;
        RotateXPlane = glm::rotate(glm::mat4(1.0f), sangle, vec3(0,0,1));

        M = M * TransPlane* SPlane;
        glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
        pic->draw(pplane, false);
        pplane->unbind();
		vec3 col = vec3(.8, .8, 1);
		for(int i = 0; i< 200; i++)
		{


            between[0] = points[i].pos;
            for (int x = 0; x < points[i].neighbors.size(); x++)
            {
                between[1] = points[i].neighbors[x]->pos;
                draw_between.re_init_line(between);


                draw_between.draw(P, V, col);
            }
            if(i <  40) {
                pplane->bind();
                SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(0.007f, 0.007f, 0.007f));
                TransPlane = glm::translate(glm::mat4(1.0f), points[i].pos);
                RotY = glm::rotate(glm::mat4(1.0f),3.14159f/2.f,vec3(0,1,0));
                M = TransPlane* RotY * SPlane;
                glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
                glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
                glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Texture2);
                shape->draw(pplane, false);
                pplane->unbind();
            }
        }



		key->bind();

		glUniformMatrix4fv(key->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(key->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(key->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(key->getUniform("campos"), 1, &mycam.pos[0]);
		glBindVertexArray(VertexArrayID2);
		//actually draw from vertex 0, 3 vertices
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
		mat4 Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);



		TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.82f, -12));
		S = glm::scale(glm::mat4(1.0f), glm::vec3(0.016f, 0.016f, 0.016f));
		glm::mat4 rot = glm::rotate(glm::mat4(1.0),-.3f,vec3(1,0,0));
		M = TransZ* rot * S;
		mat4 body = M;
		glUniformMatrix4fv(key->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniformMatrix4fv(key->getUniform("Manim"), 200, GL_FALSE, &animmat[0][0][0]);
		glDrawArrays(GL_LINES, 4, size_stick-4);
		//glDrawArrays(GL_POINTS, 4, size_stick-4);
		glBindVertexArray(0);

		key->unbind();
		pplane->bind();
        cout << loc << endl;
		vec3 location = vec3(animmat[22][3])+ vec3(animmat[40][3]);
		location = vec3(location[0]/2,location[1]/2,location[2]/2);
		TransPlane = glm::translate(glm::mat4(1.0f), location);
		SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f));
		sangle = -3.1415926 / 2.;
		mat4 RotateYPlane = glm::rotate(glm::mat4(1.0f), -3.14159f/2.0f -.5f, vec3(0,1,0));
		RotateXPlane = glm::rotate(glm::mat4(1.0f), 3.14159f/10.0f, vec3(1,0,0));

		M = body * lookat(animmat[44][3],location)* RotateXPlane* RotateYPlane* SPlane;
		glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
		needle->draw(pplane, false);			//render!!!!!!
        for(int i= 0; i<20; i++)
        {
            points[i].pos =vec3( M * vec4(i*+.08-0.8,0,0,1));
            //cout << points[i].pos[1] << endl;
        }

		location = vec3(animmat[49][3])+ vec3(animmat[69][3]);
		location = vec3(location[0]/2,location[1]/2,location[2]/2);
		TransPlane = glm::translate(glm::mat4(1.0f), vec3(animmat[63+loc][3]));
		//cout<< loc<< endl;
		SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f));
		sangle = -3.1415926 / 2.;
		RotateYPlane = glm::rotate(glm::mat4(1.0f), -3.14159f/2.0f + .1f, vec3(0,1,0));
		RotateXPlane = glm::rotate(glm::mat4(1.0f), 3.14159f/10.0f, vec3(1,0,0));

		M = body *lookat(animmat[49][3],location) * RotateXPlane* RotateYPlane* SPlane;
		glUniformMatrix4fv(pplane->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(pplane->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(pplane->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(pplane->getUniform("campos"), 1, &mycam.pos[0]);
		needle->draw(pplane, false);			//render!!!!!!!
        for(int i= 20; i<40; i++)
        {
            points[i].pos =vec3( M * vec4(i*-.08+2.35,0,0,1));
        }

        if(counts > 2)
        {
            for (int i = 40; i < 200; i++) {
                vec3 force = vec3(0, -.6, 0);
                points[i].update(force, frametime);
            }
        }
        else
        {
            for (int i = 40; i < 200 && first; i++) {
                points[i].init();
            }
        }


        pplane->unbind();
		
		// Draw the box using GLSL.
		prog->bind();
//send the matrices to the shaders


		glDepthMask(GL_FALSE);
		//send the matrices to the shaders
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
	
		glBindVertexArray(VertexArrayID);
		//actually draw from vertex 0, 3 vertices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
		Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, -1.2f, -10.5));
		static float agle = 0;
		agle +=.01;
		M = TransZ* S* Vi;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		
		//glDisable(GL_DEPTH_TEST);
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0, STARCOUNT);
		//glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);

		glDepthMask(GL_TRUE);
		prog->unbind();

		ball_roll[0] = body * animmat[27][3];

		ball_roll[1] = body * animmat[54][3];
		yarn_path.re_init_line(ball_roll);
		col = vec3(0,0,1);
//		smoothrender.draw(P,V,col);
		col = vec3(.8,.8,1);
		yarn_path.draw(P,V,col);


	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();
	srand(time(0));
	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
