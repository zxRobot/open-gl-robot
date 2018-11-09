#include <stdio.h>
#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GraphicsObj.h"
#include "GLHelper.h"
#include "mat.h"

CGraphicsObj g_obj;
GLuint g_simple_GLSL_prog;

float g_rot_angle=0.0f;
float g_current_time;

void init_shaders(void)
{
	g_simple_GLSL_prog=InitShader(
		"..\\shaders\\single_light-vs.txt",
		"..\\shaders\\single_light-fs.txt");
	glUseProgram(g_simple_GLSL_prog);

	mat4 M;
	int loc=glGetUniformLocation(g_simple_GLSL_prog, "view_matrix");
	glUniformMatrix4fv(loc, 1, GL_TRUE, M);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "global_ambient_light");
	glUniform4f(loc, 0.2f, 0.2f, 0.2f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "light_position");
	glUniform4f(loc, 1.0f, 1.5f, 0.6f, 0.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "light_color");
	glUniform4f(loc, 1.0f, 1.0f, 1.0f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "diffuse_reflectivity");
	glUniform4f(loc, 1.0f, 0.5f, 0.0f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "specular_reflectivity");
	glUniform4f(loc, 0.8f, 0.8f, 0.8f, 1.0f);

	loc=glGetUniformLocation(g_simple_GLSL_prog, "shininess");
	glUniform1f(loc, 128.0f);
}

void init_models(void)
{
	vec3 tetra_v[]={
		vec3(-1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(0.0f, 1.0f, -1.0f),
		vec3(0.0f, 0.0f, 1.0f)
	};
	g_obj.CreateGasket3D(tetra_v, 3);
}

void init(void)
{
	g_current_time=0.001f*glutGet(GLUT_ELAPSED_TIME);

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	printf("Vender: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));

	init_shaders();

	init_models();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);

	mat4 M;
	M=Perspective(60.0f, (float)w/(float)h, 0.01f, 5.0f);
	glUseProgram(g_simple_GLSL_prog);
	int loc=glGetUniformLocation(g_simple_GLSL_prog, "projection_matrix");
	glUniformMatrix4fv(loc, 1, GL_TRUE, M);
}

void display(void)
{
	mat4 M;
	M=Translate(0.0f, 0.0f, -2.0f)*RotateY(g_rot_angle);
	int loc=glGetUniformLocation(g_simple_GLSL_prog, "model_matrix");
	glUniformMatrix4fv(loc, 1, GL_TRUE, M);

	mat3 Mn=Normal(M);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "normal_matrix");
	glUniformMatrix3fv(loc, 1, GL_TRUE, Mn);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	g_obj.Draw();
	glFlush();
}

void idle(void)
{
	float t=0.001f*glutGet(GLUT_ELAPSED_TIME);
	float dt=t-g_current_time;
	g_current_time=t;

	g_rot_angle+=90.0f*dt;
	g_rot_angle=fmod(g_rot_angle, 360.0f);

	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_SINGLE);
	glutInitWindowSize(640, 480);

	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow("Gasket 3D Program");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);

	glewExperimental=GL_TRUE;
	glewInit();

	init();

	glutMainLoop();

	return 1;
}
