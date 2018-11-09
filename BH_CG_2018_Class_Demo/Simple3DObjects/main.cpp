#include <stdio.h>
#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GraphicsObj.h"
#include "GLHelper.h"
#include "mat.h"

CGraphicsObj g_obj[4];
int g_current_obj_index=0;
GLuint g_simple_GLSL_prog;

mat4 g_M_prev_rotation, g_M_current_rotation;
vec3 g_track_ball_vec[2];
bool g_interactive_rotation_mode=false;

int g_window_width, g_window_height;

void MapMousePosToNormCoord(int mouse_x, int mouse_y, vec2& P)
{
	P.x=2.0f*(float)mouse_x/g_window_width-1.0f;
	P.y=2.0f*(float)(g_window_height-mouse_y)/g_window_height-1.0f;
}

void MapMousePosToTrackBall(int mouse_x, int mouse_y, vec3& P)
{
	vec2 P2D;
	MapMousePosToNormCoord(mouse_x, mouse_y, P2D);
	float r2=dot(P2D, P2D);
	if (r2>=1.0f)
	{
		float r=sqrt(r2);
		P.z=0.0f;
		P.x=P2D.x/r;
		P.y=P2D.y/r;
	}
	else
	{
		P.x=P2D.x;
		P.y=P2D.y;
		P.z=sqrt(1.0f-r2);
	}
}

void GetTrackBallRotationMatrix(
	const vec3& P1, const vec3& P2, mat4& M)
{
	vec3 N=cross(P1, P2);
	float d2=dot(N, N);
	if (d2<3e-6f)
	{
		M=mat4();
	}
	else
	{
		float a=asin(sqrt(d2))/DegreesToRadians;
		M=Rotate(a, N.x, N.y, N.z);
	}
}

void init_shaders(void)
{
	g_simple_GLSL_prog=InitShader(
		//"..\\shaders\\single_light-vs.txt",
		//"..\\shaders\\single_light-fs.txt"
		"..\\shaders\\single_light-phone_shading-vs.txt",
		"..\\shaders\\single_light-phone_shading-fs.txt"
		);
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
	g_obj[0].CreateGasket3D(tetra_v, 3);
	g_obj[1].CreateCube(1.0f);
	g_obj[2].CreateRect(1.6f, 1.0f, 8, 10);
	g_obj[3].CreateSphere(0.6f, 32, 16);
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
}

void reshape(int w, int h)
{
	g_window_width=w;
	g_window_height=h;

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
	if (g_interactive_rotation_mode)
		M=g_M_current_rotation*g_M_prev_rotation;
	else
		M=g_M_prev_rotation;

	M=Translate(0.0f, 0.0f, -2.0f)*M;
	int loc=glGetUniformLocation(g_simple_GLSL_prog, "model_matrix");
	glUniformMatrix4fv(loc, 1, GL_TRUE, M);

	mat3 Mn=Normal(M);
	loc=glGetUniformLocation(g_simple_GLSL_prog, "normal_matrix");
	glUniformMatrix3fv(loc, 1, GL_TRUE, Mn);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	g_obj[g_current_obj_index].Draw();
	glFlush();
}

void mouse(int button, int state, int x, int y)
{
	if (button==GLUT_LEFT_BUTTON)
	{
		if (state==GLUT_DOWN)
		{
			g_interactive_rotation_mode=true;
			MapMousePosToTrackBall(x, y, g_track_ball_vec[0]);
			g_track_ball_vec[1]=g_track_ball_vec[0];
			g_M_current_rotation=mat4();
			glutPostRedisplay();
		}
		else
		{
			g_interactive_rotation_mode=false;
			g_M_prev_rotation=g_M_current_rotation*g_M_prev_rotation;
		}
	}
}

void mouse_motion(int x, int y)
{
	if (g_interactive_rotation_mode)
	{
		MapMousePosToTrackBall(x, y, g_track_ball_vec[1]);
		GetTrackBallRotationMatrix(
			g_track_ball_vec[0],
			g_track_ball_vec[1],
			g_M_current_rotation);
		glutPostRedisplay();
	}
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	case '1':
		g_current_obj_index=0;
		glutPostRedisplay();
		break;
	case '2':
		g_current_obj_index=1;
		glutPostRedisplay();
		break;
	case '3':
		g_current_obj_index=2;
		glutPostRedisplay();
		break;
	case '4':
		g_current_obj_index=3;
		glutPostRedisplay();
		break;
	}
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_SINGLE);
	glutInitWindowSize(640, 480);

	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow("3D Objects");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(mouse_motion);
	glutKeyboardFunc(keyboard);

	glewExperimental=GL_TRUE;
	glewInit();

	init();

	glutMainLoop();

	return 1;
}
