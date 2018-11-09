#include <stdlib.h>
#include <stddef.h>
#include "GraphicsObj.h"

CGraphicsObj::CGraphicsObj(void)
{
	vertex_array_obj=0;
	vertex_buffer_obj=0;
	num_vertices=0;
	primitive_type=GL_TRIANGLES;
}

void CGraphicsObj::Draw(void)
{
	glBindVertexArray(vertex_array_obj);
	glDrawArrays(primitive_type, 0, num_vertices);
	glBindVertexArray(0);
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

	glGenVertexArrays(1, &vertex_array_obj);
	glBindVertexArray(vertex_array_obj);

	glGenBuffers(1, &vertex_buffer_obj);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_obj);
	glBufferData(GL_ARRAY_BUFFER,
		num_vertices*sizeof(CGraphicsObjVertex), p,
		GL_STATIC_DRAW);

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
