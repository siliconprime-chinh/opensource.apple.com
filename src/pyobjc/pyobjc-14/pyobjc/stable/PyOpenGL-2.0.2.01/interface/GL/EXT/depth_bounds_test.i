/*
# AUTOGENERATED DO NOT EDIT

# If you edit this file, delete the AUTOGENERATED line to prevent re-generation
# BUILD api_versions [0x001]
*/

%module depth_bounds_test

#define __version__ "$Revision: 1.1.2.1 $"
#define __date__ "$Date: 2004/11/15 07:38:07 $"
#define __api_version__ API_VERSION
#define __author__ "PyOpenGL Developers <pyopengl-devel@lists.sourceforge.net>"
#define __doc__ ""

%{
/**
 *
 * GL.EXT.depth_bounds_test Module for PyOpenGL
 * 
 * Authors: PyOpenGL Developers <pyopengl-devel@lists.sourceforge.net>
 * 
***/
%}

%include util.inc
%include gl_exception_handler.inc

%{
#ifdef CGL_PLATFORM
# include <OpenGL/glext.h>
#else
# include <GL/glext.h>
#endif

#if !EXT_DEFINES_PROTO || !defined(GL_EXT_depth_bounds_test)
DECLARE_VOID_EXT(glDepthBoundsEXT, (GLclampd zmin, GLclampd zmax), (zmin, zmax))
#endif
%}

/* FUNCTION DECLARATIONS */
void glDepthBoundsEXT(GLclampd zmin, GLclampd zmax);
DOC(glDepthBoundsEXT, "glDepthBoundsEXT(zmin, zmax)")

/* CONSTANT DECLARATIONS */
#define       GL_DEPTH_BOUNDS_TEST_EXT 0x8890
#define            GL_DEPTH_BOUNDS_EXT 0x8891


%{
static char *proc_names[] =
{
#if !EXT_DEFINES_PROTO || !defined(GL_EXT_depth_bounds_test)
"glDepthBoundsEXT",
#endif
	NULL
};

#define glInitDepthBoundsTestEXT() InitExtension("GL_EXT_depth_bounds_test", proc_names)
%}

int glInitDepthBoundsTestEXT();
DOC(glInitDepthBoundsTestEXT, "glInitDepthBoundsTestEXT() -> bool")

%{
PyObject *__info()
{
	if (glInitDepthBoundsTestEXT())
	{
		PyObject *info = PyList_New(0);
		return info;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}
%}

PyObject *__info();

