#ifndef __DD_UTIL_H__
#define __DD_UTIL_H__
#import <OpenGLES/ES2/gl.h>
#include <stdlib.h>
#include <string>
#include <functional>

#define GLES2_NOT_INIT -1
#define GLES2_NO_TEXTURE -1
#define GLES2_ON_DRAW 1

#ifndef LOGE
#define LOGE printf
#endif
#ifndef LOGW
#define LOGW printf
#endif
#ifndef LOGD
#define LOGD printf
#endif

namespace DD {
	typedef std::function<void()> GLCallbackV;
	typedef std::function<void(int,int)> GLCallbackII;

	class GLException {
	public:
		GLException(std::string description) :
			m_description(description)
		{

		}
		std::string m_description;
	};

	void glPrintError(int programObject);
	GLuint loadProgram(const char* vertexShaderString = NULL, const char* fragmentShaderString = NULL);
	void glCheckError(std::string op = "");
	bool getTexture(GLuint** pFrameBuffers, GLuint** pFrameBufferTextures, int width, int height);
}

#endif

