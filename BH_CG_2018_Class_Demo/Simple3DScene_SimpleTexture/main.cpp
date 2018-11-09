#include <stdio.h>
#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GraphicsObj.h"
#include "GLHelper.h"
#include "mat.h"
#include "Camera.h"

class CModel3D
{
public:
	CGraphicsObj mesh;
	mat4 model_matrix;
	vec4 diffuse_color;
};

CModel3D g_models[4];
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

bool g_use_mipmap=true;

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
		"..\\shaders\\multi_lights-single_tex-vs.txt",
		"..\\shaders\\multi_lights-single_tex-fs.txt"
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
	vec3 tetra_v[]={
		vec3(-1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(0.0f, 1.0f, -1.0f),
		vec3(0.0f, 0.0f, 1.0f)
	};
	g_models[0].mesh.CreateRect(6.0f, 5.0f, 12, 10, 6.0f, 5.0f);
	g_models[1].mesh.CreateGasket3D(tetra_v, 3);
	g_models[2].mesh.CreateCube(1.0f, 5.0f, 4.0f, 3.0f);
	g_models[3].mesh.CreateSphere(0.6f, 32, 16, 6.0f, 4.0f);

	float t=1.4f;
	g_models[1].model_matrix=Translate(-t, t, 1.0f);
	g_models[2].model_matrix=Translate(-t, -t, 0.5f);
	g_models[3].model_matrix=Translate(t, -t, 0.6f);

	g_models[0].diffuse_color=vec4(1.0f, 1.0f, 1.0f, 1.0f);
	g_models[1].diffuse_color=vec4(1.0f, 0.5f, 0.5f, 1.0f);
	g_models[2].diffuse_color=vec4(0.2f, 1.0f, 0.2f, 1.0f);
	g_models[3].diffuse_color=vec4(0.4f, 0.4f, 1.0f, 1.0f);
}

void init(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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

	glBindTexture(GL_TEXTURE_2D, g_main_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		g_use_mipmap?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);

	for (int i=0; i<4; ++i)
	{
		M=g_models[i].model_matrix;

		loc=glGetUniformLocation(g_simple_GLSL_prog, "model_matrix");
		glUniformMatrix4fv(loc, 1, GL_TRUE, M);

		Mn=Normal(M);
		loc=glGetUniformLocation(g_simple_GLSL_prog, "normal_matrix");
		glUniformMatrix3fv(loc, 1, GL_TRUE, Mn);

		loc=glGetUniformLocation(g_simple_GLSL_prog, "diffuse_reflectivity");
		glUniform4fv(loc, 1, g_models[i].diffuse_color);

		loc=glGetUniformLocation(g_simple_GLSL_prog, "enable_tex_map");
		glUniform1i(loc, i!=1);

		g_models[i].mesh.Draw();
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
	case 'm':
	case 'M':
		g_use_mipmap=!g_use_mipmap;
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
