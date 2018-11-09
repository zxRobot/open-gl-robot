/*#include <stdio.h>
#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GraphicsObj.h"
#include "GLHelper.h"
#include "mat.h"
#include "Camera.h"
#include "ImageLib.h"
#include <stack>

class CSceneNode
{
public:
	CGraphicsObj *p_mesh;
	mat4 model_matrix;
	vec4 diffuse_color;
	GLuint texture;

	mat4 from_parent_matrix;
	mat4 instance_matrix;
	CSceneNode *p_child;
	CSceneNode *p_sibling;

	CSceneNode(void)
	{
		diffuse_color=vec4(1.0f);
	}
};

std::stack <mat4> g_matrix_stack;

enum {
	NODE_GROUND=0,
	NODE_BASE,
	NODE_LOWER_ARM,
	NODE_UPPER_ARM,
	NODE_LOWER_ARM_2,
	NODE_UPPER_ARM_2,
	NUM_NODES
};

enum {
	OBJ_RECT=0,
	OBJ_CUBE,
	NUM_OBJS
};

enum {
	JOINT_GROUND_TO_BASE=0,
	JOINT_BASE_TO_LARM,
	JOINT_LARM_TO_UARM,
	JOINT_BASE_TO_LARM2,
	JOINT_LARM2_TO_UARM2,
	NUM_JOINTS
};

CSceneNode g_scene_nodes[NUM_NODES];
CGraphicsObj g_scene_objects[NUM_OBJS];
float g_joint_angles[NUM_JOINTS]={0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

#define BASE_WIDTH 0.5f
#define BASE_HEIGHT 0.5f
#define LOWER_ARM_WIDTH 0.2f
#define LOWER_ARM_HEIGHT 1.0f
#define UPPER_ARM_WIDTH 0.1f
#define UPPER_ARM_HEIGHT 0.8f
#define ARM_DISTANCE 0.3f

GLuint g_simple_GLSL_prog;

int g_window_width, g_window_height;

CCamera g_camera;
float g_camera_move_step=0.1f;
bool g_camera_rotation_mode=false;
int g_mouse_pos[2];

float g_FOV=60.0f;

float g_rot_angle=0.0f;
float g_current_time;

int g_lights_enable[3]={1, 1, 0};

GLuint g_main_texture;

void AnimateRobot(void);

GLuint CreateCheckerboardTexture(void)
{
	GLubyte my_image[64][64][3];
	int i, j;
	for (j=0; j<64; ++j)
	{
		for (i=0; i<64; ++i)
		{
			if ((i/16)%2==(j/16)%2)
			{
				my_image[i][j][0]=255;
				my_image[i][j][1]=0;
				my_image[i][j][2]=0;
			}
			else
			{
				my_image[i][j][0]=255;
				my_image[i][j][1]=255;
				my_image[i][j][2]=255;
			}
		}
	}

	GLuint itex;
	glGenTextures(1, &itex);
	glBindTexture(GL_TEXTURE_2D, itex);
	glTexImage2D(GL_TEXTURE_2D, 0,
		GL_RGB, 64, 64, 0,
		GL_RGB, GL_UNSIGNED_BYTE, my_image);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//	float border_color[4]={0.0f, 0.0f, 1.0f, 1.0f};
//	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, 
//		border_color);
	glTexParameteri(GL_TEXTURE_2D, 
		GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, 
		GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	return itex;
}

void init_shaders(void)
{
	g_simple_GLSL_prog=InitShader(
		//"..\\shaders\\single_light-vs.txt",
		//"..\\shaders\\single_light-fs.txt"
		"..\\shaders\\multi_lights-multi_tex-vs.txt",
		"..\\shaders\\multi_lights-multi_tex-fs.txt"
		);
	glUseProgram(g_simple_GLSL_prog);

	int loc;
	mat4 M;
	loc=glGetUniformLocation(g_simple_GLSL_prog, "global_ambient_light");
	glUniform4f(loc, 0.2f, 0.2f, 0.2f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "specular_reflectivity");
	glUniform4f(loc, 0.8f, 0.8f, 0.8f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "shininess");
	glUniform1f(loc, 128.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "tex_map");
	glUniform1i(loc, 0);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "detail_map");
	glUniform1i(loc, 1);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[0].enable");
	glUniform1i(loc, g_lights_enable[0]);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[0].position");
	glUniform4f(loc, 1.0f, 1.5f, 2.0f, 0.0f);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[0].color");
	glUniform4f(loc, 1.0f, 1.0f, 1.0f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[1].enable");
	glUniform1i(loc, g_lights_enable[1]);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[1].attenuation");
	glUniform3f(loc, 1.0f, 0.0f, 0.01f);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[1].color");
	glUniform4f(loc, 0.0f, 1.0f, 0.0f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[2].enable");
	glUniform1i(loc, g_lights_enable[2]);
}

void init_models(void)
{
	g_scene_objects[OBJ_RECT].CreateRect(6.0f, 5.0f, 12, 10, 6.0f, 5.0f);
	g_scene_objects[OBJ_CUBE].CreateCube(1.0f, 1.0f, 1.0f, 1.0f);

	g_scene_nodes[NODE_GROUND].texture=LoadTexture2DFromFile(
		"..\\textures\\wood01.jpg");
	g_scene_nodes[NODE_BASE].texture=LoadTexture2DFromFile(
		"..\\textures\\metal01.jpg");
	g_scene_nodes[NODE_LOWER_ARM].texture=LoadTexture2DFromFile(
		"..\\textures\\stone01.jpg");
	g_scene_nodes[NODE_UPPER_ARM].texture=LoadTexture2DFromFile(
		"..\\textures\\glass01.jpg");
	g_scene_nodes[NODE_LOWER_ARM_2].texture=LoadTexture2DFromFile(
		"..\\textures\\cloth01.jpg");
	g_scene_nodes[NODE_UPPER_ARM_2].texture=LoadTexture2DFromFile(
		"..\\textures\\rock03.jpg");

	g_scene_nodes[NODE_GROUND].p_mesh=&g_scene_objects[OBJ_RECT];
	g_scene_nodes[NODE_BASE].p_mesh=&g_scene_objects[OBJ_CUBE];
	g_scene_nodes[NODE_LOWER_ARM].p_mesh=&g_scene_objects[OBJ_CUBE];
	g_scene_nodes[NODE_UPPER_ARM].p_mesh=&g_scene_objects[OBJ_CUBE];
	g_scene_nodes[NODE_LOWER_ARM_2].p_mesh=&g_scene_objects[OBJ_CUBE];
	g_scene_nodes[NODE_UPPER_ARM_2].p_mesh=&g_scene_objects[OBJ_CUBE];

	g_scene_nodes[NODE_GROUND].p_child=&g_scene_nodes[NODE_BASE];
	g_scene_nodes[NODE_GROUND].p_sibling=NULL;
	g_scene_nodes[NODE_BASE].p_child=&g_scene_nodes[NODE_LOWER_ARM];
	g_scene_nodes[NODE_BASE].p_sibling=NULL;
	g_scene_nodes[NODE_LOWER_ARM].p_child=&g_scene_nodes[NODE_UPPER_ARM];
	g_scene_nodes[NODE_LOWER_ARM].p_sibling=&g_scene_nodes[NODE_LOWER_ARM_2];
	g_scene_nodes[NODE_UPPER_ARM].p_child=NULL;
	g_scene_nodes[NODE_UPPER_ARM].p_sibling=NULL;
	g_scene_nodes[NODE_LOWER_ARM_2].p_child=&g_scene_nodes[NODE_UPPER_ARM_2];
	g_scene_nodes[NODE_LOWER_ARM_2].p_sibling=NULL;
	g_scene_nodes[NODE_UPPER_ARM_2].p_child=NULL;
	g_scene_nodes[NODE_UPPER_ARM_2].p_sibling=NULL;

	g_scene_nodes[NODE_GROUND].from_parent_matrix=mat4();
	g_scene_nodes[NODE_GROUND].instance_matrix=mat4();
	g_scene_nodes[NODE_BASE].instance_matrix=
		Translate(0.0f, 0.0f, 0.5f*BASE_HEIGHT)
		*Scale(BASE_WIDTH, BASE_WIDTH, BASE_HEIGHT);
	g_scene_nodes[NODE_LOWER_ARM].instance_matrix=
		Translate(0.0f, 0.0f, 0.5f*LOWER_ARM_HEIGHT)
		*Scale(LOWER_ARM_WIDTH, LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT);
	g_scene_nodes[NODE_UPPER_ARM].instance_matrix=
		Translate(0.0f, 0.0f, 0.5f*UPPER_ARM_HEIGHT)
		*Scale(UPPER_ARM_WIDTH, UPPER_ARM_WIDTH, UPPER_ARM_HEIGHT);
	g_scene_nodes[NODE_LOWER_ARM_2].instance_matrix=
		Translate(0.0f, 0.0f, 0.5f*LOWER_ARM_HEIGHT)
		*Scale(LOWER_ARM_WIDTH, LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT);
	g_scene_nodes[NODE_UPPER_ARM_2].instance_matrix=
		Translate(0.0f, 0.0f, 0.5f*UPPER_ARM_HEIGHT)
		*Scale(UPPER_ARM_WIDTH, UPPER_ARM_WIDTH, UPPER_ARM_HEIGHT);
}

void init(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	printf("Vender: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));

	init_shaders();

	init_models();

	g_main_texture=CreateCheckerboardTexture();

	g_camera.Init(vec3(0.0f, 6.0f, 1.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f));

	g_current_time=0.001f*glutGet(GLUT_ELAPSED_TIME);

	AnimateRobot();
}

void TraverseSceneNode(CSceneNode *p_node, mat4& current_matrix)
{
	if (p_node==NULL) return;
	g_matrix_stack.push(current_matrix);
	current_matrix=current_matrix*p_node->from_parent_matrix;
	p_node->model_matrix=current_matrix*p_node->instance_matrix;
	if (p_node->p_child!=NULL)
		TraverseSceneNode(p_node->p_child, current_matrix);
	current_matrix=g_matrix_stack.top();
	g_matrix_stack.pop();
	if (p_node->p_sibling!=NULL)
		TraverseSceneNode(p_node->p_sibling, current_matrix);
}

void AnimateRobot(void)
{
	g_scene_nodes[NODE_BASE].from_parent_matrix=
		RotateZ(g_joint_angles[JOINT_GROUND_TO_BASE]);
	g_scene_nodes[NODE_LOWER_ARM].from_parent_matrix=
		Translate(-0.5f*ARM_DISTANCE, 0.0f, BASE_HEIGHT)
		*RotateX(g_joint_angles[JOINT_BASE_TO_LARM]);
	g_scene_nodes[NODE_UPPER_ARM].from_parent_matrix=
		Translate(0.0f, 0.0f, LOWER_ARM_HEIGHT)
		*RotateX(g_joint_angles[JOINT_LARM_TO_UARM]);
	g_scene_nodes[NODE_LOWER_ARM_2].from_parent_matrix=
		Translate(0.5f*ARM_DISTANCE, 0.0f, BASE_HEIGHT)
		*RotateX(g_joint_angles[JOINT_BASE_TO_LARM2]);
	g_scene_nodes[NODE_UPPER_ARM_2].from_parent_matrix=
		Translate(0.0f, 0.0f, LOWER_ARM_HEIGHT)
		*RotateX(g_joint_angles[JOINT_LARM2_TO_UARM2]);
	mat4 M;
	TraverseSceneNode(&g_scene_nodes[NODE_GROUND], M);
}

void reshape(int w, int h)
{
	g_window_width=w;
	g_window_height=h;

	glViewport(0, 0, w, h);
}

void display(void)
{
	int loc;
	mat4 M;
	mat3 Mn;

	g_camera.GetViewMatrix(M);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "view_matrix");
	glUniformMatrix4fv(loc, 1, GL_TRUE, M);

	M=Perspective(g_FOV, (float)g_window_width/(float)g_window_height,
		0.01f, 20.0f);
	glUseProgram(g_simple_GLSL_prog);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "projection_matrix");
	glUniformMatrix4fv(loc, 1, GL_TRUE, M);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_main_texture);

	glActiveTexture(GL_TEXTURE0);
	for (int i=0; i<NUM_NODES; ++i)
	{
		M=g_scene_nodes[i].model_matrix;

		loc=glGetUniformLocation(g_simple_GLSL_prog, "model_matrix");
		glUniformMatrix4fv(loc, 1, GL_TRUE, M);

		Mn=Normal(M);
		loc=glGetUniformLocation(g_simple_GLSL_prog, "normal_matrix");
		glUniformMatrix3fv(loc, 1, GL_TRUE, Mn);

		loc=glGetUniformLocation(g_simple_GLSL_prog, "diffuse_reflectivity");
		glUniform4fv(loc, 1, g_scene_nodes[i].diffuse_color);

		loc=glGetUniformLocation(g_simple_GLSL_prog, "enable_tex_map");
		glUniform1i(loc, g_scene_nodes[i].texture!=0);

		if (g_scene_nodes[i].texture!=0)
			glBindTexture(GL_TEXTURE_2D, g_scene_nodes[i].texture);

		g_scene_nodes[i].p_mesh->Draw();
	}

	glFlush();
}

void mouse(int button, int state, int x, int y)
{
	if (button==GLUT_RIGHT_BUTTON)
	{
		if (state==GLUT_DOWN)
		{
			g_camera_rotation_mode=true;
			g_mouse_pos[0]=x;
			g_mouse_pos[1]=y;
		}
		else
		{
			g_camera_rotation_mode=false;
		}
	}
}

void mouse_motion(int x, int y)
{
	if (g_camera_rotation_mode)
	{
		int dx=x-g_mouse_pos[0];
		int dy=y-g_mouse_pos[1];
		g_mouse_pos[0]=x;
		g_mouse_pos[1]=y;
		g_camera.TurnLeft(-1.0f*dx);
		g_camera.LookUp(-1.0f*dy);
		glutPostRedisplay();
	}
}

void keyboard(unsigned char key, int x, int y)
{
	const float joint_angle_delta=5.0f;
	switch(key)
	{
	case 'w':
	case 'W':
		g_camera.MoveForward(g_camera_move_step);
		glutPostRedisplay();
		break;
	case 's':
	case 'S':
		g_camera.MoveForward(-g_camera_move_step);
		glutPostRedisplay();
		break;
	case 'a':
	case 'A':
		g_camera.MoveLeft(g_camera_move_step);
		glutPostRedisplay();
		break;
	case 'd':
	case 'D':
		g_camera.MoveLeft(-g_camera_move_step);
		glutPostRedisplay();
		break;
	case 'r':
	case 'R':
		g_camera.MoveUp(g_camera_move_step);
		glutPostRedisplay();
		break;
	case 'f':
	case 'F':
		g_camera.MoveUp(-g_camera_move_step);
		glutPostRedisplay();
		break;
	case '1':
		g_joint_angles[JOINT_GROUND_TO_BASE]=
			fmod(g_joint_angles[JOINT_GROUND_TO_BASE]
				+joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '2':
		g_joint_angles[JOINT_GROUND_TO_BASE]=
			fmod(g_joint_angles[JOINT_GROUND_TO_BASE]
				-joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '3':
		g_joint_angles[JOINT_BASE_TO_LARM]=
			fmod(g_joint_angles[JOINT_BASE_TO_LARM]
				+joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '4':
		g_joint_angles[JOINT_BASE_TO_LARM]=
			fmod(g_joint_angles[JOINT_BASE_TO_LARM]
				-joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '5':
		g_joint_angles[JOINT_LARM_TO_UARM]=
			fmod(g_joint_angles[JOINT_LARM_TO_UARM]
				+joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '6':
		g_joint_angles[JOINT_LARM_TO_UARM]=
			fmod(g_joint_angles[JOINT_LARM_TO_UARM]
				-joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '7':
		g_joint_angles[JOINT_BASE_TO_LARM2]=
			fmod(g_joint_angles[JOINT_BASE_TO_LARM2]
				+joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '8':
		g_joint_angles[JOINT_BASE_TO_LARM2]=
			fmod(g_joint_angles[JOINT_BASE_TO_LARM2]
				-joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '9':
		g_joint_angles[JOINT_LARM2_TO_UARM2]=
			fmod(g_joint_angles[JOINT_LARM2_TO_UARM2]
				+joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	case '0':
		g_joint_angles[JOINT_LARM2_TO_UARM2]=
			fmod(g_joint_angles[JOINT_LARM2_TO_UARM2]
				-joint_angle_delta, 360.0f);
		AnimateRobot();
		glutPostRedisplay();
		break;
	}
}

void special_key(int key, int x, int y)
{
	int loc;

	switch(key)
	{
	case GLUT_KEY_PAGE_UP:
		g_FOV+=0.5f;
		if (g_FOV>178.0f) g_FOV=178.0f;
		glutPostRedisplay();
		break;
	case GLUT_KEY_PAGE_DOWN:
		g_FOV-=0.5f;
		if (g_FOV<1.0f) g_FOV=1.0f;
		glutPostRedisplay();
		break;
	case GLUT_KEY_F1:
		glUseProgram(g_simple_GLSL_prog);
		g_lights_enable[0]=!g_lights_enable[0];
		loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[0].enable");
		glUniform1i(loc, g_lights_enable[0]);
		glutPostRedisplay();
		break;
	case GLUT_KEY_F2:
		glUseProgram(g_simple_GLSL_prog);
		g_lights_enable[1]=!g_lights_enable[1];
		loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[1].enable");
		glUniform1i(loc, g_lights_enable[1]);
		glutPostRedisplay();
		break;
	}
}

void idle(void)
{
	float t=0.001f*glutGet(GLUT_ELAPSED_TIME);
	float dt=t-g_current_time;
	g_current_time=t;

	g_rot_angle+=1.0f*dt;
	g_rot_angle=fmod(g_rot_angle, 360.0f);

	vec3 P;
	P.x=4.0f*cos(g_rot_angle);
	P.y=4.0f*sin(g_rot_angle);
	P.z=3.0f;

	glUseProgram(g_simple_GLSL_prog);
	int loc=glGetUniformLocation(g_simple_GLSL_prog, "lights[1].position");
	glUniform4f(loc, P.x, P.y, P.z, 1.0f);

	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_SINGLE);
	glutInitWindowSize(640, 480);

	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow("3D Scene");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(mouse_motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special_key);
	glutIdleFunc(idle);

	glewExperimental=GL_TRUE;
	glewInit();

	init();

	glutMainLoop();

	return 1;
}

// 旋转的步进值
// SphereWorld.cpp
// OpenGL SuperBible
// New and improved (performance) sphere world
// Program by Richard S. Wright Jr.
#pragma comment(lib,"GLTools.lib")




#include <GLTools.h>
#include <GLShaderManager.h>
#include <GLFrustum.h>
#include <GLBatch.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>
#include <math.h>
#include <stdio.h>
#include <GL\freeglut.h>




#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];


GLShaderManagershaderManager;// Shader Manager
GLMatrixStackmodelViewMatrix(200);// Modelview Matrix
GLMatrixStackprojectionMatrix(200);// Projection Matrix
GLFrustumviewFrustum;// View Frustum
GLGeometryTransformtransformPipeline;// Geometry Transform Pipeline
GLFramecameraFrame;// Camera frame


GLTriangleBatchsunBatch;
GLTriangleBatchearthBatch;
GLTriangleBatch moonBatch;


GLBatchfloorBatch;


GLuintuiTextures[4];






void DrawSongAndDance(GLfloat yRot)// Called to draw dancing objects
{
static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat vLightPos[] = { 0.0f, 0.0f, 0.0f, 0.0f };

// Get the light position in eye space
M3DVector4fvLightTransformed;
M3DMatrix44f mCamera;
modelViewMatrix.GetMatrix(mCamera);
m3dTransformVector4(vLightTransformed, vLightPos, mCamera);

// Draw the light source
modelViewMatrix.PushMatrix();
modelViewMatrix.Translatev(vLightPos);
shaderManager.UseStockShader(GLT_SHADER_FLAT,
transformPipeline.GetModelViewProjectionMatrix(),
vWhite);
earthBatch.Draw();
modelViewMatrix.PopMatrix();

glBindTexture(GL_TEXTURE_2D, uiTextures[3]);
for(int i = 0; i < NUM_SPHERES; i++) {
modelViewMatrix.PushMatrix();
modelViewMatrix.MultMatrix(spheres[i]);
shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
modelViewMatrix.GetMatrix(),
transformPipeline.GetProjectionMatrix(),
vLightTransformed,
vWhite,
0);
moonBatch.Draw();
modelViewMatrix.PopMatrix();
}

// Song and dance
modelViewMatrix.Translate(0.0f, 0.0f, -4.0f);
modelViewMatrix.PushMatrix();// Saves the translated origin
//modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);

// Draw stuff relative to the camera
glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
modelViewMatrix.GetMatrix(),
transformPipeline.GetProjectionMatrix(),
vLightTransformed,
vWhite,
0);
sunBatch.Draw();
modelViewMatrix.PopMatrix(); // Erased the rotate


//问题代码,地球贴图部分
modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
modelViewMatrix.Translate(0.8f, 0.0f, -1.0f);


modelViewMatrix.PushMatrix();
modelViewMatrix.Rotate(yRot*2.7f, 0.0f, 1.0f, 0.0f);


glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,
modelViewMatrix.GetMatrix(),
transformPipeline.GetProjectionMatrix(),
vLightTransformed,
vWhite,
0);
earthBatch.Draw();
modelViewMatrix.PopMatrix();
//


modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
modelViewMatrix.Translate(0.0f, 0.0f, -0.2f);



glBindTexture(GL_TEXTURE_2D, uiTextures[3]);
shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
modelViewMatrix.GetMatrix(),
transformPipeline.GetProjectionMatrix(),
vLightTransformed,
vWhite,
0);
moonBatch.Draw();
}


bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode)
{
GLbyte *pBits;
int nWidth, nHeight, nComponents;
GLenum eFormat;

// Read the texture bits
pBits = gltReadTGABits(szFileName, &nWidth, &nHeight, &nComponents, &eFormat);
if(pBits == NULL)
return false;

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, nWidth, nHeight, 0,
eFormat, GL_UNSIGNED_BYTE, pBits);

free(pBits);


if(minFilter == GL_LINEAR_MIPMAP_LINEAR ||
minFilter == GL_LINEAR_MIPMAP_NEAREST ||
minFilter == GL_NEAREST_MIPMAP_LINEAR ||
minFilter == GL_NEAREST_MIPMAP_NEAREST)
glGenerateMipmap(GL_TEXTURE_2D);

return true;
}

void SetupRC()
{
// Make sure OpenGL entry points are set
glewInit();

// Initialze Shader Manager
shaderManager.InitializeStockShaders();

glEnable(GL_DEPTH_TEST);
glEnable(GL_CULL_FACE);

glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

// This makes a torus
gltMakeSphere(sunBatch,0.4f, 30, 30);

// This makes a sphere
gltMakeSphere(earthBatch, 0.1f, 26, 13);

gltMakeSphere(moonBatch, 0.05f, 26, 13);


// Make the solid ground
GLfloat texSize = 10.0f;
floorBatch.Begin(GL_TRIANGLE_FAN, 4, 1);
floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
floorBatch.Vertex3f(-20.0f, -0.41f, 20.0f);

floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
floorBatch.Vertex3f(20.0f, -0.41f, 20.0f);

floorBatch.MultiTexCoord2f(0, texSize, texSize);
floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);

floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);
floorBatch.End();

// Make 3 texture objects
glGenTextures(4, uiTextures);

// Load the Marble
glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
LoadTGATexture("marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);

// Load Mars
glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
LoadTGATexture("sun.tga", GL_LINEAR_MIPMAP_LINEAR,
GL_LINEAR, GL_CLAMP_TO_EDGE);

// Load Moon
glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
LoadTGATexture("earth.tga", GL_LINEAR_MIPMAP_LINEAR,
GL_LINEAR, GL_CLAMP_TO_EDGE);


glBindTexture(GL_TEXTURE_2D, uiTextures[3]);
LoadTGATexture("moon.tga", GL_LINEAR_MIPMAP_LINEAR,
GL_LINEAR, GL_CLAMP_TO_EDGE);

// Randomly place the spheres
for(int i = 0; i < NUM_SPHERES; i++) {
GLfloat x = ((GLfloat)((rand() % 400) - 200) * 0.1f);
GLfloat z = ((GLfloat)((rand() % 400) - 200) * 0.1f);
spheres[i].SetOrigin(x, 0.0f, z);
}
}

void ShutdownRC(void)
{
glDeleteTextures(4, uiTextures);
}







// Called to draw scene
void RenderScene(void)
{
static CStopWatchrotTimer;
float yRot = rotTimer.GetElapsedSeconds() * 10.0f;


glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

modelViewMatrix.PushMatrix();
M3DMatrix44f mCamera;
cameraFrame.GetCameraMatrix(mCamera);
modelViewMatrix.MultMatrix(mCamera);

// Draw the world upside down
modelViewMatrix.PushMatrix();
modelViewMatrix.Scale(1.0f, -1.0f, 1.0f); // Flips the Y Axis
modelViewMatrix.Translate(0.0f, 0.8f, 0.0f); // Scootch the world down a bit...
glFrontFace(GL_CW);
DrawSongAndDance(yRot);
glFrontFace(GL_CCW);
modelViewMatrix.PopMatrix();

// Draw the solid ground
glEnable(GL_BLEND);
glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
static GLfloat vFloorColor[] = { 1.0f, 1.0f, 1.0f, 0.75f};
shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE,
transformPipeline.GetModelViewProjectionMatrix(),
vFloorColor,
0);

floorBatch.Draw();


glDisable(GL_BLEND);


DrawSongAndDance(yRot);

modelViewMatrix.PopMatrix();


// Do the buffer Swap
glutSwapBuffers();

// Do it again
glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y)
{
float linear = 0.1f;
float angular = float(m3dDegToRad(5.0f));

if(key == GLUT_KEY_UP)
cameraFrame.MoveForward(linear);

if(key == GLUT_KEY_DOWN)
cameraFrame.MoveForward(-linear);

if(key == GLUT_KEY_LEFT)
cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);

if(key == GLUT_KEY_RIGHT)
cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
}




void ChangeSize(int nWidth, int nHeight)
{
glViewport(0, 0, nWidth, nHeight);
transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);

viewFrustum.SetPerspective(35.0f, float(nWidth)/float(nHeight), 1.0f, 100.0f);
projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
modelViewMatrix.LoadIdentity();
}


int main(int argc, char* argv[])
{
gltSetWorkingDirectory(argv[0]);

glutInit(&argc, argv);
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
glutInitWindowSize(800,600);

glutCreateWindow("OpenGL SphereWorld");

glutReshapeFunc(ChangeSize);
glutDisplayFunc(RenderScene);
glutSpecialFunc(SpecialKeys);


SetupRC();


glutMainLoop();


ShutdownRC();
return 0;
}*/
#include <stdio.h>
#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GraphicsObj.h"
#include "GLHelper.h"
#include "mat.h"
#include "Camera.h"
#include "ImageLib.h"
//#include "glut32.h"
#include<math.h>
#include<iostream>
using namespace std;
 
 
#define PI 3.14159
void Rotate() ;
//水星、金星、地球、火星、木星、土星、天王星、和海王星 
//adam,hesper,earth,mars,jupiter,saturn,uranus,neptune 
static float year = 0, day = 0,adamYear=0,hesperYear=0,marsYear=0,jupiterYear=0,saturnYear=0,uranusYear=0,neptuneYear=0,sunYear=0,month=0;
static float mar_satellite_1,mar_satellite_2 ;
float light_angle=0; 
float light_radius=8.0; 
double x=0,y=0;
 
 
 
 
double aix_x=0.0,aix_y=0.5,aix_z=0.5;
 
 
GLdouble angle = 100.0;
 
 
/*float theta=-90.0; //圆环旋转角
float angle=10; //左右场景每次旋转角
float sightangle=-90;
float s=10; //前后直走步长
float R=100; 
int inner=10,outer=80;
*/
 
 
/*float eyex=0,eyey=0,eyez=outer+4*inner+50;  
float atx=0,aty=0,atz=0;     
float atx1,atz1,eyex1,eyez1;*/
 
 
void lPosition()                                       
{ 
     float y,z; 
     y=light_radius*cos(light_angle); 
     z=light_radius*sin(light_angle);
     float light_position[] = { 3.0,y,z, 0.0 }; 
glLightfv(GL_LIGHT0, GL_POSITION, light_position);   
} 
 
 
 
 
void init(void) 
{ 
    glClearColor (0.0, 0.0, 0.0, 0.0);
    lPosition(); 
    glShadeModel (GL_SMOOTH);            
    glEnable(GL_LIGHTING);               
    glEnable(GL_LIGHT0); 
    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_COLOR_MATERIAL); 
}
 
 
 
 
void material_sun()                               //设置太阳材质，以下同
{ 
    GLfloat mat_specular[] = { 1.0, 0.0, 0.0, 1.0 };       
    GLfloat mat_shininess[] = { 50.0 };                  
    GLfloat lmodel_ambient[]={1.0,0.0,0.0,1.0};        //太阳颜色为红色      
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0};             
  
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);     
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);   
} 
 
 
void material_adam() 
{ 
    GLfloat mat_specular[] = { 1.0, 0.0, 0.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={1.3,1.0,0.2,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_hesper() 
{ 
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={1.0,1.0,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_earth() 
{ 
    GLfloat mat_specular[] = { 1.0, 0.0, 0.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.1,0.2,0.6,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_mars() 
{ 
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={1.0,0.1,0.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_jupiter() 
{ 
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.8,0.7,0.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_saturn() 
{ 
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.6,0.6,0.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_uranus() 
{ 
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.3,0.3,0.7,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
 
 
void material_neptune() 
{ 
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.0,0.1,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
} 
void material_moon()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={1.0,1.0,0.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
void material_mar_satellite_1()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.0,1.0,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
 
 
void material_mar_satellite_2()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.0,0.0,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
void material_neptune_satellite_1()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={1.0,0.0,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
void material_neptune_satellite_2()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.0,0.0,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
void material_uranus_satellite_1()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.3,0.2,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
void material_uranus_satellite_2()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.3,0.4,0.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
void material_jupiter_satellite()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.3,0.6,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
void material_saturn_satellite()
{
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
    GLfloat mat_shininess[] = { 50.0 }; 
    GLfloat lmodel_ambient[]={0.3,0.6,1.0,1.0}; 
    GLfloat white_light[]={1.0, 1.0,1.0, 1.0}; 
 
 
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, lmodel_ambient); 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient); 
}
 
 
 
 
void sun()                  //绘制太阳
{ 
    glPushMatrix(); 
material_sun();                               
glTranslatef (-15,0,0);
glRotatef ((GLfloat) day, 0.0, 1.0, 0.0);   //太阳自转
    glutSolidSphere(10, 200, 200);                //画球体        
    glPopMatrix(); 
} 
 
 
void adam()               //水星
{ 
    glPushMatrix();
material_adam();              
glTranslatef (-15,0,0);
    glRotatef ((GLfloat) adamYear, aix_x, aix_y, aix_z);    //水星公转周期
    glTranslatef (15,0,0);
glTranslatef (0.2,0,0);
    glRotatef ((GLfloat) day, 0.0, 1.0, 0.0);        //水星自转
glTranslatef (-0.5,0,0);
glTranslatef (0.5,0,0);
    glutSolidSphere(0.5, 20, 16); 
    glPopMatrix();  
} 
 
 
void hesper() 
{ 
    glPushMatrix(); 
    material_hesper();
glTranslatef (-15,0,0);
    glRotatef ((GLfloat)hesperYear,aix_x, aix_y, aix_z); 
glTranslatef (15,0,0);
    glTranslatef (0.8,0,0);
    glRotatef ((GLfloat) day, 0.0, 1.0, 0.0); 
glTranslatef (-1,0,0);
glTranslatef (1,0,0);
    glutSolidSphere(0.8, 20, 16);
    glPopMatrix(); 
} 
 
 
void earth() 
{ 
    glPushMatrix(); 
    material_earth(); 
glTranslatef (-15,0,0);
    glRotatef ((GLfloat) year, aix_x, aix_y, aix_z); 
glTranslatef (15,0,0);
    glTranslatef (2,0,0); 
    glRotatef ((GLfloat) day, 0.0, 1.0, 0.0);
glTranslatef (-2,0,0);
glTranslatef (2,0,0);
    glutSolidSphere(1.0, 20, 16);               
//  glColor3f(0.0, 1.0, 0.0);
 
 
material_moon();                          //绘制月球，以下以同样方法绘制各行星的卫星
     glRotatef(month/100, 0.0, 0.0, 1.0);
     glTranslatef(2, 0.0, 0.0);
     glutSolidSphere(0.3, 10, 10);
 
 
 
 
glPopMatrix(); 
} 
 
 
void mars() 
{ 
   glPushMatrix(); 
   material_mars();
   glTranslatef (-15,0,0);
   glRotatef ((GLfloat)marsYear,aix_x, aix_y, aix_z);
   glTranslatef (15,0,0);
   glTranslatef (7,0,0); 
   glRotatef ((GLfloat) day, 0.0, 1.0, 0.0); 
   glTranslatef (-7,0,0);
   glTranslatef (7,0,0);
   glutSolidSphere(0.6, 20, 16);  
   
    glPushMatrix();
    material_mar_satellite_1();
     glRotatef(month/20, 0.0, 1.0, 0.0);
     glTranslatef(1, 0.0, 0.0);
     glutSolidSphere(0.1, 10, 10);
glPopMatrix(); 
 
 
glPushMatrix();
  material_mar_satellite_2();
     glRotatef(month/46, 0.0, 1.0, 0.0);
     glTranslatef(1.5, 0.0, 0.0);
     glutSolidSphere(0.2, 10, 10);
glPopMatrix(); 
 
 
   glPopMatrix(); 
}
 
void jupiter() 
{ 
   glPushMatrix(); 
   material_jupiter();
   glTranslatef (-15,0,0);
   glRotatef ((GLfloat)jupiterYear, aix_x, aix_y, aix_z); 
   glTranslatef (15,0,0);
   glTranslatef (13,0,0); 
   glRotatef ((GLfloat) day, 0.0, 1.0, 0.0); 
   glTranslatef (-13,0,0); 
   glTranslatef (13,0,0); 
   glutSolidSphere(2.0, 20, 16);  
 
 
    glPushMatrix();
 
 
glRotatef(90,1.0,0,0.0);
glutSolidTorus(0.25, 5.0, 5, 64);
glRotatef(-90,1.0,0,0.0);
   
     
/* glutSolidTorus(0.1, 1.25, 10, 64);
glRotatef(-90,1.0,0,0.0);
glRotatef(90,1.0,0,0.0);*/
   
 
    material_jupiter_satellite();
 
 
glRotatef(90,1.0,0,0.0);
     glRotatef(month/58, 0.0, 1.0, 0.0);
     glTranslatef(3, 0.0, 0.0);
     glutSolidSphere(0.6, 10, 10);
 
 
 
 
/* glutSolidTorus(0.07, 1.65, 10, 64);
glRotatef(-90,1.0,0,0.0);
glPopMatrix();*/
 
 
   glPopMatrix(); 
   glPopMatrix();
}
 
 
void saturn() 
{ 
     glPushMatrix();
material_saturn();
glTranslatef (-15,0,0);
     glRotatef ((GLfloat) saturnYear, aix_x, aix_y, aix_z); 
glTranslatef (15,0,0);
     glTranslatef (20,0,0); 
     glRotatef ((GLfloat) day, 0.0, 1.0, 0.0);
glTranslatef (-20,0,0); 
glTranslatef (20,0,0); 
glRotatef (0.3, 1.0, 0.0, 0.0);
     glutSolidSphere(1.4, 20, 16);  
 
 
    glPushMatrix();
 
 
glRotatef(90,1.0,0,0.0);
glutSolidTorus(0.1, 3.0, 5, 64);
glRotatef(-90,1.0,0,0.0);
 
 
 
    material_saturn_satellite();
     glRotatef(month/76, 0.0, 1.0, 0.0);
     glTranslatef(2.7, 0.0, 0.0);
     glutSolidSphere(0.4, 10, 10);
 
 
     glPopMatrix(); 
glPopMatrix();
} 
 
 
void uranus() 
{ 
     glPushMatrix();
material_uranus();
glTranslatef (-15,0,0);
     glRotatef ((GLfloat) uranusYear,aix_x, aix_y, aix_z);
glTranslatef (15,0,0);
     glTranslatef (28,0,0); 
     glRotatef ((GLfloat) day, 0.0, 1.0, 0.0); 
glTranslatef (-28,0,0); 
glTranslatef (28,0,0); 
glRotatef (0.5, 1.0, 0.0, 0.0);
     glutSolidSphere(1.5, 20, 16); 
 
 
   glPushMatrix();
 
 
glRotatef(90,1.0,0,0.0);
glutSolidTorus(0.1, 3.0, 5, 64);
glRotatef(-90,1.0,0,0.0);
 
 
 
    glPushMatrix();
    material_uranus_satellite_1();
     glRotatef(month/108, 0.0, 1.0, 0.0);
     glTranslatef(2, 0.0, 0.0);
     glutSolidSphere(0.23, 10, 10);
glPopMatrix(); 
 
 
glPushMatrix();
  material_uranus_satellite_2();
     glRotatef(month/145, 0.0, 1.0, 0.0);
     glTranslatef(3.5, 0.0, 0.0);
     glutSolidSphere(0.35, 10, 10);
glPopMatrix();
 
 
     glPopMatrix(); 
glPopMatrix(); 
} 
 
 
void neptune() 
{ 
     glPushMatrix();
material_neptune();
glTranslatef (-15,0,0);
     glRotatef ((GLfloat) neptuneYear,aix_x, aix_y, aix_z); 
glTranslatef (15,0,0);
     glTranslatef (32,0,0); 
     glRotatef ((GLfloat) day, 0.0, 1.0, 0.0); 
glTranslatef (-32,0,0); 
glTranslatef (32,0,0); 
glRotatef (0.5, 1.0, 0.0, 0.0);
     glutSolidSphere(1.3, 20, 16);    
 
 
   glPushMatrix();
 
 
glRotatef(90,1.0,0,0.0);
glutSolidTorus(0.1, 3.0, 5, 64);
glRotatef(-90,1.0,0,0.0);
 
 
 
    glPushMatrix();
    material_neptune_satellite_1();
     glRotatef(month/347, 0.0, 1.0, 0.0);
     glTranslatef(2.5, 0.0, 0.0);
     glutSolidSphere(0.35, 10, 10);
glPopMatrix(); 
 
 
glPushMatrix();
  material_neptune_satellite_2();
     glRotatef(month/389, 0.0, 1.0, 0.0);
     glTranslatef(3.5, 0.0, 0.0);
     glutSolidSphere(0.3, 10, 10);
glPopMatrix(); 
 
 
     glPopMatrix(); 
 glPopMatrix(); 
 
 
} 
 
 
void display(void) 
{ 
    lPosition(); 
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 
 
 
 
 
    sun(); 
    adam(); 
    hesper(); 
    earth(); 
    mars(); 
    jupiter(); 
    saturn(); 
    uranus(); 
    neptune(); 
 
 
Rotate();
 
 
    glutSwapBuffers(); 
} 
 
 
void myidle()
{
day+=10.0;
if (day>=360)
day=day-360;
 
 
glutPostRedisplay();  
}
 
 
void Rotate()          //设置各行星的公转周期
{ 
   //adamYear=0,hesperYear=0,marsYear=0,jupiterYear=0,saturnYear=0,uranusYear=0,neptuneYear=0; 
 //  adamYear=(adamYear+1.2)%360; 
   adamYear+=0.12;
   if(adamYear>=360)
  adamYear-=360;
//   hesperYear=(hesperYear+2)%360; 
   hesperYear+=0.10;
   if(hesperYear>=360)
  hesperYear-=360;
 //  year=(year+0.8)%360;
   year+=0.08;
   if(year>=360)
  year-=360;
//   marsYear=(marsYear+0.6)%360;
   marsYear+=0.06;
   if(marsYear>=360)
  marsYear-=360;
 
 
//   jupiterYear=(jupiterYear+0.5)%360; 
    jupiterYear+=0.05;
   if(jupiterYear>=360)
  jupiterYear-=360;
//   saturnYear=(saturnYear+0.4)%360; 
    saturnYear+=0.04;
   if(saturnYear>=360)
  saturnYear-=360;
 //  uranusYear=(uranusYear+0.3)%360; 
   uranusYear+=0.03;
   if(uranusYear>=360)
  uranusYear-=360;
//   neptuneYear=(neptuneYear+0.1)%360; 
    neptuneYear+=0.01;
   if(neptuneYear>=360)
  neptuneYear-=360;
   glutPostRedisplay();                
   month+=0.03;
   if(month>=360)
  month-=360;
} 
 
 
void mykeyboard(unsigned char key, int x, int y)  
{
switch(key)
{
case 'U':
case 'u':
aix_y-=0.01;
aix_z+=0.01;
break;
case 'D':
case 'd':
aix_y+=0.01;
aix_z-=0.01;
break;
 
 
case 'W':
case 'w':
angle-=10.0;
break;
case 'S':
case 's':
angle+=10.0;
break;
}
    glutPostRedisplay();  
     
}
 
 
 
 
void reshape (int w, int h)              
{ 
    glViewport (0, 0, (GLsizei) w, (GLsizei) h);                
    glMatrixMode (GL_PROJECTION);                               
    glLoadIdentity ();    
 
    gluPerspective(angle, (GLfloat) w/(GLfloat) h, 1.0, 200.0); 
 
    glMatrixMode(GL_MODELVIEW); 
glLoadIdentity();
glTranslatef(0.0,0.0,-50.0);                               
} 
 
 
 
 
 
 
 
 
int main(int argc, char** argv) 
{ 
cout<<argc<<endl;
cout<<&argc<<endl;
 
 
for(int i=0;i<argc;i++)
{
cout<<*argv[i]<<endl;
}
 
 
 
 
   glutInit(&argc, argv);                                      
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);   
   glutInitWindowSize (800, 600);                              
   glutInitWindowPosition (100, 100);                          
   glutCreateWindow ("solar system");                          
   init (); 
   glutDisplayFunc(display);          
   glutReshapeFunc(reshape);       
   glutIdleFunc(myidle);   
   
   glutKeyboardFunc(mykeyboard);   
 
 
   glutMainLoop();                 
   return 0; 
} 