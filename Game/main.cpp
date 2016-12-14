#define _CRT_SECURE_NO_DEPRECATE 
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
#include "text.h"
// Assimp includes

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

#include "C:/Graphics/Game/Simple OpenGL Image Library/src/SOIL.h"


/*----------------------------------------------------------------------------
                   MESH TO LOAD
  ----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MAN_MESH "C:/Graphics/Game/man.DAE"
#define WOMAN_MESH "C:/Graphics/Game/woman.dae"
#define HEART_MESH "C:/Graphics/Game/heart.dae"

#define LEG_MESH "C:/Graphics/Game/leg.DAE"
#define MONKEY_MESH "C:/Graphics/Game/monkeytower.dae"
#define WALL_MESH "C:/Graphics/Game/wall.dae"
#define CASTLE_MESH "C:/Graphics/Game/castle2.dae"

GLuint tex_cube;
GLuint textures[6];
float anim = 0;
float fog_density = 0.0f;
bool night;
float night_cycle = 0.0f;
/*----------------------------------------------------------------------------
  ----------------------------------------------------------------------------*/

int NUM_MODELS = 7;

std::vector<float> * g_vp = new std::vector<float>[NUM_MODELS];
std::vector<float> * g_vn = new std::vector<float>[NUM_MODELS];
std::vector<float> * g_vt = new std::vector<float>[NUM_MODELS];
int * g_point_count = new int[NUM_MODELS]{ 0 };
GLuint vaos[] = { 0,1,2,3,4,5,6,7,8,9,10 };
bool * keys = new bool[256]{ false };
bool win = false;
bool lose = false;


mat4 trans_view;
mat4 man_pos = identity_mat4();

int gameState = 0;
int state = 0;
float leg_cycle = 0;
float leg_cycle2 = 0;

int leg_state = 0;
int leg_state2 = 0;

float gravity = 1;
// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;


unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;
GLfloat rotate_x = 0.0f;
GLfloat rotate_z = 0.0f;


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
                   MESH LOADING FUNCTION
  ----------------------------------------------------------------------------*/

bool load_mesh (const char* file_name, int file_num) {
  const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate); // TRIANGLES!
  if (!scene) {
    fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
    return false;
  }
  printf ("  %i animations\n", scene->mNumAnimations);
  printf ("  %i cameras\n", scene->mNumCameras);
  printf ("  %i lights\n", scene->mNumLights);
  printf ("  %i materials\n", scene->mNumMaterials);
  printf ("  %i meshes\n", scene->mNumMeshes);
  printf ("  %i textures\n", scene->mNumTextures);
  
  g_point_count[file_num] = 0;

  for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
    const aiMesh* mesh = scene->mMeshes[m_i];
    printf ("    %i vertices in mesh\n", mesh->mNumVertices);
    g_point_count[file_num] += mesh->mNumVertices;
    for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
      if (mesh->HasPositions ()) {
        const aiVector3D* vp = &(mesh->mVertices[v_i]);
        //printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
        g_vp[file_num].push_back (vp->x);
        g_vp[file_num].push_back (vp->y);
        g_vp[file_num].push_back (vp->z);
      }
      if (mesh->HasNormals ()) {
        const aiVector3D* vn = &(mesh->mNormals[v_i]);
        //printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
        g_vn[file_num].push_back (vn->x);
        g_vn[file_num].push_back (vn->y);
        g_vn[file_num].push_back (vn->z);
      }
      if (mesh->HasTextureCoords (0)) {
        const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
        //printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
        g_vt[file_num].push_back (vt->x);
        g_vt[file_num].push_back (vt->y);
      }
      if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
      }
    }
  }
  
  aiReleaseImport (scene);
  return true;
}

#pragma endregion MESH LOADING



// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {   
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		printf("Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferMesh(const char * file_path, int file_num) {
/*----------------------------------------------------------------------------
                   LOAD MESH HERE AND COPY INTO BUFFERS
  ----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	load_mesh (file_path, file_num);

	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, g_point_count[file_num] * 3 * sizeof (float), &g_vp[file_num][0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, g_point_count[file_num] * 3 * sizeof (float), &g_vn[file_num][0], GL_STATIC_DRAW);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	unsigned int vt_vbo = 0;
	glGenBuffers (1, &vt_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glBufferData (GL_ARRAY_BUFFER, g_point_count[file_num] * 2 * sizeof (float), &g_vt[file_num][0], GL_STATIC_DRAW);
	
	glGenVertexArrays(1, &vaos[file_num]);
	glBindVertexArray(vaos[file_num]);

	glEnableVertexAttribArray (loc1);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (loc2);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray (loc3);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	
}

#pragma endregion VBO_FUNCTIONS

void animateLeg() {
	if (leg_state == 0)
	{
		if (leg_cycle >= -20)
		{
			leg_cycle = leg_cycle - 0.1;
		}
		else
		{
			leg_state = 1;
		}
	}
	else if (leg_state == 1)
	{
		if (leg_cycle <= 20)
		{
			leg_cycle = leg_cycle + 0.1;
		}
		else
		{
			leg_state = 0;
		}
	}
}

bool collisionY()
{
	if (man_pos.m[13] >= 581 && man_pos.m[13] <= 624 && man_pos.m[14] <= -46) // wall 2
	{
		gravity += 0.02;
		return true;
	}
	if (man_pos.m[13] >= 781 && man_pos.m[13] <= 824 && man_pos.m[14] <= -66) // wall 2
	{
		gravity += 0.02;
		return true;
	}
	else if (((man_pos.m[13] <= 581 || man_pos.m[13] >= 624) && (man_pos.m[13] <= 781 || man_pos.m[13] >= 824)) && man_pos.m[14] <= -1.8)
	{
		gravity+=0.02;
		return true;
	}
	gravity = 0;
	return false;
}

bool collision(int dir)
{
	float xpos = man_pos.m[12];
	float zpos = man_pos.m[13];
	float ypos = man_pos.m[14];
	if (gameState == 0 && zpos >= 624 && zpos <= 780 && ypos >= -10) // wall 3
	{
		gameState = -1;
		return true;
	}
	switch (dir)
	{
	case 0: //xneg (left)
		if (xpos >= 130)
		{
			return true;
		}
		else if (zpos >= 181 && zpos <= 224 && xpos >= -61) // wall 1
		{
			return true;
		}
		break;
	case 1: //xpos (right)
		if (xpos <= -130)
		{
			return true;
		}
		else if (zpos >= 381 && zpos <= 424 && xpos <= 61) // wall 1
		{
			return true;
		}
		break;
	case 2: //zpos (forward)
		if (zpos >= 180 && zpos <= 224 && xpos >= -60) // wall 1
		{
			return true;
		}
		if (zpos >= 380 && zpos <= 424 && xpos <= 60) // wall 2
		{
			return true;
		}
		if (zpos >= 580 && zpos <= 624 && ypos >= -45) // wall 3
		{
			return true;
		}
		if (zpos >= 780 && zpos <= 824 && ypos >= -65) // wall 4
		{
			return true;
		}
		if (zpos >= 1000)  // win!
		{
			gameState = 1;
			return true;
		}
		break;
	case 3: //zneg (back)
		if (zpos <= -100)
		{
			return true;
		}
		if (zpos >= 181 && zpos <= 226 && xpos >= -60) // wall 1
		{
			return true;
		}
		if (zpos >= 381 && zpos <= 426 && xpos <= 60) // wall 2
		{
			return true;
		}
		if (zpos >= 581 && zpos <= 626 && ypos >= -45) // wall 2
		{
			return true;
		}
		if (zpos >= 781 && zpos <= 826 && ypos >= -65) // wall 2
		{
			return true;
		}

		break;
	}
	return false;
}

void move_stuff() {
	if (keys[27])
	{
		exit(1);
		return;
	}
	if (keys['w'] && !state && gameState == 0)
	{
		man_pos = translate(man_pos, vec3(0.0f, 0.0f, -0.2f));
	}
	if (keys['a'] && !state && !collision(3) && gameState == 0)
	{
		animateLeg();
		trans_view = translate(trans_view, vec3(0.01f, 0.0f, 0.0f));
		man_pos = translate(man_pos, vec3(0.0f, -0.1f, 0.0f));
	}
	if (keys['d'] && !state && !collision(2) && gameState == 0)
	{
		animateLeg();
		trans_view = translate(trans_view, vec3(-0.01f, 0.0f, 0.0f));
		man_pos = translate(man_pos, vec3(0.0f, 0.1f, 0.0f));
	}
	if (keys['w'] && state && !collision(2) && gameState == 0)
	{
		animateLeg();
		trans_view = translate(trans_view, vec3(0.0f, 0.0f, 0.01f));
		man_pos = translate(man_pos, vec3(0.0f, 0.1f, 0.0f));
	}
	if (keys['s'] && state && !collision(3) && gameState == 0)
	{
		animateLeg();
		trans_view = translate(trans_view, vec3(0.0f, 0.0f, -0.01f));
		man_pos = translate(man_pos, vec3(0.0f, -0.1f, 0.0f));
	}
	if (keys['a'] && state && !collision(0) && gameState == 0)
	{
		animateLeg();
		//trans_view = translate(trans_view, vec3(0.1f, 0.0f, 0.0f));
		man_pos = translate(man_pos, vec3(0.1f, 0.0f, 0.0f));
	}
	if (keys['d'] && state && !collision(1) && gameState == 0)
	{
		animateLeg();
		//trans_view = translate(trans_view, vec3(-0.1f, 0.0f, 0.0f));
		man_pos = translate(man_pos, vec3(-0.1f, 0.0f, 0.0f));
	}
	if (keys['q'] && !state && gameState == 0)
	{
		state = !state;
		trans_view = rotate_y_deg(trans_view, 90.0f);
	}
	if (keys['e'] && state && gameState == 0)
	{
		state = !state;
		trans_view = rotate_y_deg(trans_view, -90.0f);
	}
/*
	if (keys['z'])
	{
		trans_view = rotate_x_deg(trans_view, 0.2f);
	}
	if (keys['x'])
	{
		trans_view = rotate_x_deg(trans_view, -0.2f);
	}
	if (keys['c'])
	{
		trans_view = rotate_z_deg(trans_view, 0.2f);
	}
	if (keys['v'])
	{
		trans_view = rotate_z_deg(trans_view, -0.2f);
	}
*/
}



void display(){
	move_stuff();
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f + night_cycle, 0.8f + night_cycle, 1.0f + night_cycle, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);

	if (collisionY())
	{
		man_pos.m[14] += 0.01*gravity;
	}

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	int texture_mat_location = glGetUniformLocation(shaderProgramID, "textureColor");
	int fog_location = glGetUniformLocation(shaderProgramID, "FogDensity");

	if (!night && fog_density <= 0.04f)
	{
		fog_density += 0.000005;
		night_cycle -= 0.0001;
	}
	else
	{
		night = true;
	}
	if (night && fog_density >= 0.0f)
	{
		fog_density -= 0.000005;
		night_cycle += 0.0001;
	}
	else
	{
		night = false;
	}
	glUniform1f(fog_location, fog_density);


	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glActiveTexture(GL_TEXTURE);

	// Root of the Hierarchy
	mat4 view = trans_view;
	mat4 persp_proj = perspective(45.0, (float)width/(float)height, 0.1, 10000.0);
	mat4 global1 = identity_mat4 ();

	view = translate(view, vec3(0.0, -4.0, -35.0f));
	
	mat4 local1 = man_pos;
	local1 = rotate_x_deg(local1, 90.0f);
	local1 = rotate_y_deg(local1, 90.0f);
	local1 = scale(local1, vec3(0.1f, 0.1f, 0.1f));

	vec3 texColor = vec3(1.0f, 0.0f, 0.0f);

	mat4 globalman = global1 * local1;
	// update uniforms & draw
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, globalman.m);

	glUniform3fv(texture_mat_location, 1, texColor.v);

	glBindVertexArray(vaos[0]);
	glDrawArrays (GL_TRIANGLES, 0, g_point_count[0]);

	
	//LEGS
	mat4 local2 = translate(identity_mat4(), vec3(15.0f, 0.0f, 0.0f));
	local2 = scale(local2, vec3(0.5f, 0.5f, 1.5f));
	local2 = rotate_x_deg(local2, leg_cycle);
	mat4 global2 = globalman * local2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
    vec3 texColor2 = vec3(0.0f, 0.0f, 1.0f);
	glUniform3fv(texture_mat_location, 1, texColor2.v);
	glBindVertexArray(vaos[1]);
	glDrawArrays(GL_TRIANGLES, 0, g_point_count[1]);

	//LEG2
	mat4 local3 = translate(identity_mat4(), vec3(-15.0f, 0.0f, 0.0f));
	local3 = scale(local3, vec3(0.5f, 0.5f, 1.5f));
	local3 = rotate_x_deg(local3, -leg_cycle);
	mat4 global3 = globalman * local3;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global3.m);
	glBindVertexArray(vaos[2]);
	glDrawArrays(GL_TRIANGLES, 0, g_point_count[2]);

	if (gameState == 2)
	{
		if (!win)
		{
			PlaySound("../win.wav", NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);
			win = true;
		}
		if (state)
		{
			state = !state;
			trans_view = rotate_y_deg(trans_view, -90.0f);
		}
		mat4 localw = translate(identity_mat4(), vec3(man_pos.m[12],(1100.0f - anim),0.0f));
		localw = rotate_x_deg(localw, 90.0f);
		localw = rotate_y_deg(localw, 90.0f);
		localw = scale(localw, vec3(0.1f, 0.1f, 0.1f));

		vec3 texColor = vec3(1.0f, 0.0f, 1.0f);
		glUniform3fv(texture_mat_location, 1, texColor.v);
		mat4 globalwoman = global1 * localw;
		// update uniforms & draw
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, globalwoman.m);

		glBindVertexArray(vaos[5]);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[5]);

		//LEGS
		mat4 localw2 = translate(identity_mat4(), vec3(15.0f, 0.0f, 0.0f));
		localw2 = scale(localw2, vec3(0.5f, 0.5f, 1.5f));
		localw2 = rotate_x_deg(localw2, leg_cycle2);
		mat4 globalw2 = globalwoman * localw2;
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, globalw2.m);
		vec3 texColor2 = vec3(0.0f, 1.0f, 1.0f);
		glUniform3fv(texture_mat_location, 1, texColor2.v);
		glBindVertexArray(vaos[1]);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[1]);

		//LEG2
		mat4 localw3 = translate(identity_mat4(), vec3(-15.0f, 0.0f, 0.0f));
		localw3 = scale(localw3, vec3(0.5f, 0.5f, 1.5f));
		localw3 = rotate_x_deg(localw3, -leg_cycle2);
		mat4 globalw3 = globalwoman * localw3;
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, globalw3.m);
		glBindVertexArray(vaos[2]);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[2]);

		if (!night && fog_density <= 0.04f)
		{
			fog_density += 0.000005;
			night_cycle -= 0.0001;
		}
		else
		{
			night = true;
		}
		if (night && fog_density >= 0.0f)
		{
			fog_density -= 0.000005;
			night_cycle += 0.0001;
		}
		else
		{
			night = false;
		}

		if (anim >= 80)
		{

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textures[5]);
			glActiveTexture(GL_TEXTURE0);

			float temp = 0.0f;
			glUniform1f(fog_location, temp);

			mat4 localh = identity_mat4();
			localh = scale(localh, vec3(5.0f, 5.0f, 5.0f));
			//localh = rotate_x_deg(localh, -180.0f);
			localh = rotate_y_deg(localh, -180.0f);
			localh = rotate_z_deg(localh, 90.0f);
			localh = translate(localh, vec3(0.0f, -20.0f, -40.0f));
			mat4 globalh = globalwoman * localh;
			// update uniforms & draw
			glUniformMatrix4fv(matrix_location, 1, GL_FALSE, globalh.m);
			texColor2 = vec3(1.0f, 1.0f, 1.0f);
			glUniform3fv(texture_mat_location, 1, texColor2.v);

			glBindVertexArray(vaos[6]);
			glDrawArrays(GL_TRIANGLES, 0, g_point_count[6]);

			glUniform1f(fog_location, fog_density);



		}
		else
		{
			if (leg_state2 == 0)
			{
				if (leg_cycle2 >= -20)
				{
					leg_cycle2 = leg_cycle2 - 0.1;
				}
				else
				{
					leg_state2 = 1;
				}
			}
			else if (leg_state2 == 1)
			{
				if (leg_cycle2 <= 20)
				{
					leg_cycle2 = leg_cycle2 + 0.1;
				}
				else
				{
					leg_state2 = 0;
				}
			}
			anim += 0.2;
		}
	}
	if (gameState == -3)
	{
		add_text("XD", -0.285f, 0.0f, 60.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		gameState = 3;
	}
	//FLOOR
	//glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glActiveTexture(GL_TEXTURE);
	mat4 local4 = identity_mat4();
	local4 = scale(local4, vec3(100, 1, 15));
	local4 = translate(local4, vec3(60.0f, -4.3f, 0.0f));
	mat4 global4 = global1 * local4;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global4.m);
	vec3 texColor3 = vec3(1.0f, 1.0f, 1.0f);
	glUniform3fv(texture_mat_location, 1, texColor3.v);
	glBindVertexArray(vaos[3]);
	glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);

	//LAVA
	//glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 2);
	float temp = 0.0f;
	glUniform1f(fog_location, temp);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	glActiveTexture(GL_TEXTURE);

	mat4 local10 = identity_mat4();
	local10 = scale(local10, vec3(9, 2, 15));
	local10 = translate(local10, vec3(70.0f, -5.0f, 0.0f));
	mat4 global10 = global1 * local10;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global10.m);
	glBindVertexArray(vaos[3]);
	glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);

	glUniform1f(fog_location, fog_density);


	//CASTLE
	//glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 2);
	if (globalman.m[12] <= 50.0f)
	{
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textures[4]);
		glActiveTexture(GL_TEXTURE0);

		mat4 local11 = identity_mat4();
		local11 = rotate_x_deg(local11, -90);
		local11 = scale(local11, vec3(2, 5, 2));
		local11 = translate(local11, vec3(110.0f, 1.8f, 6.5f));
		mat4 global11 = global1 * local11;
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global11.m);

		glBindVertexArray(vaos[4]);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[4]);
	}


	//WALLS

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glActiveTexture(GL_TEXTURE);

	mat4 local8 = identity_mat4();
	local8 = scale(local8, vec3(1, 10, 15));
	local8 = translate(local8, vec3(60.0f, -9.0f, 0.0f));
	mat4 global8 = global1 * local8;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global8.m);
	glBindVertexArray(vaos[3]);

	glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);


	mat4 local9 = identity_mat4();
	local9 = scale(local9, vec3(1, 10, 15));
	local9 = translate(local9, vec3(80.0f, -7.0f, 0.0f));
	mat4 global9 = global1 * local9;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global9.m);

	glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);

	mat4 local7 = identity_mat4();
	local7 = scale(local7, vec3(1, 10, 10));
	local7 = translate(local7, vec3(40.0f, 5.0f, 5.0f));

	mat4 global7 = global1 * local7;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global7.m);
	if (globalman.m[12] >= 60.0f && state)
	{	}
	else if (globalman.m[12] <= 42.0f || !state)
	{
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);
		glDisable(GL_BLEND);
	}

	mat4 local6 = identity_mat4();
	local6 = scale(local6, vec3(1, 10, 10));
	local6 = translate(local6, vec3(20.0f, 5.0f, -5.0f));
	mat4 global6 = global1 * local6;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global6.m);
	if (globalman.m[12] >= 38.0f && state)
	{}
	else if (globalman.m[12] <= 22.0f || !state)
	{
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
		glDrawArrays(GL_TRIANGLES, 0, g_point_count[3]);
		glDisable(GL_BLEND);
	}

	if (gameState == 1)
	{
		add_text("YOU WIN!", -0.285, 0.5, 60.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		gameState = 2;
	}
	if (gameState == -1)
	{
		PlaySound("../death.wav", NULL, SND_ASYNC | SND_FILENAME );
		add_text("YOU LOSE!", -0.285, 0, 60.0f, 1.0f, 0.0f, 0.0f, 1.0f);
		gameState = -4;
	}
	draw_texts();

    glutSwapBuffers();

}

void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// rotate the model slowly around the y axis
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load mesh into a vertex buffer array
	generateObjectBufferMesh(MAN_MESH, 0);
	generateObjectBufferMesh(LEG_MESH, 1);
	generateObjectBufferMesh(LEG_MESH, 2);
	generateObjectBufferMesh(WALL_MESH, 3);
	generateObjectBufferMesh(CASTLE_MESH, 4);
	generateObjectBufferMesh(WOMAN_MESH, 5);
	generateObjectBufferMesh(HEART_MESH, 6);


	trans_view = identity_mat4 ();

	glGenTextures(5, textures);

	int width, height;
	unsigned char* image;

	glActiveTexture(GL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	image = SOIL_load_image("../grassoriginal.png", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	SOIL_free_image_data(image);
	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 0);

	glActiveTexture(GL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, textures[1]);

	int width1, height1;
	unsigned char* image1;
	image1 = SOIL_load_image("../wood.png", &width1, &height1, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, image1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	SOIL_free_image_data(image1);
	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 1);


	glActiveTexture(GL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	image1 = SOIL_load_image("../brick.jpg", &width1, &height1, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, image1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	SOIL_free_image_data(image1);
	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 2);

	glActiveTexture(GL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	image1 = SOIL_load_image("../lava.png", &width1, &height1, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, image1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	SOIL_free_image_data(image1);
	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 3);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[4]);
	image1 = SOIL_load_image("../cobble.png", &width1, &height1, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, image1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	SOIL_free_image_data(image1);
	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[5]);
	image1 = SOIL_load_image("../heart.png", &width1, &height1, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, image1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	SOIL_free_image_data(image1);
	glUniform1i(glGetUniformLocation(shaderProgramID, "tex"), 5);

	init_text_rendering("C:/Graphics/Game/Game/freemono.png", "C:/Graphics/Game/Game/freemono.meta", 800, 600);
	// x and y are -1 to 1
	// size_px is the maximum glyph size in pixels (try 100.0f)
	// r,g,b,a are red,blue,green,opacity values between 0.0 and 1.0
	// if you want to change the text later you will use the returned integer as a parameter
	int hello_id = add_text("Movement: WASD\nChange Camera: QE", -1, 1, 35.0f, 1.0f, 1.0f, 1.0f, 1.0f);

	PlaySound("../music.wav", NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	keys[key] = true;
}

void keyboardUp(unsigned char key, int x, int y)
{
	keys[key] = false;
}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Paper Mario");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutKeyboardUpFunc(keyboardUp);
	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}