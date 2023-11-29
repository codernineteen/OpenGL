//
// Display a color cube
//
// Colors are assigned to each vertex and then the rasterizer interpolates
//   those colors across the triangles.  We us an orthographic projection
//   as the default projetion.

#include "cube.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/glew.h"

#define _CRT_SECURE_NO_WARNINGS
#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

enum eShadeMode { NO_LIGHT, GORAUD, PHONG, MODE};
int shadeMode = NO_LIGHT;
int isTexture = false;
int isFace = false;

glm::mat4 projectMat;
glm::mat4 viewMat;
glm::mat4 modelMat;

// matrix id
GLuint projectMatrixID;
GLuint viewMatrixID;
GLuint modelMatrixID;
// mode id
GLuint shadeModeID;
GLuint textureModeID;
GLuint faceID;

float incrementFactor = 0.0f;
float rotAngle = 0.0f;
float tailRot = 0.0f;

float upperLegRot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
float lowerLegRot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

typedef glm::vec4  color4;
typedef glm::vec4  point4;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVertices];
glm::vec4 normals[NumVertices];
glm::vec2 textureCoors[NumVertices];

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {
	point4(-0.5, -0.5, 0.5, 1.0),
	point4(-0.5, 0.5, 0.5, 1.0),
	point4(0.5, 0.5, 0.5, 1.0),
	point4(0.5, -0.5, 0.5, 1.0),
	point4(-0.5, -0.5, -0.5, 1.0),
	point4(-0.5, 0.5, -0.5, 1.0),
	point4(0.5, 0.5, -0.5, 1.0),
	point4(0.5, -0.5, -0.5, 1.0)
};


//----------------------------------------------------------------------------

// quad generates two triangles for each face and assigns colors
//    to the vertices
int Index = 0;
glm::vec4
quad(int a, int b, int c, int d)
{

	glm::vec3 normalVecPoint;
	glm::vec4 normalVec;

	// normal ∫§≈Õ ∞ËªÍ
	normalVecPoint = glm::cross(glm::vec3(vertices[b] - vertices[a]), glm::vec3(vertices[c] - vertices[a]));
	normalVec = glm::vec4(normalVecPoint.x, normalVecPoint.y, normalVecPoint.z, 0.0);
	glm::normalize(normalVec);

	// u
	points[Index] = vertices[a]; 
	normals[Index] = normalVec;
	textureCoors[Index] = glm::vec2(0.0, 1.0);
	Index++;

	// v
	points[Index] = vertices[b]; 
	normals[Index] = normalVec;
	textureCoors[Index] = glm::vec2(0.0, 0.0);
	Index++;

	// v1
	points[Index] = vertices[c]; 
	normals[Index] = normalVec;
	textureCoors[Index] = glm::vec2(1.0, 0.0);
	Index++;

	// u
	points[Index] = vertices[a];  
	normals[Index] = normalVec;
	textureCoors[Index] = glm::vec2(0.0, 1.0);
	Index++;

	// v1
	points[Index] = vertices[c];
	normals[Index] = normalVec;
	textureCoors[Index] = glm::vec2(1.0, 0.0);
	Index++;

	// v2
	points[Index] = vertices[d]; 
	normals[Index] = normalVec;
	textureCoors[Index] = glm::vec2(1.0, 1.0);
	Index++;

	return normalVec;
}


//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors
void
colorcube()
{
	glm::vec4 n1 = quad(1, 0, 3, 2);
	glm::vec4 n2 = quad(2, 3, 7, 6);
	glm::vec4 n3 = quad(3, 0, 4, 7);
	glm::vec4 n4 = quad(6, 5, 1, 2);
	glm::vec4 n5 = quad(4, 5, 6, 7);
	glm::vec4 n6 = quad(5, 4, 0, 1);
}

//----------------------------------------------------------------------------

// Load bmp texture

GLuint loadBMP_custom(const char* imagepath) {

	printf("Reading image %s\n", imagepath);

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char* data;

	// Open the file
	FILE* file = fopen(imagepath, "rb");
	if (!file) {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
		getchar();
		return 0;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    fclose(file); return 0; }
	if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    fclose(file); return 0; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	// Everything is in memory now, the file can be closed.
	fclose(file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete[] data;

	// Poor filtering, or ...
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	// ... nice trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// ... which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	printf("%d x %d image read.", width, height);

	// Return the ID of the texture we just created
	return textureID;
}
//----------------------------------------------------------------------------

// OpenGL initialization
void
init()
{
	colorcube();

	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals) + sizeof(textureCoors),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals);
	glBufferSubData(
		GL_ARRAY_BUFFER, 
		sizeof(points) + sizeof(normals), 
		sizeof(textureCoors), 
		textureCoors
	);

	// Load shaders and use the resulting shader program
	GLuint program = InitShader("src/vshader.glsl", "src/fshader.glsl");
	glUseProgram(program);

	// set up vertex arrays
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(points)));

	GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
	glEnableVertexAttribArray(vTexCoord);
	glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(points) + sizeof(normals)));

	projectMatrixID = glGetUniformLocation(program, "mProject");
    projectMat = glm::perspective(glm::radians(65.0f), 1.0f, 0.1f, 100.0f);
	glUniformMatrix4fv(projectMatrixID, 1, GL_FALSE, &projectMat[0][0]);

	viewMatrixID = glGetUniformLocation(program, "mView");
	viewMat = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMat[0][0]);

	modelMatrixID = glGetUniformLocation(program, "mModel");
	modelMat = glm::mat4(1.0f);
	glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMat[0][0]);

	shadeModeID = glGetUniformLocation(program, "shadeMode");
	glUniform1i(shadeModeID, shadeMode);

	textureModeID = glGetUniformLocation(program, "isTexture");
	glUniform1i(textureModeID, isTexture);

	faceID = glGetUniformLocation(program, "isFace");
	glUniform1i(textureModeID, isFace);

	GLuint Texture = loadBMP_custom("corgi-body.bmp");
	GLuint Texture2 = loadBMP_custom("corgi-face.bmp");

	GLuint TextureID = glGetUniformLocation(program, "corgiBodyTexture");
	GLuint Texture2ID = glGetUniformLocation(program, "corgiFaceTexture");

	// Bind body texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);

	// Set body "myTextureSampler" sampler to use Texture Unit 0
	glUniform1i(TextureID, 0);

	// Bind face texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, Texture2);

	// Set the uniform variable for the second texture
	glUniform1i(Texture2ID, 1);


	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

//--------------------------------
/*
*
* Draw dog shape and its animation
* 
*/

void glUniformPVMMatrix4fv() {
	glUniformMatrix4fv(projectMatrixID, 1, GL_FALSE, &projectMat[0][0]);
	glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMat[0][0]);
	glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMat[0][0]);
}

void drawDog(glm::mat4 bodyMat) {
	// root
	glm::vec3 newPos = glm::vec3(-incrementFactor, 0.0f, 0.0f);
	bodyMat = glm::translate(bodyMat,newPos);

	viewMat = glm::lookAt(glm::vec3(-20, 3, 5), newPos, glm::vec3(0, 2, 0)); // update view matrix

	// front upper legs
	glm::vec3 LFUpperLeg = glm::vec3(-1.5, -1.0, 1.5);
	glm::vec3 RFUpperLeg = glm::vec3(-1.5, -1.0, -1.5);
	// rear upper legs
	glm::vec3 LRUpperLeg = glm::vec3(1.5, -1.0, 1.5);
	glm::vec3 RRUpperLeg = glm::vec3(1.5, -1.0, -1.5);
	// lower leg offset
	glm::vec3 LowerLegPos = glm::vec3(0.0, 0.0, 1.0);

	glm::vec3 upperLegPos[4] = { glm::vec3(-1.5, -1.0, 1.5) , glm::vec3(-1.5, -1.0, -1.5), glm::vec3(1.5, -1.0, 1.5), glm::vec3(1.5, -1.0, -1.5) };


	// dog body
	modelMat = glm::scale(bodyMat, glm::vec3(1.5, 0.6, 0.6));

	glUniformPVMMatrix4fv();
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	// head
	modelMat = glm::scale(bodyMat, glm::vec3(1.5, 1.3, 1.3));
	modelMat = glm::translate(modelMat, glm::vec3(-0.8, 0.8, 0)); 

	glUniformPVMMatrix4fv();
	glDrawArrays(GL_TRIANGLES, 0, 30);
	isFace = true;
	glUniform1i(faceID, isFace);
	glDrawArrays(GL_TRIANGLES, 30, 6);
	isFace = false;
	glUniform1i(faceID, isFace);


	// tail
	modelMat = glm::rotate(bodyMat, glm::radians(-50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	modelMat = glm::rotate(modelMat, glm::radians(tailRot), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMat = glm::scale(modelMat, glm::vec3(0.2, 1, 0.2));
	modelMat = glm::translate(modelMat, glm::vec3(1.35, 1.2, 0.0));
	
	glUniformPVMMatrix4fv();
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	//leg shape

	glm::mat4 legMatParents[4];
	glm::mat4 legMatChildren[4];

	for (int i = 0; i < 4; i++) 
	{
		glm::mat4 legMat = glm::scale(bodyMat, glm::vec3(0.25, 0.55, 0.25));
		legMat = glm::translate(legMat, upperLegPos[i]);
		legMat = glm::rotate(legMat, glm::radians(upperLegRot[i]), glm::vec3(0.0f, 0.0f, 1.0f));
		legMatParents[i] = legMat;
		modelMat = legMatParents[i];
		glUniformPVMMatrix4fv();
		glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	}

	for (int i = 0; i < 4; i++)
	{
		glm::mat4 legMat = glm::rotate(legMatParents[i], glm::radians(lowerLegRot[i]), glm::vec3(0.0f, 0.0f, 1.0f));
		legMat = glm::translate(legMat, glm::vec3(0, -1.1, 0.2));
		legMatChildren[i] = legMat;
		modelMat = legMatChildren[i];
		glUniformPVMMatrix4fv();
		glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	}
}


void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	modelMat = glm::mat4(1.0f);

	drawDog(modelMat);


	glutSwapBuffers();
}

//----------------------------------------------------------------------------

void idle()
{
	static int prevTime = glutGet(GLUT_ELAPSED_TIME);
	int currTime = glutGet(GLUT_ELAPSED_TIME);

	if (abs(currTime - prevTime) >= 20)
	{
		float t = abs(currTime - prevTime);
		rotAngle += glm::radians(t*360.0f / 10000.0f);
		incrementFactor += (t / 250.0f);

		upperLegRot[0] = 30.0f * glm::cos(incrementFactor);
		upperLegRot[1] = -30.0f * glm::cos(incrementFactor);
		upperLegRot[2] = 30.0f * glm::cos(incrementFactor);
		upperLegRot[3] = -30.0f * glm::cos(incrementFactor);

		for (int i = 0; i < 4; i++)
		{
			float PI = glm::pi<float>();
			float cosRatio = glm::cos(incrementFactor);
			float sinRatio = glm::sin(incrementFactor - PI/2);
			if (cosRatio < 0.0f) cosRatio = 0;
			if (sinRatio < 0.0f) sinRatio = 0;

			if(i % 2 ==0)
				lowerLegRot[i] = -30.0f * sinRatio;
			else
				lowerLegRot[i] = -30.0f * cosRatio;
		}

		tailRot = 15.0f * glm::cos(incrementFactor);

		prevTime = currTime;
		glutPostRedisplay();
	}
}

//----------------------------------------------------------------------------

void
keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'l': case 'L':
		shadeMode = (++shadeMode % MODE);
		glUniform1i(shadeModeID, shadeMode);
		break;
	case 't': case 'T':
		isTexture = !isTexture;
		glUniform1i(textureModeID, isTexture);
		glutPostRedisplay();
		break;
	case 033:  // Escape key
	case 'q': case 'Q':
		exit(EXIT_SUCCESS);
		break;
	}
}

//----------------------------------------------------------------------------

void resize(int w, int h)
{
	float ratio = (float)w / (float)h;
	glViewport(0, 0, w, h);

	projectMat = glm::perspective(glm::radians(65.0f), ratio, 0.1f, 100.0f);

	glutPostRedisplay();
}

//----------------------------------------------------------------------------

int
main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutInitContextVersion(3, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow("Color Car");

	glewInit();

	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(resize);
	glutIdleFunc(idle);

	glutMainLoop();
	return 0;
}
