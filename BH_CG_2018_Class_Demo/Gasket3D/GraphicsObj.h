#ifndef _GRAPHICS_OBJ_H_
#define _GRAPHICS_OBJ_H_

#include "GL/glew.h"
#include "vec.h"

class CGraphicsObjVertex
{
public:
	point3 position;
	vec3 normal;
};

class CGraphicsObj
{
public:
	GLuint vertex_array_obj;
	GLuint vertex_buffer_obj;

	GLenum primitive_type;
	int num_vertices;

	CGraphicsObj(void);
	
	void Draw(void);
	
protected:
	void Divide3DTetra(CGraphicsObjVertex *v_out, int &ipos,
		const vec3& v0_in, const vec3& v1_in,
		const vec3& v2_in, const vec3& v3_in,
		int depth);

public:
	void CreateGasket3D(
		const vec3 tetra_vertices[4], int depth);
};

#endif
