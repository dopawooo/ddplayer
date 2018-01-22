#include "DDGLUtil.h"
//#include "dd_log.h"

#define __MODULE__ "DDGLUtil"

namespace DD {
	void glCheckError(std::string op){
		int error = glGetError();
		if (error != 0) {
			throw new GLException(op);
		}
	}

	static GLuint LoadShader(GLenum type, const char *shaderSrc) {
		GLuint shader;
		GLint compiled;

		// Create the shader object
		shader = glCreateShader(type);

		if (shader == 0)
			return 0;
		// Load the shader source
		glShaderSource(shader, 1, &shaderSrc, NULL);
		// Compile the shader
		glCompileShader(shader);
		// Check the compile status
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen > 1) {
				char* infoLog = (char*)malloc(sizeof(char) * infoLen);
				glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
				//esLogMessage("Error compiling shader:\n%s\n", infoLog);
				LOGE("Error LoadShader:\n%s\n", infoLog);
				free(infoLog);
			}
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	GLuint loadProgram(const char* vertexShaderString, const char* fragmentShaderString) {
		GLuint vertexShader;
		GLuint fragmentShader;
		GLuint programObject;
		GLint linked;

		// Create the program object
		programObject = glCreateProgram();
		if (programObject == 0)
			return 0;

		vertexShader = LoadShader(GL_VERTEX_SHADER, vertexShaderString);
		fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragmentShaderString);
		glAttachShader(programObject, vertexShader);
		glAttachShader(programObject, fragmentShader);
		//// Bind vPosition to attribute 0   
		//glBindAttribLocation(programObject, 0, "position");
		// Link the program
		glLinkProgram(programObject);
		// Check the link status
		glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
		if (!linked) {
			GLint infoLen = 0;
			glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen > 1) {
				char* infoLog = (char*)malloc(sizeof(char) * infoLen);
				glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
				LOGE("Error linking program:\n%s\n", infoLog);
				free(infoLog);
			}
			glDeleteProgram(programObject);
			return 0;
		}
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glCheckError("loadProgram");
		return programObject;
	}

	void glPrintError(int programObject) {
		int infoLen = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);
			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			LOGE("Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
	}

	bool getTexture(GLuint** pFrameBuffers, GLuint** pFrameBufferTextures, int width, int height) {
		glEnable(GL_TEXTURE_2D);// ∆Ù”√Œ∆¿Ì
		GLenum err = 0;
		GLuint* frameBuffers = new GLuint[1];
		GLuint* frameBufferTextures = new GLuint[1];
		glGenFramebuffers(1, frameBuffers);
		glGenTextures(1, frameBufferTextures);
		glBindTexture(GL_TEXTURE_2D, frameBufferTextures[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameterf(GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D,
			GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, frameBufferTextures[0], 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if ((err = glGetError()) == 0) {
			*pFrameBuffers = frameBuffers;
			*pFrameBufferTextures = frameBufferTextures;
			return true;
		}
		else {
			if (frameBuffers[0] != 0) {
				glDeleteFramebuffers(1, frameBuffers);
			}
			if (frameBufferTextures[0] != 0) {
				glDeleteTextures(1, frameBufferTextures);
			}
            *pFrameBuffers = NULL;
            *pFrameBufferTextures = NULL;
			return false;
		}
	}
}
