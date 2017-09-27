#pragma once
#include <GLES2/gl2.h>
namespace PnaclPlayer
{
	struct Shader
	{
		Shader() : program(0), texcoord_scale_location(0) {}
		~Shader() {}

		GLuint program;
		GLint texcoord_scale_location;
	};
}