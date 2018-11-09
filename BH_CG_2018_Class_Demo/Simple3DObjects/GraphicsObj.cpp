#include <stdlib.h>
#include <stddef.h>
#include "GraphicsObj.h"

CGraphicsObj::CGraphicsObj(void)
{
	vertex_array_obj=0;
	vertex_buffer_obj=0;
	vertex_index_obj=0;
	num_vertices=0;
	num_indices=0;
	primitive_type=GL_TRIANGLES;
}

void CGraphicsObj::Draw(void)
{
	glBindVertexArray(vertex_array_obj);
	if (vertex_index_obj==0)
		glDrawArrays(primitive_type, 0, num_vertices);
	else
		glDrawElements(primitive_type, num_indices,
			GL_UNSIGNED_INT, (void *)0);
	glBindVertexArray(0);
}

void CGraphicsObj::CreateGLBuffers(
	CGraphicsObjVertex *p, GLuint *indices)
{
	glGenVertexArrays(1, &vertex_array_obj);
	glBindVertexArray(vertex_array_obj);

	glGenBuffers(1, &vertex_buffer_obj);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_obj);
	glBufferData(GL_ARRAY_BUFFER,
		num_vertices*sizeof(CGraphicsObjVertex), p,
		GL_STATIC_DRAW);

	if (indices!=NULL)
	{
		glGenBuffers(1, &vertex_index_obj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_index_obj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			num_indices*sizeof(GLuint), indices,
			GL_STATIC_DRAW);
		delete [] indices;
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,
		3, GL_FLOAT, GL_FALSE, sizeof(CGraphicsObjVertex), 
		(GLvoid *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,
		3, GL_FLOAT, GL_FALSE, sizeof(CGraphicsObjVertex), 
		(GLvoid *)offsetof(CGraphicsObjVertex, normal));

	glBindVertexArray(0);

	delete [] p;
}

void CGraphicsObj::Divide3DTetra(
	CGraphicsObjVertex *v_out, int &ipos,
	const vec3& v0_in, const vec3& v1_in,
	const vec3& v2_in, const vec3& v3_in,
	int depth)
{
	int i, j;
	vec3 mid[6], N;
	if (depth>0)
	{
		mid[0]=0.5f*(v0_in+v3_in);
		mid[1]=0.5f*(v1_in+v3_in);
		mid[2]=0.5f*(v2_in+v3_in);
		mid[3]=0.5f*(v1_in+v2_in);
		mid[4]=0.5f*(v2_in+v0_in);
		mid[5]=0.5f*(v0_in+v1_in);

		Divide3DTetra(v_out, ipos,
			v0_in, mid[5], mid[4], mid[0], depth-1);
		Divide3DTetra(v_out, ipos,
			v1_in, mid[3], mid[5], mid[1], depth-1);
		Divide3DTetra(v_out, ipos,
			v2_in, mid[4], mid[3], mid[2], depth-1);
		Divide3DTetra(v_out, ipos,
			mid[0], mid[1], mid[2], v3_in, depth-1);
	}
	else
	{
		const vec3 *pv[4]={&v0_in, &v1_in, &v2_in, &v3_in};
		const int iv_faces[4][3]={
			{3,0,1},{3,1,2},{3,2,0},{0,2,1}
		};
		for (i=0; i<4; ++i)
		{
			N=TriangleNormal(
				*pv[iv_faces[i][0]],
				*pv[iv_faces[i][1]],
				*pv[iv_faces[i][2]]);
			for (j=0; j<3; ++j)
			{
				v_out[ipos].position=*pv[iv_faces[i][j]];
				v_out[ipos].normal=N;

				ipos++;
			}
		}
	}
}

void CGraphicsObj::CreateGasket3D(
	const vec3 tetra_vertices[4], int depth)
{
	primitive_type=GL_TRIANGLES;

	num_vertices=12;
	int i;
	for (i=0; i<depth; ++i)
		num_vertices*=4;

	CGraphicsObjVertex *p=new CGraphicsObjVertex [num_vertices];

	i=0;
	Divide3DTetra(p, i, 
		tetra_vertices[0], tetra_vertices[1], 
		tetra_vertices[2], tetra_vertices[3],
		depth);

	CreateGLBuffers(p, NULL);
}

void CGraphicsObj::CreateCube(float size)
{
	primitive_type=GL_TRIANGLES;

	num_vertices=24;
	num_indices=36;

	CGraphicsObjVertex *p=new CGraphicsObjVertex [num_vertices];
	GLuint *indices=new GLuint [num_indices];

	static const point3 vpos[8]={
		point3(-1.0f, -1.0f, -1.0f),
		point3( 1.0f, -1.0f, -1.0f),
		point3( 1.0f,  1.0f, -1.0f),
		point3(-1.0f,  1.0f, -1.0f),
		point3(-1.0f, -1.0f,  1.0f),
		point3( 1.0f, -1.0f,  1.0f),
		point3( 1.0f,  1.0f,  1.0f),
		point3(-1.0f,  1.0f,  1.0f)
	};

	static const int face_iv[6][4]={
		{0,4,7,3},
		{1,2,6,5},
		{0,1,5,4},
		{3,7,6,2},
		{0,3,2,1},
		{4,5,6,7}
	};

	static const vec3 face_N[6]={
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, 0.0f, 1.0f)
	};

	float s_half=0.5f*size;
	int i, j, k;
	for (i=0, k=0; i<6; ++i)
	{
		for (j=0; j<4; ++j, ++k)
		{
			p[k].position=vpos[face_iv[i][j]]*s_half;
			p[k].normal=face_N[i];
		}
	}

	for (i=0, k=0; i<6; ++i)
	{
		j=i*4;
		indices[k++]=j;
		indices[k++]=j+1;
		indices[k++]=j+2;
		indices[k++]=j;
		indices[k++]=j+2;
		indices[k++]=j+3;
	}

	CreateGLBuffers(p, indices);
}

void CGraphicsObj::CreateRect(
	float sx, float sy, int nx, int ny)
{
	primitive_type=GL_TRIANGLES;

	num_vertices=(nx+1)*(ny+1);
	num_indices=6*nx*ny;
	float dx=sx/nx;
	float dy=sy/ny;

	CGraphicsObjVertex *p=new CGraphicsObjVertex [num_vertices];
	GLuint *indices=new GLuint [num_indices];

	int i, j, k;
	for (k=0, j=0; j<=ny; ++j)
	{
		float y=-0.5f*sy+j*dy;
		for (i=0; i<=nx; ++i, ++k)
		{
			p[k].position=point3(-0.5f*sx+i*dx, y, 0.0f);
			p[k].normal=vec3(0.0f, 0.0f, 1.0f);
		}
	}

	k=0;
	int t[4];
	for (j=0; j<ny; ++j)
	{
		for (i=0; i<nx; ++i)
		{
			t[0]=j*(nx+1)+i;
			t[1]=t[0]+1;
			t[3]=t[0]+nx+1;
			t[2]=t[3]+1;

			if (i%2==j%2)
			{
				indices[k++]=t[0];
				indices[k++]=t[1];
				indices[k++]=t[2];
				indices[k++]=t[0];
				indices[k++]=t[2];
				indices[k++]=t[3];
			}
			else
			{
				indices[k++]=t[0];
				indices[k++]=t[1];
				indices[k++]=t[3];
				indices[k++]=t[1];
				indices[k++]=t[2];
				indices[k++]=t[3];
			}
		}
	}

	CreateGLBuffers(p, indices);
}

void CGraphicsObj::CreateSphere(
	float radius, int n_theta, int n_phi)
{
	primitive_type=GL_TRIANGLES;

	num_vertices=(n_theta+1)*(n_phi+1);
	num_indices=6*n_theta*n_phi;
	float dtheta=(M_PI+M_PI)/n_theta;
	float dphi=M_PI/n_phi;

	CGraphicsObjVertex *p=new CGraphicsObjVertex [num_vertices];
	GLuint *indices=new GLuint [num_indices];

	int i, j, k;
	for (k=0, j=0; j<=n_phi; ++j)
	{
		float phi=-0.5f*M_PI+j*dphi;
		float cphi=cos(phi);
		float sphi=sin(phi);
		for (i=0; i<=n_theta; ++i, ++k)
		{
			float theta=i*dtheta;
			float ctheta=cos(theta);
			float stheta=sin(theta);
			p[k].normal=vec3(
				cphi*ctheta, cphi*stheta, sphi);
			p[k].position=radius*p[k].normal;
		}
	}

	k=0;
	int t[4];
	for (j=0; j<n_phi; ++j)
	{
		for (i=0; i<n_theta; ++i)
		{
			t[0]=j*(n_theta+1)+i;
			t[1]=t[0]+1;
			t[3]=t[0]+n_theta+1;
			t[2]=t[3]+1;

			if (i%2==j%2)
			{
				indices[k++]=t[0];
				indices[k++]=t[1];
				indices[k++]=t[2];
				indices[k++]=t[0];
				indices[k++]=t[2];
				indices[k++]=t[3];
			}
			else
			{
				indices[k++]=t[0];
				indices[k++]=t[1];
				indices[k++]=t[3];
				indices[k++]=t[1];
				indices[k++]=t[2];
				indices[k++]=t[3];
			}
		}
	}

	CreateGLBuffers(p, indices);
}

