#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <unistd.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define DEG2RAD(p) p*(6.28/360)
#define RAD2DEG(p) p*(360/6.28)
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

float mod(float q)
{
	if(q <= 0)
		q = q*(-1);
	return q;
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}


/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/
float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
float teta = 30;
float triangle_rotation = 0;
float rectangle_rotation = 30;
float x = -840.0f + 200 * cos(DEG2RAD(teta));//*(0.5) + (250 * cos(DEG2RAD(teta)) * (0.5));
float y = -140.f + 220 * sin(DEG2RAD(teta));// * (0.8) + (220 * sin(DEG2RAD(teta)) * (0.4));
float u = 100;
float inipig3ver = 80;
int flag =0;
int speednotsuff = 0;
int timesparks = 0;
float z = 0;
float t = 0;
float r = 0;
float prevu;
int lifes = 0;
float tetacannon = 30;
int checkonce8 = 0;
int checkonce7 = 0;
int checkonce7hor = 0;
int checkonce6 = 0;
float ang = 0;
int yay2 = 0;
int block9fall = 0;
int checkonce11 = 0;
int yes = 0;
int reboundfromright = 0;
float camera_rotation_angle = 90;
float circle_rotation = 0;
float horblock,vertblock;
float pig6collisioncomplete = 0;
int width = 1600;
int yay = 0;
int block10fall = 0;
float iniblock9 = 450.0f;
float iniblock10 = 650.0f;
int height = 800;
float q,p;
float timedur = 0;
int pig5birdcollide = 0;
int pig5collisioncomplete = 0;
int pig5flag =0;
int checkonce10 = 0;
float iniproy9 = 0,iniproy10 = 0;
float currenttime = 0;
int flaggyfally = 0;
float starttime = 0;
float iniblock9ver = 40.0f;
float inipig5ver = 75.0f;
int times2 = 0;
float smokex = -670,smokey = 275;
int basegone = 0;
int cominghere = 0;
int lifeflag = 0;
VAO *block3,*lifecircle;
int pig5flag2 = 0;
float checkonce9 = 0;
float firsttimez = 0;
int times = 0;
int pig6collide = 0;
float lefthor = -1000.0f;
float block9_rotate = 0;
float righthor = 1000.0f;
float zoomie = 1;
float vertup = 500.0f;
int pig3collide = 0;
float vertdown = -500.f;
float inipig6hor = 300.0f;
float checkonce7hor2 = 0;
int checkonce4hor3 = 0;
int checkonce12 = 0;
int checkonce13 = 0;
int checkonce4hor4 = 0;
float inipig6ver = -120.0f;
float inipig2hor = 650;
int pig3birdcollide = 0;
int times3 = 0;
int times4 = 0;

float inipig2ver = 220;
float iniblock11 = 400.0f;
float iniblock11ver = 150.0f;
int block9died = 0;
int block10died = 0;
int checkonce4hor2 = 0;
float block10_rotate = 0;
float e =1;
int pig2collide = 0;
int pig7collide = 0;
int checkonce82 = 0;
float horiu;
int count = 0;
float zoo = 0;
int pig3collisioncomplete = 0;
float xpig5 = 820.0f;
int pig7disappear = 0;
int checkonce4 = 0;
float block7_rotation = 0;
int pig4collide = 0;
float inipig4hor = 370.0f;
float inipig4ver = 70.0f;
float smokehor = 0.5,smokever = 0.5;
float xpig3=570.0f;
float pig3flag =0;
int score = 0;
float times9 = 0;
VAO *speedrect;
int flaggy9 = 0;
int flaggy10 = 0;
float t10;
float times10 = 0;
int block9finish = 0;
int timescheck = 0;
float tlast = 0;
float t9 = 0;
float iniblock10ver = 40;
int times6 = 0,times5 = 0,pig5collide = 0;
int block10finish = 0;
int block10collide = 0;
float circle_rot_dir = 1;
bool circle_rot_status =true;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.

	if (action == GLFW_REPEAT || action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_A:
				if(flag == 0 && teta < 80)
				{
					rectangle_rotation = rectangle_rotation + 2;
					tetacannon = tetacannon + 2;
					teta = tetacannon;
					/*	if(tetacannon == 0){
						x = x-2;
						y = -136.f;}
						else
						{*/
					x = -840.0f + 200 * cos(DEG2RAD(teta));//*(0.5) + (250 * cos(DEG2RAD(teta)) * (0.5)); 
					y = -140.f + 220 * sin(DEG2RAD(teta));
					if(tetacannon < 0)
						y = -140.f + 180 * sin(DEG2RAD(teta));//* (0.8) + (220 * sin(DEG2RAD(teta)) * (0.4));
					else if(tetacannon > 0 && tetacannon <= 10)
						y = -140.f + 280 * sin(DEG2RAD(tetacannon));
					//}
				}
				break;
			case GLFW_KEY_B:

				if(flag == 0 && teta > -10)
				{
					rectangle_rotation = rectangle_rotation - 2;
					tetacannon = tetacannon - 2;
					teta = tetacannon;
					/*if(tetacannon == 0){
					  x = x-2;
					  y = -133.f;}
					  else
					  {*/	
					x = -840.0f + 200 * cos(DEG2RAD(tetacannon));//*(0.5) + (250 * cos(DEG2RAD(teta)) * (0.5)); 
					y = -140.f + 220 * sin(DEG2RAD(tetacannon));
					if(tetacannon < 0)
						y = -140.f + 180 * sin(DEG2RAD(tetacannon));//* (0.8) + (220 * sin(DEG2RAD(teta)) * (0.4));
					else if(tetacannon > 0 && tetacannon <= 10)
						y = -140.f + 280 * sin(DEG2RAD(tetacannon));
					//}
				}
				//canon should go down
				break;
			case GLFW_KEY_SPACE:
				if(flag == 0)
				{
					flag = 1;
					checkonce6 = 0;
					checkonce8 = 0;
					times = 0;
					prevu = u;
					teta = tetacannon;	
					/*if(tetacannon == 0){
					  x = x-2;
					  y = -133.f;}
					  else
					  {*/

					x = -840.0f + 200 * cos(DEG2RAD(tetacannon));//*(0.5) + (250 * cos(DEG2RAD(teta)) * (0.5)); 
					y = -140.f + 220 * sin(DEG2RAD(tetacannon));
					if(tetacannon < 0)
						y = -140.f + 180 * sin(DEG2RAD(tetacannon));//* (0.8) + (220 * sin(DEG2RAD(teta)) * (0.4));
					else if(tetacannon > 0 && tetacannon <= 10)
						y = -140.f + 280 * sin(DEG2RAD(tetacannon));
					//	}
					z=0;
					r = 0;
					t = 0;
				}
				break;
			case GLFW_KEY_F:
				if(flag == 0){
					if(u< 200)
						u = u+5;	

				}
				break;
			case GLFW_KEY_S:
				if(flag == 0){
					if(u>=5)
						u = u-5;
				}
				break;

			case GLFW_KEY_LEFT:
				if(lefthor > -2000)
				{	
					//             circle_rot_status = !circle_rot_status;
					lefthor = lefthor-50;
					righthor = righthor - 50;
				}
				break;
			case GLFW_KEY_RIGHT:
				// do something ..
				if(righthor < 2000){
					lefthor = lefthor + 50;
					righthor = righthor + 50;
				}
				break;
			case GLFW_KEY_UP:
				if( zoomie > 0.5){
					zoomie = zoomie - 0.1;
					vertup = vertup + 30;
					vertdown = vertdown - 30;
					lefthor = lefthor - 60;
					righthor = righthor + 60;
					//left = left * zoomie;
					//right = right * zoomie;
					//	lefthor = lefthor + left;
					//	righthor = righthor + right;

				}	
				break;
			case GLFW_KEY_DOWN:
				if(zoomie < 1.5)
				{	
					zoomie = zoomie + 0.1;
					vertup = vertup - 30;
					vertdown = vertdown + 30;
					lefthor = lefthor + 60;
					righthor = righthor - 60 ;
					//	lefthor = lefthor + left;
					//	righthor = righthor + right;	
					//       rectangle_rot_status = !rectangle_rot_status;
				}
			default:
				break;
		}
	}
	else if (action == GLFW_RELEASE) {
		switch (key) {

			break;
			case GLFW_KEY_SPACE:
			starttime = glfwGetTime();
			break;
			case GLFW_KEY_ESCAPE:
			quit(window);
			break;
			default:
			break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		default:
			break;
	}
}

float mousex,mousey,radi;
/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:

			if (action == GLFW_RELEASE)
			{
				if(flag == 0){
					if(u < 200)
						u = u+5;	
				}

			}




			// triangle
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_RELEASE) {
				if(flag == 0)
				{
					if(u >= 5)
					{
						u = u-5;
					}
				}

			}
			break;

		default:
			break;
	}
}
/*void mouse(GLFWwindow* window,double x,double y)
  {
  mousex = float(x / zoomie);
  mousey = float(y / zoomie);//(-1) * (float(y)-500);
  cout << "mousex " << mousex << '\n';
  cout << mousey << '\n';
  radi = atan((mousey + 140)/(mousex + 840));
  if(RAD2DEG(radi) < 80 && RAD2DEG(radi) > -10){
  tetacannon = tetacannon + RAD2DEG(radi);
  rectangle_rotation = tetacannon;
  teta = tetacannon;

  x = -840.0f + 200 * cos(DEG2RAD(tetacannon));//*(0.5) + (250 * cos(DEG2RAD(teta)) * (0.5)); 
  y = -140.f + 220 * sin(DEG2RAD(tetacannon));
  if(tetacannon < 0)
  y = -140.f + 180 * sin(DEG2RAD(tetacannon));//* (0.8) + (220 * sin(DEG2RAD(teta)) * (0.4));
  else if(tetacannon > 0 && tetacannon <= 10)
  y = -140.f + 280 * sin(DEG2RAD(tetacannon));
  }
  }*/		


void scrollback(GLFWwindow* window,double x,double y)
{
	zoo = float(y)/10;
	if(zoo >= 0)
	{
		if( zoomie > 0.5){
			zoomie = zoomie - 0.1;
			vertup = vertup + 30;
			vertdown = vertdown - 30;
			lefthor = lefthor - 60;
			righthor = righthor + 60;
			//left = left * zoomie;
			//right = right * zoomie;
			//	lefthor = lefthor + left;
			//	righthor = righthor + right;

		}	
	}
	else if(zoo < 0){
		if(zoomie < 1.5)
		{	
			zoomie = zoomie + 0.1;
			vertup = vertup - 30;
			vertdown = vertdown + 30;
			lefthor = lefthor + 60;
			righthor = righthor - 60 ;
			//	lefthor = lefthor + left;
			//	righthor = righthor + right;	
			//       rectangle_rot_status = !rectangle_rot_status;
		}
	}
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	   is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	// Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

	// Ortho projection for 2D views
	Matrices.projection = glm::ortho(-1000.0f, 1000.0f, -500.0f, 500.0f, 0.1f, 500.0f);
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

VAO *triangle, *rectangle,*tree5,*tree6;
VAO *circle,*hill,*upback,*block2;
VAO *tree1,*trunk1,*downback,*downfull,*pig2,*pig3,*pig4,*pig5,*pig6,*pig7;
VAO *block1,*block4,*block5,*block6,*block7,*block8,*block9,*block10,*block11;
VAO *tree2,*tree3,*tree4,*pig1,*basecannon;
VAO *sparks;

void createbase()
{
	const GLfloat vertex_buffer_data [] = {
		0, 0,0, // vertex 0
		400,0,0, // vertex 1
		200,200,0 // vertex 2
	};

	const GLfloat color_buffer_data [] = {
		0.5,0.20,0.05, // color 0
		0.5,0.20,0.05, // color 1
		0.5,0.20,0.05 // color 2
	};


	// create3DObject creates and returns a handle to a VAO that can be used later
	hill = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
// Creates the triangle object used in this sample code
void createTriangle ()
{
	// ONLY vertices between the bounds specified in glm::ortho will be visible on screen 

	// Define vertex array as used in glBegin (GL_TRIANGLES) 

	GLfloat* vertex_buffer_data = new GLfloat [3*360];
	GLfloat* color_buffer_data = new GLfloat [3*360];
	float rad = 60;

	for(int i=0;i<360;i++)
	{
		vertex_buffer_data [3*i] = (rad * cos(DEG2RAD(i)));
		vertex_buffer_data [3*i + 1] = (rad * sin(DEG2RAD(i)));
		vertex_buffer_data [3*i + 2] = 0;
		if(i%2 == 0)
		{
			color_buffer_data [3*i] = 0.8;
			color_buffer_data [3*i + 1] = 0.58;
			color_buffer_data [3*i + 2] = 0.047;
		}
		else
		{
			color_buffer_data [3*i] = 1.0;
			color_buffer_data [3*i + 1] = 1.0;
			color_buffer_data [3*i + 2] = 1.0;
		}
	}
	//205 149 12
	//61 61 61
	/*	 vertex_buffer_data [3*360 + 1] = 12;
	// vertex 1
	vertex_buffer_data [360 * 3 + 2] = 12;
	vertex_buffer_data [360 * 3 + 3] = 0;*/
	/*1.2,-1,0, // vertex 2
	  1.2, 1,0, // vertex 3

	  1.2, 1,0, // vertex 3
	  -1.2, 1,0, // vertex 4
	  -1.2,-1,0  // vertex 1
	  };*/

/*		vertex_buffer_data [360 * 3 + 4] = 15;
		vertex_buffer_data [360 * 3 + 5] = 12;
		vertex_buffer_data [360 * 3 + 6] = 0;

		vertex_buffer_data [360 * 3 + 7] = 12;
		vertex_buffer_data [360 * 3 + 8] = 15;
		vertex_buffer_data [360 * 3 + 9] = 0;

		color_buffer_data[3*360 + 1] = 1;
		color_buffer_data[3*360 + 2] = 0;
		color_buffer_data[3*360 + 3] = 0;
		color_buffer_data[3*360 + 4] = 1;
		color_buffer_data[3*360 + 5] = 0;
		color_buffer_data[3*360 + 6] = 0;
		color_buffer_data[3*360 + 7] = 1;
		color_buffer_data[3*360 + 8] = 0;
		color_buffer_data[3*360 + 9] = 0;

/* color_buffer_data [] = {
1,0,0, // color 1
0,0,1, // color 2
0,1,0, // color 3

0,1,0, // color 3
0.3,0.3,0.3, // color 4
1,0,0  // color 1
};*/
triangle = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);

/*static const GLfloat vertex_buffer_data [] = {
  0, 50,0, // vertex 0
  -50,-50,0, // vertex 1
  50,-50,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
  1,0,0, // color 0
  1,0,0, // color 1
  1,0,0 // color 2
  };

// create3DObject creates and returns a handle to a VAO that can be used later
triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);*/
}

// Creates the rectangle object used in this sample code
VAO* createRectangle (float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,float color1,float color2,float color3)
{
	// GL3 accepts only Triangles. Quads are not supported
	const GLfloat vertex_buffer_data [] = {
		x1,y1,0,
		x2,y2,0,
		x3,y3,0,

		x3,y3,0,
		x4,y4,0,
		x1,y1,0
			//-30,0,0, // vertex 1
			//190, 0,0, // vertex 2
			//220, 50,0, // vertex 3

			//220, 50,0, // vertex 3
			//-30, 50,0, // vertex 4
			//-30,0,0  // vertex 1
	};

	const GLfloat color_buffer_data [] = {
		color1,color2,color3, // color 1
		color1,color2,color3, // color 2
		color1,color2,color3,// color 3

		color1,color2,color3,// color 3
		color1,color2,color3, // color 4
		color1,color2,color3,  // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}


VAO* createblocks (float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,float color1,float color2,float color3)
{
	// GL3 accepts only Triangles. Quads are not supported
	const GLfloat vertex_buffer_data [] = {
		x1,y1,0, // vertex 1
		x2,y2,0, // vertex 2
		x3,y3,0, // vertex 3

		x3,y3,0, // vertex 3
		x4,y4,0, // vertex 4
		x1,y1,0  // vertex 1
	};

	const GLfloat color_buffer_data [] = {
		color1,color2,color3, // color 1
		color1,color2,color3,// color 2
		color1,color2,color3,// color 3

		color1,color2,color3, // color 3
		color1,color2,color3, // color 4
		color1,color2,color3,  // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	VAO *new1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	return new1;
}





//139 101 8

/*void createCircle ()
  {
// GL3 accepts only Triangles. Quads are not supported

GLfloat* vertex_buffer_data = new GLfloat [3*360];
GLfloat* color_buffer_data = new GLfloat [3*360];
float rad = 40;
for(int i=0;i<360;i++)
{
vertex_buffer_data [3*i] = (rad * cos(DEG2RAD(i)));
vertex_buffer_data [3*i + 1] = (rad * sin(DEG2RAD(i)));
vertex_buffer_data [3*i + 2] = 0;

color_buffer_data [3*i] = 1;
color_buffer_data [3*i + 1] = 0;
color_buffer_data [3*i + 2] = 0;
}


circle = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
}*/


VAO* createTrees (float rad1,float rad2,float color1,float color2,float color3)
{
	// GL3 accepts only Triangles. Quads are not supported

	GLfloat* vertex_buffer_data = new GLfloat [3*360];
	GLfloat* color_buffer_data = new GLfloat [3*360];
	for(int i=0;i<360;i++)
	{
		vertex_buffer_data [3*i] = (rad1 * cos(DEG2RAD(i)));
		vertex_buffer_data [3*i + 1] = (rad2 * sin(DEG2RAD(i)));
		vertex_buffer_data [3*i + 2] = 0;

		color_buffer_data [3*i] = color1;
		color_buffer_data [3*i + 1] = color2;
		color_buffer_data [3*i + 2] = color3;
	}
	create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
}
VAO* createSparks (float rad1,float rad2)
{
	// GL3 accepts only Triangles. Quads are not supported

	GLfloat* vertex_buffer_data = new GLfloat [3*360];
	GLfloat* color_buffer_data = new GLfloat [3*360];
	for(int i=0;i<360;i++)
	{
		vertex_buffer_data [3*i] = (rad1 * cos(DEG2RAD(i)));
		vertex_buffer_data [3*i + 1] = (rad2 * sin(DEG2RAD(i)));
		vertex_buffer_data [3*i + 2] = 0;
		if(i%2==0)
		{
		color_buffer_data [3*i] = 0.8;
		color_buffer_data [3*i + 1] = 0.58;
		color_buffer_data [3*i + 2] = 0.047;
		}
		else
		{
		color_buffer_data [3*i] = 0.239;
		color_buffer_data [3*i + 1] = 0.239;
		color_buffer_data [3*i + 2] = 0.239;
		}
	}
	create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
}


void createbackground ()
{
	// GL3 accepts only Triangles. Quads are not supported
	const GLfloat vertex_buffer_data [] = {
		-5000,-300,0, // vertex 1
		5000,-300,0, // vertex 2
		5000,2500,0, // vertex 3

		5000,2500,0, // vertex 3
		-5000,2500,0, // vertex 4
		-5000,-300,0  // vertex 1
	};

	const GLfloat color_buffer_data [] = {
		1,1,1, // color 1
		1,1,1, // color 2
		0,1,1, // color 3

		0,1,1,
		0,1,1,
		1,1,1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	upback = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createbackground2 ()
{
	// GL3 accepts only Triangles. Quads are not supported
	const GLfloat vertex_buffer_data [] = {
		-5000,-315,0, // vertex 1
		5000,-315,0, // vertex 2
		5000,-300,0, // vertex 3

		5000,-300,0, // vertex 3
		-5000,-300,0, // vertex 4
		-5000,-315,0  // vertex 1
	};

	const GLfloat color_buffer_data [] = {
		0,1,0, // color 1
		0,1,0, // color 2
		0,1,0, // color 3

		0,1,0, // color 3
		0,1,0, // color 4
		0,1,0  // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	downback = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createbackground3 ()
{
	// GL3 accepts only Triangles. Quads are not supported
	const GLfloat vertex_buffer_data [] = {
		-5000,-2500,0, // vertex 1
		5000,-2500,0, // vertex 2
		5000,-315,0, // vertex 3

		5000,-315,0, // vertex 3
		-5000,-315,0, // vertex 4
		-5000,-2500,0  // vertex 1
	};

	const GLfloat color_buffer_data [] = {
		// 0.5,0.15,0.05,// color 1
		// color 2
		0.5,0.20,0.05,

		0.5,0.20,0.05,
		0.74,0.36,0, // color 3

		//0.6,0.4,0.12, // color 3
		0.74,0.36,0,
		// color 4
		0.74,0.36,0, 
		0.5,0.20,0.05, // color 1
	};
	//139 69 19 dark
	//205 133 63 light
	// create3DObject creates and returns a handle to a VAO that can be used later
	downfull = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createbaseofcannon ()
{
	// GL3 accepts only Triangles. Quads are not supported
	const GLfloat vertex_buffer_data [] = {
		0,0,0, // vertex 1
		300,0,0, // vertex 2
		300,50,0, // vertex 3


		300,50,0, // 3
		200,100,0, //4
		0,0,0,//1

		0,0,0, //1
		200,100,0, //4
		100,100,0, //5

		100,100,0, //5
		0,100,0, //6
		0,0,0

	};

	const GLfloat color_buffer_data [] = {

		0.54,0.137,0.137,
		0.54,0.137,0.137,
		0.54,0.137,0.137,
		0.54,0.137,0.137,
		0.54,0.137,0.137,	
		0.54,0.137,0.137,
		0.54,0.137,0.137,
		0.54,0.137,0.137,
		0.54,0.137,0.137,

		0.54,0.137,0.137,
		0.54,0.137,0.137,
		0.54,0.137,0.137
			/*  0.5,0.15,0.05,// color 1
			    0.5,0.15,0.05,// color 2
			    0.6,0.4,0.12, // color 3

			    0.6,0.4,0.12, // color 3
			    0.6,0.4,0.12, // color 4
			    0.5,0.15,0.05,  // color 1



			    0.6,0.4,0.12, // 1
			    0.6,0.4,0.12, // color 4
			    0.5,0.15,0.05,//5

			    0.6,0.4,0.12, // 5
			    0.6,0.4,0.12, // color 6
			    0.5,0.15,0.05//1*/
			//139 35 35 
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	basecannon = create3DObject(GL_TRIANGLES, 12, vertex_buffer_data, color_buffer_data, GL_FILL);
}


/* static const GLfloat vertex_buffer_data [] = {
   -1.2,-1,0, // vertex 1
   1.2,-1,0, // vertex 2
   1.2, 1,0, // vertex 3

   1.2, 1,0, // vertex 3
   -1.2, 1,0, // vertex 4
   -1.2,-1,0  // vertex 1
   };

   static const GLfloat color_buffer_data [] = {
   1,0,0, // color 1
   0,0,1, // color 2
   0,1,0, // color 3

   0,1,0, // color 3
   0.3,0.3,0.3, // color 4
   1,0,0  // color 1
   };

// create3DObject creates and returns a handle to a VAO that can be used later
circle = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
} */



/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
	Matrices.projection = glm::ortho(lefthor, righthor, vertdown, vertup, 0.1f, 500.0f);
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model





	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatedownback = glm::translate (glm::vec3(0,0,0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatedownback; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(downback);



	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateupback = glm::translate (glm::vec3(0,0,0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translateupback; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(upback);



	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatebrown = glm::translate (glm::vec3(0,0,0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatebrown; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(downfull);



	/* Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatetrees = glm::translate (glm::vec3(-790, 300, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatetrees; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(tree1);*/

	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatetrees2 = glm::translate (glm::vec3(-610, 300, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatetrees2; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(tree2);

	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatetrees3 = glm::translate (glm::vec3(-735, 325, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatetrees3; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(tree3);

	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatetrees4 = glm::translate (glm::vec3(-735, 275, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatetrees4; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(tree4);

	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatetrees5 = glm::translate (glm::vec3(-670, 325, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatetrees5; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(tree5);





	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatecan = glm::translate (glm::vec3(-900,-300, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatecan; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(basecannon);

	Matrices.model = glm::mat4(1.0f);

	//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatehill = glm::translate (glm::vec3(450,-300, 0));        // glTranslatef
	// rotate about vector (-1,1,1)
	Matrices.model *= translatehill; //* rotateRectangle;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(hill);

	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateTriangle = glm::translate (glm::vec3(-800.0f, -140.0f, 0.0f)); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 

	glm::mat4 TriangleTransform = translateTriangle; //* rotateTriangle;

	Matrices.model *= TriangleTransform; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(triangle);


	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatepig = glm::translate (glm::vec3(890.0f, -210.0f, 0.0f)); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 

	/*glm::mat4 Transformpig = translatepig; //* rotateTriangle;

	  Matrices.model *= Transformpig; 
	  MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(pig1);*/ 

	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateblock1 = glm::translate (glm::vec3(870.0f, -300.0f, 0.0f)); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 

	glm::mat4 Transformblock1 = translateblock1; //* rotateTriangle;

	Matrices.model *= Transformblock1; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block1); 

	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock2 = glm::translate (glm::vec3(830.0f, -280.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock2 = translateblock2; //* rotateblock2;

	Matrices.model *= Transformblock2; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block2); 


	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock3 = glm::translate (glm::vec3(750.0f, -200.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock3 = translateblock3; //* rotateblock2;

	Matrices.model *= Transformblock3; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block3); 


	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock4 = glm::translate (glm::vec3(300.0f, 0.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock4 = translateblock4; //* rotateblock2;

	Matrices.model *= Transformblock4; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block4); 


	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock5 = glm::translate (glm::vec3(550.0f,-200.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock5 = translateblock5; //* rotateblock2;

	Matrices.model *= Transformblock5; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block5); 

	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock6 = glm::translate (glm::vec3(400.0f, -300.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock6 = translateblock6; //* rotateblock2;

	Matrices.model *= Transformblock6; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block6); 

	Matrices.model = glm::mat4(1.0f);
	glm::mat4 rotateblock7 = glm::rotate((float)(block7_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock7 = glm::translate (glm::vec3(150.0f, -200.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock7 = translateblock7 * rotateblock7;

	Matrices.model *= Transformblock7; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block7); 


	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock8 = glm::translate (glm::vec3(190.0f, -300.0f, 0.0f)); // glTranslatef


	glm::mat4 Transformblock8 = translateblock8; //* rotateblock2;

	Matrices.model *= Transformblock8; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block8); 


	Matrices.model = glm::mat4(1.0f);
	glm::mat4 rotateblock9 = glm::rotate((float)((block9_rotate)*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock9 = glm::translate (glm::vec3(iniblock9,iniblock9ver, 0.0f)); // glTranslatef


	glm::mat4 Transformblock9 = translateblock9 * rotateblock9;

	Matrices.model *= Transformblock9; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block9); 


	Matrices.model = glm::mat4(1.0f);
	glm::mat4 rotateblock10 = glm::rotate((float)(block10_rotate*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock10 = glm::translate (glm::vec3(iniblock10, iniblock10ver, 0.0f)); // glTranslatef


	glm::mat4 Transformblock10 = translateblock10 * rotateblock10;

	Matrices.model *= Transformblock10; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block10);

	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translateblock11 = glm::translate (glm::vec3(iniblock11, iniblock11ver, 0.0f)); // glTranslatef


	glm::mat4 Transformblock11 = translateblock11; //* rotateblock2;

	Matrices.model *= Transformblock11; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(block11); 

	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatepig2 = glm::translate (glm::vec3(inipig2hor, inipig2ver, 0.0f)); // glTranslatef


	glm::mat4 Transformpig2 = translatepig2; //* rotateblock2;

	Matrices.model *= Transformpig2; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(pig2); 


	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatepig4 = glm::translate (glm::vec3(inipig4hor, inipig4ver, 0.0f)); // glTranslatef


	glm::mat4 Transformpig4 = translatepig4; //* rotateblock2;

	Matrices.model *= Transformpig4; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(pig4); 




	Matrices.model = glm::mat4(1.0f);
	// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
	glm::mat4 translatepig6 = glm::translate (glm::vec3(inipig6hor, inipig6ver, 0.0f)); // glTranslatef
	glm::mat4 Transformpig6 = translatepig6; //* rotateblock2;

	Matrices.model *= Transformpig6; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//camera_rotation_angle++; // Simulating camera rotation
	draw3DObject(pig6); 

	if(pig7disappear == 0)
	{

		Matrices.model = glm::mat4(1.0f);
		// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
		glm::mat4 translatepig7 = glm::translate (glm::vec3(320.0f, -260.0f, 0.0f)); // glTranslatef
		glm::mat4 Transformpig7 = translatepig7; //* rotateblock2;

		Matrices.model *= Transformpig7; 
		MVP = VP * Matrices.model; // MVP = p * V * M

		//  Don't change unless you are sure!!
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		//camera_rotation_angle++; // Simulating camera rotation
		draw3DObject(pig7); 
	}

	//  Don't change unless you are sure!!

	// glm::mat4 rotatecircle = glm::rotate((float)(circle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
	//glm::mat4 triangleTransform = translateTriangle; //* rotateTriangle;

	float lifesx = 900;
	float lifesy = 450;
	for( int i=0; i<(10-lifes); i++)
	{

		Matrices.model = glm::mat4(1.0f);
		//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
		glm::mat4 translatelifes = glm::translate (glm::vec3((lifesx-(i*50)),lifesy,0));        // glTranslatef
		// rotate about vector (-1,1,1)
		Matrices.model *= translatelifes;// * rotateRectangle;
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// draw3DObject draws the VAO given to it using current MVP matrix
		draw3DObject(lifecircle);
	}
	if(flag == 1)
	{

		currenttime = glfwGetTime();
		while(timedur <= 0.1)
		{
			currenttime = glfwGetTime();
			timedur+= currenttime - starttime;
		}
		if(timedur > 0.1)	
			timedur = 0.1;
		t = timedur + t;
		//cout << t << '\n';
		timedur = 0;
		//cout << starttime - currenttime << '\n';
		horblock = x+r;
		vertblock = y+z;
		r =  t * u * cos(DEG2RAD(teta))*e ;
		z = u*(sin(DEG2RAD(teta)))*t - (4.9*t*t);
		q = x+r;
		p = mod(y+z);
		float iniy9;
		float iniy10;
		if(iniblock9 <= 300 && flaggy9 == 0 && yay2==0 && block9fall!=1)
		{
			block9died = 1;
			block9finish = 1;
			if(times9 == 0)
			{
				t9 = 0;
				iniy9 = iniblock9ver;
				iniproy9 = iniblock9;
				times9 = 1;
				block9_rotate = 0;
			}
			t9 = t9+0.1;

			if(iniblock9ver + 300 <= 5 && iniblock9ver + 300 >= -5 )
			{
				flaggy9 = 1;
				block9_rotate = 180 - block9_rotate;
				iniblock9ver = -300;
			}
			else
			{
				block9finish = 1;
				iniblock9ver = iniy9 - (4.9*t9*t9);
				block9_rotate++;
				iniblock9 = iniproy9 + u*cos(DEG2RAD(180))*0.2*t9;
			}
			//	cout << block9finish << '\n';

			//	cout << flaggy9 << " flaggy " << '\n';
			//cout << "iniproy9" << iniproy9 << '\n';
			//cout << "iniblock9=" << iniblock9 << '\n';
			//cout << "iniblock9ver " << iniblock9ver << '\n';
			//cout << "iniy9" << iniy9 << '\n';
			//	cout << block9_rotate << '\n';
		}

		if(flaggy9 == 1){
			//	block9_rotate = 0;
			block9finish = 0;
		}
		if(iniblock10 <= 300 && flaggy10 == 0 && yay==0 && block10fall!=1)
		{
			block10died = 1;
			block10finish = 1;
			if(times10 == 0)
			{
				t10 = 0;
				iniproy10 = iniblock10;
				iniy10 = iniblock10ver;
				times10 = 1;
				block10_rotate = 0;
			}
			t10 = t10+0.1;

			if(iniblock10ver + 300 <= 5 && iniblock10ver + 300 >= -5 )
			{
				flaggy10 = 1;
				block10_rotate = 179 - block10_rotate;
				iniblock10ver = -300;
			}
			else
			{
				block10finish = 1;
				iniblock10ver = iniy10 - (4.9*t10*t10);
				block10_rotate++;
				iniblock10 = iniproy10 + u*cos(DEG2RAD(180))*0.4*t10;
			}
			//	cout << iniblock10 << "   iniblock10" << '\n';
			//cout << block10finish << '\n';

			//cout << flaggy9 << " flaggy10 " << '\n';

		}
		if(flaggy10 == 1){
			//	block9_rotate = 0;
			block10finish = 0;
		}
		if(pig6collide == 1)
		{
			//	cout << "yes" << '\n';

			if(times6 == 0)
			{
				smokex = inipig6hor;
				smokey = inipig6ver;
				count = 0;
			}


			smokehor = smokehor + 0.1;
			smokever = smokever + 0.1;
			/*if(vertblock <= -130 || horblock >= 400)
			  {
			  r = 0;
			  z = 0;
			  x = 360;
			  y = -130;
			  }*/
			x = 2600;
			r = 0;

			count++;
			if(count == 15)	
			{			
				speednotsuff = 1;
				pig6collide = 0;
			}
			if(times6 == 0)
			{
				times6 = 1;
				inipig6hor = 2600;
				inipig6ver = 2600;
			}
		}
		if(pig5birdcollide == 1)
		{
			//	cout << "yes" << '\n';

			if(times5 == 0)
			{
				smokex = xpig5;
				smokey = inipig5ver;
				count = 0;
			}


			smokehor = smokehor + 0.1;
			smokever = smokever + 0.1;
			/*if(vertblock <= -130 || horblock >= 400)
			  {
			  r = 0;
			  z = 0;
			  x = 360;
			  y = -130;
			  }*/
			x = 2600;
			r = 0;

			count++;
			if(count == 15)	
			{			
				speednotsuff = 1;
				pig5collide = 0;
			}
			if(times5 == 0)
			{
				times5 = 1;
				xpig5 = 2600;
				inipig5ver = 2600;
			}
		}

		else if(pig7collide == 1)
		{
			block7_rotation--;
			r = 0;
			if(y >= -260)
				y = y-10;
			z = 0;
			if(block7_rotation == -20)
			{
				//score = score+100;
				pig7disappear = 1;
				pig7collide = 0;
				speednotsuff = 1;
			}
		}
		else if(pig2collide == 1)
		{
			//	cout << "yes" << '\n';

			if(times2 == 0)
			{
				smokex = inipig2hor;
				smokey = inipig2ver;
				count = 0;
			}

			smokehor = smokehor + 0.1;
			smokever = smokever + 0.1;
			/*if(vertblock <= -130 || horblock >= 400)
			  {
			  r = 0;
			  z = 0;
			  x = 360;
			  y = -130;
			  }*/
			x = 2600;
			r = 0;

			count++;
			if(count == 15)	
			{			
				speednotsuff = 1;
				pig2collide = 0;
			}
			if(times2 == 0)
			{
				times2 = 1;
				inipig2hor = 1600;
				inipig2ver = 1600;
			}
		}	
		else if(pig3birdcollide == 1)
		{
			//	cout << "yes" << '\n';

			if(times3 == 0)
			{
				smokex = xpig3;
				smokey = inipig3ver;
				count = 0;
			}
			smokehor = smokehor + 0.1;
			smokever = smokever + 0.1;
			/*if(vertblock <= -130 || horblock >= 400)
			  {
			  r = 0;
			  z = 0;
			  x = 360;
			  y = -130;
			  }*/
			x = 2600;
			r = 0;

			count++;
			if(count == 15)	
			{			
				speednotsuff = 1;
				pig3birdcollide = 0;

			}
			if(times3 == 0)
			{
				times3 = 1;
				xpig3 = 2600;
				inipig3ver = 2600;
			}
		}
		else if(pig4collide == 1)
		{
			//	cout << "yes" << '\n';

			if(times4 == 0)
			{
				smokex = inipig4hor;
				smokey = inipig4ver;
				count = 0;
			}

			smokehor = smokehor + 0.1;
			smokever = smokever + 0.1;
			/*if(vertblock <= -130 || horblock >= 400)
			  {
			  r = 0;
			  z = 0;
			  x = 360;
			  y = -130;
			  }*/
			x = 2600;
			r = 0;

			count++;
			if(count == 15)	
			{			
				speednotsuff = 1;
				pig4collide = 0;
			}
			if(times4 == 0)
			{
				times4 = 1;
				inipig4hor = 2600;
				inipig4ver = 2600;
			}	

		}
		else{
			float diffpig6 = sqrt(pow((horblock - inipig6hor),2) + pow((vertblock - inipig6ver),2)) - 110;
			float diffpig2 = sqrt(pow((horblock - inipig2hor),2) + pow((vertblock - inipig2ver),2)) - 90;
			float diffpig4 = sqrt(pow((horblock - inipig4hor),2) + pow((vertblock - inipig4ver),2)) - 80;
			float diffpig3 = sqrt(pow((horblock - xpig3),2) + pow((vertblock - inipig3ver),2)) - 90;
			float diffpig5 = sqrt(pow((horblock - xpig5),2) + pow((vertblock - inipig5ver),2)) - 90;

			if(yay2 == 1)
			{

				iniblock9 -= (u * cos(DEG2RAD(teta)) * (0.05));
				block9_rotate -= 3;
				if(block9_rotate == -78)
				{
					block9fall = 1;
					yay2 = 0;
				}
			}
			if(yay == 1)
			{
				iniblock10 -= (u * cos(DEG2RAD(teta)) * (0.05));
				block10_rotate -= 3;
				if(block10_rotate == -78)
				{
					block10fall = 1;
					yay = 0;
				}
			}

			if(diffpig6<=5 && diffpig6>=-5 && pig6collide==0) 
			{
				pig6collide = 1;
				pig6collisioncomplete = 1;
				score = score + 100;
				/*	        Matrices.model = glm::mat4(1.0f);
						glm::mat4 translatetrees = glm::translate (glm::vec3(inipig6hor, inipig6ver, 0));
						glm :: mat4 scaletrees = glm::scale (glm::vec3(0.2,0.2,1));     // glTranslatef
						Matrices.model *= translatetrees * scaletrees; 
						MVP = VP * Matrices.model;
						glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
						draw3DObject(tree1);
						smokehor = smokehor + 0.1;
						smokever = smokever + 0.1;*/

			}
			if(diffpig3<=5 && diffpig3>=-5 && pig3collide!=1 && pig3birdcollide == 0 && pig3collisioncomplete==0) 
			{
				pig3birdcollide = 1;
				pig3collisioncomplete = 1;
				score = score + 100;
				//	cout << "happpening" << '\n';
				/*	        Matrices.model = glm::mat4(1.0f);
						glm::mat4 translatetrees = glm::translate (glm::vec3(inipig6hor, inipig6ver, 0));
						glm :: mat4 scaletrees = glm::scale (glm::vec3(0.2,0.2,1));     // glTranslatef
						Matrices.model *= translatetrees * scaletrees; 
						MVP = VP * Matrices.model;
						glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
						draw3DObject(tree1);
						smokehor = smokehor + 0.1;
						smokever = smokever + 0.1;*/

			}
			else if(diffpig2<=5 && diffpig2>=0 && pig2collide==0) 
			{		pig2collide = 1;score = score + 100;}

			else if(diffpig4<=5 && diffpig2>=0 && pig4collide==0) 
			{	pig4collide = 1;score = score + 100;}
			else if(diffpig5<=5 && diffpig5>=0 && pig5birdcollide==0 && pig5collisioncomplete==0) 
			{	pig5birdcollide = 1;score = score + 100;
				pig5collisioncomplete = 1;}
			else if(pig3collide == 1 && pig3collisioncomplete==0)
			{
				iniblock9 -= (u * cos(DEG2RAD(teta)) * (0.05));
				block9_rotate -= 3;
				if(/*xpig3 - iniblock9 - 55 <= 5 && xpig3 - iniblock9 - 55 >= -5 && */block9_rotate == -78)
				{	
					pig3collisioncomplete = 1;
					pig3collide = 0;
					pig3flag = 1;
					score = score + 100;
					//		cout << pig3flag << '\n';
				}
				//cout << "block9rotate" << block9_rotate << '\n';
			}	
			else if(pig5collide == 1 && pig5collisioncomplete==0)
			{
				iniblock10 -= (u * cos(DEG2RAD(teta)) * (0.05));
				block10_rotate -= 3;
				if(/*xpig3 - iniblock9 - 55 <= 5 && xpig3 - iniblock9 - 55 >= -5 && */block10_rotate == -78)
				{	
					pig5collisioncomplete = 1;
					pig5collide = 0;
					pig5flag = 1;
					score = score + 100;
					//		cout << pig3flag << '\n';
				}
				//	cout << "block9rotate" << block9_rotate << '\n';
			}	
			else
			{
				if(vertblock + 260 < 5 && vertblock+260 >= -5){
					if(times == 0){
						x = horblock;	
						y = -260;
						z = 0;			
						t = 0.1;
						starttime = glfwGetTime();
					}
					times = 1;
					z = 0;
					if( teta >=90)
					{
						horiu = u * cos(DEG2RAD(teta))*e + (0.6 * 9.8 * t);
						r = u * t * cos(DEG2RAD(teta))*e + (4.9 * t * t * 0.6);
					}
					else if(teta < 90)
					{
						horiu = u * cos(DEG2RAD(teta)) - (0.6 * 9.8 * t);
						r = u * t * cos(DEG2RAD(teta)) - (4.9 * t * t * 0.6);
					}
					if( x+r <= -860 || (horiu <= 1 && horiu >= -1))	
						speednotsuff = 1;
					//		cout << "horiu" << horiu << '\n';
					//		cout << "x+r" << x + r << '\n';
					//		cout << "speednotsufF" << speednotsuff << '\n';
				}
				//cout << "vertblock =" << vertblock << '\n';
				//	cout << "horblock =" << horblock << '\n';
				//cout << "u=" << u << '\n';*/
				if(pig7disappear!=1)
				{
					if(((vertblock+130) <= 10 && (vertblock+130)>= -10 ) && (horblock <=360  && horblock >=260) && checkonce7hor==0 && (u*cos(DEG2RAD(teta))>= 30 ))
					{
						//cout << "rebounding" ;
						///times = 0;
						score = score + 100;
						pig7collide = 1;
						z = 0;
						r = 0;
						y = y-5;
						x = horblock;
						t = 0.1;
						starttime = glfwGetTime();
						/*checkonce7 = 1;
						  teta = 180;
						  y = vertblock;
						  z = 0;
						  t = 0.1;
						  x = 110;
						  r = 0;*/
					}
				}
					if(((vertblock - 40) <= 5 && (vertblock - 40)>=  -4 ) && (horblock <= 900  && horblock >= 300) && checkonce4hor2==0 && (u*cos(DEG2RAD(teta))>= 30 ) )
					{


						x = horblock;
						r = 0;
						e = 0.5;
						z = 0;
						y = vertblock;
						t = 0.1;
						starttime = glfwGetTime();
						checkonce4hor2 = 1;

					}
					if(((vertblock - (iniblock11ver+30)) <= 5 && (vertblock - (iniblock11ver+30))>=  -5 ) && (horblock <= iniblock11+350  && horblock >= iniblock11) && checkonce4hor3==0 && (u*cos(DEG2RAD(teta))>= 30) )
					{
						x = horblock;
						r = 0;
						e = 0.5;
						z = 0;
						y = vertblock;
						t = 0.1;
						starttime = glfwGetTime();
						checkonce4hor3 = 1;
					}
					if(((vertblock - (iniblock11ver)) <= 5 && (vertblock - (iniblock11ver))>=  -4 ) && (horblock <= iniblock11+350  && horblock >= iniblock11) && checkonce4hor4==0 && (u*cos(DEG2RAD(teta))>= 30))
					{
						x = horblock;
						r = 0;
						e = 0.5;
						z = 0;
						y = vertblock;
						teta = -teta;
						t = 0.1;
						starttime = glfwGetTime();
						checkonce4hor4 = 1;

					}
					if(((vertblock+130) <= 10 && (vertblock+130)>= -10 ) && (horblock <260  && horblock >=150) && checkonce7hor2==0 && (u*cos(DEG2RAD(teta))>= 30 ) )
					{
						x = horblock;
						r = 0;
						e = 0.5;
						z = 0;
						y = vertblock;
						t = 0.1;
						starttime = glfwGetTime();
						checkonce7hor2 = 1;
						//cout << "rebounding" ;
						///times = 0;
						/*pig7collide = 1;
						  z = 0;
						  r = 0;
						  y = y-5;
						  x = horblock;
						/*checkonce7 = 1;
						teta = 180;
						y = vertblock;
						z = 0;
						t = 0.1;
						x = 110;
						r = 0;*/

					}
				
				float diffblock7 = sqrt(pow((horblock - 110),2) + pow((vertblock + 170),2)) - 40;
				if(((horblock-110) <= 10 && (horblock-110)>= -10 ) && (vertblock <= -170 && vertblock >=-200) && checkonce7==0 && (u*cos(DEG2RAD(teta))>= 30 ) || (diffblock7 <= 5 && diffblock7 >= -5 && checkonce7 == 0))
				{
					//	cout << "rebounding777" ;
					times = 0;
					checkonce7 = 1;
					teta = 180;
					y = vertblock;
					z = 0;
					t = 0.1;
					starttime = glfwGetTime();
					x = 110;
					e = 0.5;
					r = 0;
				}
				//	if(diffblock7 <= 5 && diffblock7 >= -5){

				if(pig3flag == 0)
				{
					if((horblock-(iniblock9-40) <= 10) && (horblock-(iniblock9-40) >= -10) && (vertblock <= (iniblock9ver+100) && vertblock >= iniblock9ver) && checkonce9==0 && (u*cos(DEG2RAD(teta))>= 30 ) && pig3flag == 0 && block9died!=1 && pig3collisioncomplete!=1)
					{
						//	cout << "rebounding" ;
						times = 0;
						checkonce9 = 1;
						teta = 180;
						y = vertblock;
						pig3collide = 1;
						z = 0;
						t = 0.1;
						block9fall = 1;
						starttime = glfwGetTime();
						x = iniblock9;
						e = 0.5;
						r = 0;
					}
				}

				if((horblock-(iniblock9-40) <= 10) && (horblock-(iniblock9-40) >= -10) && (vertblock <= (iniblock9ver+100) && vertblock >= iniblock9ver) && checkonce13==0 && (u*cos(DEG2RAD(teta))>= 30) && block9died!=1 && block9fall==0){
					yay2 = 1;
					checkonce13 = 1;
					block9fall = 1;

				}


				if((horblock-(iniblock10-40) <= 10) && (horblock-(iniblock10-40) >= -10) && (vertblock <= 150 && vertblock >=0) && checkonce10==0 && (u*cos(DEG2RAD(teta))>= 30 ) && pig5flag == 0 && block10died!=1 && pig5collisioncomplete!=1)
				{
					//	cout << "rebounding" ;
					times10 = 0;
					checkonce10 = 1;
					teta = 180;
					y = vertblock;
					pig5collide = 1;
					z = 0;
					t = 0.1;
					starttime = glfwGetTime();
					x = iniblock10;
					block10fall = 1;
					e = 0.5;
					r = 0;
				}

				if((horblock-(iniblock10-40) <= 10) && (horblock-(iniblock10-40) >= -10) && (vertblock <= 150 && vertblock >=0) && checkonce12==0 && (u*cos(DEG2RAD(teta))>= 30 ) && block10died!=1 && block10fall ==0)
				{
					yay = 1;
					checkonce12 = 1;
					block10fall = 1;

				}

				float diffblock4 = sqrt(pow((horblock - 260),2) + pow((vertblock - 40),2)) - 40;
				float diffblock42 = sqrt(pow((horblock - 260),2) + pow((vertblock - 0),2)) - 40;
				if(((horblock-260) <= 5 && (horblock-260)>= -5 ) && (vertblock <= 40 && vertblock >=0) && checkonce4==0 && (u*cos(DEG2RAD(teta))>= 30 )|| (diffblock4 <= 3 && diffblock4 >= -3 && checkonce4 == 0) || (diffblock42 <=3 && diffblock42 >= -3 && checkonce4 ==0))
				{
					//		cout << "rebounding";
					times = 0;
					checkonce4 = 1;
					teta = 180;
					y = vertblock;
					z = 0;
					t = 0.1;
					starttime = glfwGetTime();
					xpig5 = xpig5 - 50;
					xpig3 = xpig3 - 50;
					inipig4hor -= 50;
					if(block9died!=1 && pig3flag==0)
						iniblock9 -= 50;
					if(block10died!=1 && pig5flag==0)
						iniblock10 -= 50;
					inipig2hor += 20;
					iniblock11 += 30;
					if(pig3flag == 1 && pig5flag == 1)
					{
						inipig2hor -= 20;
						iniblock11 -= 30;
					}
					x = 260;
					e = 0.5;
					r = 0;
				}
				float diffblock111 = sqrt(pow((horblock - iniblock11-40),2) + pow((vertblock - (iniblock11ver+30)),2)) - 40;
				float diffblock112 = sqrt(pow((horblock - iniblock11-40),2) + pow((vertblock - iniblock11ver),2)) - 40;
				if(((horblock-iniblock11-40) <= 3 && (horblock-iniblock11-40)>= -3 ) && (vertblock <= (iniblock11ver+30) && vertblock >= iniblock11ver) && checkonce11==0 && (u*cos(DEG2RAD(teta))>= 30 )|| (diffblock111 <= 3 && diffblock111 >= -3 && checkonce11 == 0) || (diffblock112 <=3 && diffblock112 >= -3 && checkonce11 ==0))
				{
					//		cout << "rebounding" ;
					times = 0;
					checkonce11= 1;
					teta = 180;
					y = vertblock;
					z = 0;
					t = 0.1;
					starttime = glfwGetTime();
					x = 360;
					e = 0.5;
					r = 0;
				}
				if(((horblock-360) <= 10 && (horblock-360)>= -10 ) && (vertblock <= 40 && vertblock >=-300) && checkonce6==0 && (u*cos(DEG2RAD(teta))>= 30) )
				{
					//		cout << "rebounding" ;
					times= 0;
					checkonce6 = 1;
					teta = 180;
					y = vertblock;
					e = 0.5;
					z = 0;
					t = 0.1;
					starttime = glfwGetTime();
					x = 360;
					r = 0;
				}
				/*	else if(((horblock-360) <= 10 && (horblock-360)>= -10 ) && (vertblock <= 40 && vertblock >=-300) && checkonce6==0 && (u*cos(DEG2RAD(teta))< 30 ))
					{		
					times = 0;
					checkonce6 = 1;
					speednotsuff = 1;
					x = 360;
					r = 0;
					z = 0;

					}*/
				if(((horblock-150) <= 10 && (horblock-150)>= -10 ) && (vertblock <= -200 && vertblock >=-300) && checkonce8==0 && (u*cos(DEG2RAD(teta))>= 30 ) )
				{
					//		cout << "rebounding" ;
					times = 0;

					e = 0.5;
					checkonce8 = 1;
					teta = 180;
					t = 0.1;
					starttime = glfwGetTime();
					x = 150;
					y = vertblock;
					z = 0;
					r = 0;
				}
				if(((horblock-270) <= 10 && (horblock-270)>= -10 ) && (vertblock <= -200 && vertblock >=-300) && checkonce82==0 && (u*cos(DEG2RAD(teta))>= 30 ) )
				{
					//		cout << "rebounding" ;
					times = 0;
					e = 0.5;
					checkonce82 = 1;
					teta = 180;
					t = 0.1;
					starttime = glfwGetTime();
					x = 150;
					y = vertblock;
					z = 0;
					r = 0;
				}
				if(((iniblock9+40) <= iniblock11 || (iniblock10+40 <= iniblock11)) && cominghere==0)
				{
					cominghere=0;
					pig5collisioncomplete = 1;
					basegone = 1;
				}
				/*else if(((horblock-150) <= 10 && (horblock-150)>= -10 ) && (vertblock <= -200 && vertblock >=-300) && checkonce8==0 && (u*cos(DEG2RAD(teta))< 30 ))
				  {		
				  times = 0;
				  checkonce8 = 1;
				  speednotsuff = 1;
				  x = 150;
				  r = 0;
				  z = 0;

				  }*/

			}
			}	
			//cout << "q=" << q << '\n';
			//cout << basegone << endl; 
			if(((pig3flag == 1 && pig5flag == 1) || (basegone==1)) && yes == 0)
			{	
				if(timescheck == 0){
					if(basegone == 1)
						score = score+100;
					tlast = 0;
				}
				timescheck = 1;
				tlast += 0.1;
				inipig2ver -= 4.9*tlast*tlast;
				iniblock11ver -= 4.9*tlast*tlast;
				flaggyfally = 1;
				if(basegone == 1)
				{

					if(iniblock11ver-inipig5ver <=5 && iniblock11ver-inipig5ver >= -5)
						pig5flag2 = 1;}
				if(iniblock11ver-40 <=5){
					flaggyfally = 0;
					yes = 1;
				}
			}

			if((q > 2600 || vertblock < -1560 or speednotsuff==1) and pig6collide==0 and pig2collide == 0 and pig4collide == 0 && pig7collide == 0 && pig3collide == 0 && block9finish == 0 && block10finish == 0 && pig3birdcollide==0 && pig5collide==0 && pig5birdcollide == 0 && flaggyfally == 0 && yay == 0 && yay2 == 0) {
				//		cout << "entering" << '\n';
				//lifeflag = 1;
				lifes = lifes+1;
				flag = 0;
				usleep(500000);
				teta = tetacannon;
				speednotsuff = 0;
				pig5collide = 0;
				checkonce4 = 0;
				checkonce6 = 0;
				checkonce7 = 0;
				checkonce7hor = 0;
				checkonce8 = 0;
				checkonce9 = 0;
				checkonce11 = 0;
				u = prevu;
				ang = 0;
				smokehor = 0.5;
				smokever = 0.5;
				times2 = 0;
				checkonce82 = 0;
				checkonce7hor2 = 0;
				checkonce4hor2 = 0;
				times = 0;
				smokex = -670;
				smokey = 275;
				pig7collide = 0;
				pig6collide = 0;
				pig2collide = 0;
				e = 1;
				pig4collide = 0;
				count = 0;
				block10collide =0;
				r = 0;
				z = 0;
				t = 0;
				starttime = glfwGetTime();
				/*if(tetacannon == 0){
				  x = x-2;
				  y = -133.f;}
				  else*/

				x = -840.0f + 200 * cos(DEG2RAD(tetacannon));//*(0.5) + (250 * cos(DEG2RAD(teta)) * (0.5)); 
				y = -140.f + 220 * sin(DEG2RAD(tetacannon));
				if(tetacannon < 0)
					y = -140.f + 180 * sin(DEG2RAD(tetacannon));//* (0.8) + (220 * sin(DEG2RAD(teta)) * (0.4));
				else if(tetacannon >= 0 && tetacannon <= 10)
					y = -140.f + 280 * sin(DEG2RAD(tetacannon));

			}

			// Load identity to model matrix


			/* Render your scene */

			// rotate about vector (1,0,0)
		}
		//projectile
		//Increment angles
		/* float increments = 1;
		   triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;*/

		//  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
		//rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;

		//  draws the VAO given to it using current MVP matrix
		//  draw3DObject(triangle);
		// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
		// glPopMatrix ();
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
		glm::mat4 translateRectangle = glm::translate (glm::vec3(-840,-140,0));        // glTranslatef
		// rotate about vector (-1,1,1)
		Matrices.model *= translateRectangle * rotateRectangle;
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// draw3DObject draws the VAO given to it using current MVP matrix
		draw3DObject(rectangle);
		/* Matrices.model = glm::mat4(1.0f);
		   glm::mat4 translatebase = glm::translate (glm::vec3(-430,-430,0));        // glTranslatef
		// rotate about vector (-1,1,1)
		Matrices.model *= translatebase;
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// draw3DObject draws the VAO given to it using current MVP matrix
		draw3DObject(base); */
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translatecircle = glm::translate (glm::vec3(x + r, y + z ,0.0f)); 

		glm::mat4 circleTransform = translatecircle; //* rotatecircle;

		Matrices.model *= circleTransform; 
		MVP = VP * Matrices.model; // MVP = p * V * M

		//  Don't change unless you are sure!!
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(circle); 

		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translatetrees = glm::translate (glm::vec3(smokex, smokey, 0));
		glm :: mat4 scaletrees = glm::scale (glm::vec3(smokehor,smokever,1));     // glTranslatef
		Matrices.model *= translatetrees * scaletrees; 
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(tree1);

		Matrices.model = glm::mat4(1.0f);

		//glm::mat4 rotatee = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
		glm::mat4 translatetrees6 = glm::translate (glm::vec3(-670, 275, 0));        // glTranslatef
		// rotate about vector (-1,1,1)
		Matrices.model *= translatetrees6; //* rotateRectangle;
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// draw3DObject draws the VAO given to it using current MVP matrix
		draw3DObject(tree6);

		if(pig3flag == 0){
			Matrices.model = glm::mat4(1.0f);
			// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
			glm::mat4 translatepig3 = glm::translate (glm::vec3(xpig3, inipig3ver, 0.0f)); // glTranslatef


			glm::mat4 Transformpig3 = translatepig3; //* rotateblock2;

			Matrices.model *= Transformpig3; 
			MVP = VP * Matrices.model; // MVP = p * V * M

			//  Don't change unless you are sure!!
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			//camera_rotation_angle++; // Simulating camera rotation
			draw3DObject(pig3);} 

		if(pig5flag==0 && pig5flag2 ==0)
		{
			Matrices.model = glm::mat4(1.0f);
			// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 

			glm::mat4 translatepig5 = glm::translate (glm::vec3(xpig5,inipig5ver, 0.0f)); // glTranslatef
			glm::mat4 Transformpig5 = translatepig5; //* rotateblock2;

			Matrices.model *= Transformpig5; 
			MVP = VP * Matrices.model; // MVP = p * V * M

			//  Don't change unless you are sure!!
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			//camera_rotation_angle++; // Simulating camera rotation
			draw3DObject(pig5);
		} 

		if(flag == 0)
		{
			for(int j=0;j<u;j=j+1)
			{	

				Matrices.model = glm::mat4(1.0f);
				//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); 
				glm::mat4 translaterect = glm::translate (glm::vec3(-950,(-200+j*2),0));        // glTranslatef
				// rotate about vector (-1,1,1)
				Matrices.model *= translaterect;// * rotateRectangle;
				MVP = VP * Matrices.model;
				glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
				// draw3DObject draws the VAO given to it using current MVP matrix
				draw3DObject(speedrect);
			}
		}

		if(basegone == 1 && timesparks<40)
		{
		timesparks++;
		Matrices.model = glm::mat4(1.0f);
			// glm::mat4 rotateblock2 = glm::rotate((float)(270*M_PI/180.0f), glm::vec3(0,0,1)); 
			glm::mat4 translatesparks = glm::translate (glm::vec3(650, 40, 0.0f)); // glTranslatef


			glm::mat4 Transformsparks = translatesparks; //* rotateblock2;

			Matrices.model *= Transformsparks; 
			MVP = VP * Matrices.model; // MVP = p * V * M

			//  Don't change unless you are sure!!
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			//camera_rotation_angle++; // Simulating camera rotation
			draw3DObject(sparks); 
		}
	}


	/* Initialise glfw window, I/O callbacks and the renderer to use */
	/* Nothing to Edit here */
	GLFWwindow* initGLFW (int width, int height)
	{
		GLFWwindow* window; // window desciptor/handle

		glfwSetErrorCallback(error_callback);
		if (!glfwInit()) {
			exit(EXIT_FAILURE);
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

		if (!window) {
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(window);
		gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
		glfwSwapInterval( 1 );

		/* --- register callbacks with GLFW --- */

		/* Register function to handle window resizes */
		/* With Retina display on Mac OS X GLFW's FramebufferSize
		   is different from WindowSize */
		glfwSetFramebufferSizeCallback(window, reshapeWindow);
		glfwSetWindowSizeCallback(window, reshapeWindow);

		/* Register function to handle window close */
		glfwSetWindowCloseCallback(window, quit);

		/* Register function to handle keyboard input */
		glfwSetKeyCallback(window, keyboard);      // general keyboard input
		glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

		/* Register function to handle mouse click */
		glfwSetMouseButtonCallback(window, mouseButton);
		glfwSetScrollCallback(window, scrollback);	
		// glfwSetCursorPosCallback(window,mouse);  // mouse button clicks

		return window;
	}

	/* Initialize the OpenGL rendering properties */
	/* Add all the models to be created here */
	void initGL (GLFWwindow* window, int width, int height)
	{
		/* Objects should be created before any other gl function and shaders */
		// Create the models
		createbackground();
		createbackground2();
		createbackground3();
		createbaseofcannon();//polygon

		createTriangle();//cannon // Generate the VAO, VBOs, vertices data & copy into the array buffer
		rectangle = createRectangle (-30,0,190,0,220,50,-30,50,0.239,0.239,0.239); //
		createbase();//hill
		block1 = createblocks(0,0,40,0,40,70,0,70,0.239,0.239,0.239);//small pig standing on it
		block2 = createblocks(0,0,0,320,-40,320,-40,40,0.239,0.239,0.239); //right trapezium 1
		block3 = createblocks(0,0,0,240,-40,240,-40,40,0.239,0.239,0.239);//right trapezium 2
		block4 = createblocks(0,0,600,0,600,40,0,40,0.239,0.239,0.239); //bigger horizontal
		block5 = createblocks(0,0,0,240,-40,240,-40,-40,0.239,0.239,0.239); //left trapezium
		block6 = createblocks(0,0,40,0,40,340,0,340,0.239,0.239,0.239); //left big bar
		block7 = createblocks(0,0,250,0,250,30,0,30,0.239,0.239,0.239); //below horizontal
		block8 = createblocks(0,0,40,0,40,100,0,100,0.239,0.239,0.239);//left most vertical
		block9 = createblocks(0,0,40,0,40,120,0,120,0.239,0.239,0.239);//upward left vertical
		block10 = createblocks(0,0,40,0,40,120,0,120,0.239,0.239,0.239); //upward right vertical
		block11 = createblocks(0,0,350,0,350,30,0,30,0.239,0.239,0.239); //highest horizontal
		speedrect = createblocks(0,0,35,0,35,2,0,2,0,1,0);
		//pig1 = createTrees(25,20,0,1,0);//pigs//smallest not needed
		pig2 = createTrees(50,40,0,1,0);//highest
		pig3 = createTrees(60,40,0,1,0);//central one above fattest hor bar
		pig4 = createTrees(40,30,0,1,0);//left fat bar
		pig5 = createTrees(50,35,0,1,0);//right and fat bar
		pig6 = createTrees(70,50,0,1,0);//biggest left
		pig7 = createTrees(50,40,0,1,0);//pig on the ground
		circle = createTrees(40,40,1,0,0);//bird
		sparks = createSparks(100,20);
		tree1 = createTrees(50,35,0.619,0.619,0.619);//clouds
		tree2 = createTrees(45,35,1,1,1);
		tree3 = createTrees(45,35,1,1,1);
		tree4 = createTrees(45,35,1,1,1);
		tree5 = createTrees(45,35,1,1,1);
		tree6 = createTrees(45,35,1,1,1);
		lifecircle = createTrees(20,20,1,0,0);

		// Create and compile our GLSL program from the shaders
		programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
		// Get a handle for our "MVP" uniform
		Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


		reshapeWindow (window, width, height);

		// Background color of the scene
		glClearColor (1.0f, 1.0f, 1.0f, 1.0f); // R, G, B, A
		glClearDepth (1.0f);

		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LEQUAL);

		cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
		cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
		cout << "VERSION: " << glGetString(GL_VERSION) << endl;
		cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	}

	int main (int argc, char** argv)
	{


		GLFWwindow* window = initGLFW(width, height);

		initGL (window, width, height);

		double last_update_time = glfwGetTime(), current_time;

		/* Drawn loop */
		while (!glfwWindowShouldClose(window)) {



			// OpenGL Draw commands
			draw();

			// Swap Frame Buffer in double buffering
			glfwSwapBuffers(window);

			// Poll for Keyboard and mouse events
			glfwPollEvents();

			// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
			/*  current_time = glfwGetTime(); // Time in seconds
			    if ((current_time - last_update_time) >= 0.00001) { // atleast 0.5s elapsed since last frame
			// do something every 0.5 seconds ..
			last_update_time = current_time;*/
			//}
			//cout << "score " << score << '\n';
			if(score == 600){
				
				if(flag == 0)
					break;
			}
			if(lifes >= 10 && score < 600){
				
				//display result
				//some controls for last display
				if(flag == 0) 
					break;
			}	
			//}

	}
	if(score == 600)
	{
	cout << "SCORE=" << 600 << '\n';
	cout << "YOU WON" << '\n';
	glfwTerminate();
	EXIT_SUCCESS;


	}
	if(lifes >= 10 && score < 600){
	cout << "SCORE=" << score << '\n';
	cout << "YOU LOST" << '\n';
	glfwTerminate();
	EXIT_SUCCESS;
	}



}
