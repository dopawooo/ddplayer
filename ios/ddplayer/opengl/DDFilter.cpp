#include "DDFilter.h"
#include <assert.h>

const char DD::DDFilter::defaultVetexShaderStr[] =
"attribute vec4 position;    \n"
"attribute vec4 inputTextureCoordinate;\n"
"varying vec2 textureCoordinate;\n"
"void main()                  \n"
"{                            \n"
"   gl_Position = position;  \n"
"   textureCoordinate = inputTextureCoordinate.xy;\n"
"}                            \n";

const char DD::DDFilter::defaultFragmentShaderStr[] =
"precision mediump float;\n"
"varying highp vec2 textureCoordinate;\n"
"uniform sampler2D inputImageTexture;\n"
"void main()                                  \n"
"{                                            \n"
"	gl_FragColor = texture2D(inputImageTexture, textureCoordinate);\n"
"}                                            \n";

GLfloat DD::TextureUtil_CUBE[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f,
};
GLfloat DD::TextureUtil_NO_ROTATION[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
};
GLfloat DD::TextureUtil_ROTATED_90[] = {
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,
};
GLfloat DD::TextureUtil_ROTATED_180[] = {
	1.0f, 0.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};
GLfloat DD::TextureUtil_ROTATED_270[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
};

GLfloat* DD::TextureUtil_getRotation(TextureUtil_ROTATION rotation, bool flipHorizon, bool filpVertical) {
	GLfloat* ret = (GLfloat*)malloc(sizeof(TextureUtil_NO_ROTATION));
	switch (rotation)
	{
	case ROTATION_90:
		memcpy(ret, TextureUtil_ROTATED_90, sizeof(TextureUtil_NO_ROTATION));
		break;
	case ROTATION_180:
		memcpy(ret, TextureUtil_ROTATED_180, sizeof(TextureUtil_NO_ROTATION));
		break;
	case ROTATION_270:
		memcpy(ret, TextureUtil_ROTATED_270, sizeof(TextureUtil_NO_ROTATION));
		break;
	default:
		memcpy(ret, TextureUtil_NO_ROTATION, sizeof(TextureUtil_NO_ROTATION));
	}
	if (flipHorizon){
		ret[0] = 1.f - ret[0];
		ret[2] = 1.f - ret[2];
		ret[4] = 1.f - ret[4];
		ret[6] = 1.f - ret[6];
	}
	if (filpVertical) {
		ret[1] = 1.f - ret[1];
		ret[3] = 1.f - ret[3];
		ret[5] = 1.f - ret[5];
		ret[7] = 1.f - ret[7];
	}
	return ret;
}

DD::DDFilter::DDFilter(const char* vertexShader /*= NULL*/, const char* fragmentShader /*= NULL*/) :
mGLProgId(0) ,
mIsInitialized(false), 
mOutputWidth(0),
mOutputHeight(0),
mIntputWidth(0),
mIntputHeight(0),
mGLAttribPosition(-1),
mGLUniformTexture(-1),
mGLAttribTextureCoordinate(-1),
mHasFrameBuffer(false),
mFrameBuffers(NULL),
mFrameBufferTextures(NULL),
mGLCubeBuffer(NULL),
mGLTextureBuffer(NULL)
{
	if (!vertexShader) {
		mVertexShader = defaultVetexShaderStr;
	}
	else mVertexShader = vertexShader;
	if (!fragmentShader) {
		mFragmentShader = defaultFragmentShaderStr;
	}
	else mFragmentShader = fragmentShader;
}


bool DD::DDFilter::init() {
	if ((mGLProgId = loadProgram(mVertexShader, mFragmentShader)) == 0) {
		return false;
	}
	mGLCubeBuffer = (GLfloat*)malloc(sizeof(TextureUtil_CUBE));
	memcpy(mGLCubeBuffer, TextureUtil_CUBE, sizeof(TextureUtil_CUBE));
	mGLTextureBuffer = TextureUtil_getRotation(NO_ROTATION,false,true);
	mGLAttribPosition = glGetAttribLocation(mGLProgId, "position");
	mGLUniformTexture = glGetUniformLocation(mGLProgId, "inputImageTexture");
	mGLAttribTextureCoordinate = glGetAttribLocation(mGLProgId,"inputTextureCoordinate");
	int err = 0;
	if (onInit() && (err = glGetError()) == 0){
		mIsInitialized = true;
		return true;
	}
	else {
		glPrintError(getProgramID());
		destroy();
		return false;
	}
}


void DD::DDFilter::destroy() {
	if (mGLProgId != 0) {
		glDeleteProgram(mGLProgId);
	}
	destroyFramebuffers();
	if (mGLCubeBuffer){
		free(mGLCubeBuffer);
		mGLCubeBuffer = NULL;
	}
	if (mGLTextureBuffer) {
		free(mGLTextureBuffer);
		mGLTextureBuffer = NULL;
	}
	onDestroy();
}

void DD::DDFilter::setHasFrameBuffer(bool hasFrameBuffer) {
	mHasFrameBuffer = hasFrameBuffer;
}

void DD::DDFilter::destroyFramebuffers() {
	if (mFrameBuffers != NULL) {
		glDeleteFramebuffers(1, mFrameBuffers);
		mFrameBuffers = NULL;
	}
	if (mFrameBufferTextures != NULL) {
		glDeleteTextures(1, mFrameBufferTextures);
		mFrameBufferTextures = NULL;
	}
}

int DD::DDFilter::onDraw(GLuint textureId, GLfloat* cubeBuffer, GLfloat* textureBuffer) {
	if (!mIsInitialized) {
		return GLES2_NOT_INIT;
	}
	glUseProgram(mGLProgId);
	runPendingOnDrawTasks();
	glVertexAttribPointer(mGLAttribPosition, 2, GL_FLOAT, false, 0, cubeBuffer);
	glEnableVertexAttribArray(mGLAttribPosition);
	glVertexAttribPointer(mGLAttribTextureCoordinate, 2, GL_FLOAT, false, 0, textureBuffer);
	glEnableVertexAttribArray(mGLAttribTextureCoordinate);
	if (textureId != GLES2_NO_TEXTURE) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glUniform1i(mGLUniformTexture, 0);
	}
	onDrawArraysPre();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(mGLAttribPosition);
	glDisableVertexAttribArray(mGLAttribTextureCoordinate);
	glBindTexture(GL_TEXTURE_2D, 0);
	return GLES2_ON_DRAW;
}

void DD::DDFilter::runPendingOnDrawTasks() {
	std::function<void()> task1;
	bool hasTask = false;
	while (true) {
		mDrawLock.lock();
		if (mOndrawTasks.size() > 0) {
			task1 = mOndrawTasks.front();
			hasTask = true;
			mOndrawTasks.pop_front();
		}
		mDrawLock.unlock();
		if (hasTask) {
			task1();
			hasTask = false;
		}
		else break;
	}
}

void DD::DDFilter::setInteger(GLint location, GLint intValue) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniform1i(location, intValue);
	});
	mDrawLock.unlock();
}

void DD::DDFilter::runOnDraw(std::function<void()> task) {
	mOndrawTasks.push_back(task);
}

void DD::DDFilter::setFloat(GLint location, GLfloat floatValue) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniform1f(location, floatValue);
	});
	mDrawLock.unlock();
}

void DD::DDFilter::setFloatArray(GLint location, GLsizei count, GLfloat* floatValue) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniform1fv(location, count, floatValue);
		delete[] floatValue;
	});
	mDrawLock.unlock();
}

void DD::DDFilter::setFloatVec2(GLint location, GLfloat* arrayValue) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniform2fv(location, 1, arrayValue);
		delete[] arrayValue;
	});
	mDrawLock.unlock();
}

void DD::DDFilter::setFloatVec3(GLint location, GLfloat* arrayValue) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniform3fv(location, 1, arrayValue);
		delete[] arrayValue;
	});
	mDrawLock.unlock();
}

void DD::DDFilter::setFloatVec4(GLint location, GLfloat* arrayValue) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniform4fv(location, 1, arrayValue);
		delete[] arrayValue;
	});
	mDrawLock.unlock();
}

void DD::DDFilter::setPoint(GLint location, PointF point) {
	GLfloat* pi = new GLfloat[2];
	pi[0] = point.x;
	pi[1] = point.y;
	setFloatVec2(location, pi);
}

void DD::DDFilter::setUniformMatrix3f(GLint location, GLfloat* matrix) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
//        transpose : 指明矩阵是列优先(column major)矩阵（GL_FALSE）还是行优先(row major)矩阵（GL_TRUE）
//        opengl ES必须为false
		glUniformMatrix3fv(location, 1, false, matrix);
		delete[] matrix;
	});
    mDrawLock.unlock();
}

void DD::DDFilter::setUniformMatrix4f(GLint location, GLfloat* matrix) {
	mDrawLock.lock();
	mOndrawTasks.push_back([=] {
		glUniformMatrix4fv(location, 1, false, matrix);
		delete[] matrix;
	});
    mDrawLock.unlock();
}

int DD::DDFilter::createTexture(int width /*= 0*/, int heigth /*= 0*/) {
	GLuint ti[1];
	glGenTextures(1, ti);
	if (ti[0] <= 0)return 0;
	glBindTexture(GL_TEXTURE_2D, ti[0]);
	if (width != 0 || heigth != 0) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, heigth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return ti[0];
}

void DD::DDFilter::scaleClipAndRotate(int oriWidth, int oriHeight, float ratio, int orientaion, bool xMirror, bool yMirror) {
	if (mGLTextureBuffer) {
		delete mGLTextureBuffer;
	}
	mGLTextureBuffer = TextureUtil_getRotation(NO_ROTATION, false, true);

	GLfloat* oriFragmentPointer = mGLTextureBuffer;

	//clip
	int clipWidth = oriWidth;
	int clipHeight = oriHeight;
	if (oriWidth / (float)oriHeight > ratio) {
		clipHeight = oriHeight;
		clipWidth = (int)(clipHeight * ratio);
	}
	else if (oriWidth / (float)oriHeight < ratio) {
		clipWidth = oriWidth;
		clipHeight = (int)(clipWidth / ratio);
	}
	float xClip = (float)clipWidth / oriWidth;
	float yClip = (float)clipHeight / oriHeight;
	xClip = (1 - xClip) / 2;
	yClip = (1 - yClip) / 2;
	for (int i = 0; i < 4; i++) {
		if (oriFragmentPointer[2 * i] < 0.5f)
			oriFragmentPointer[2 * i] += xClip;
		else oriFragmentPointer[2 * i] -= xClip;
		if (oriFragmentPointer[2 * i + 1] < 0.5f)
			oriFragmentPointer[2 * i + 1] += yClip;
		else oriFragmentPointer[2 * i + 1] -= yClip;
	}

	//rotate
	int k = orientaion / 90;
	for (int i = 0; i < k; i++) {
		float tX = oriFragmentPointer[0];
		float tY = oriFragmentPointer[1];
		oriFragmentPointer[0] = oriFragmentPointer[2];
		oriFragmentPointer[1] = oriFragmentPointer[3];
		oriFragmentPointer[2] = oriFragmentPointer[6];
		oriFragmentPointer[3] = oriFragmentPointer[7];
		oriFragmentPointer[6] = oriFragmentPointer[4];
		oriFragmentPointer[7] = oriFragmentPointer[5];
		oriFragmentPointer[4] = tX;
		oriFragmentPointer[5] = tY;
	}

	//mirror
	if (k == 0 || k == 2) {
		if (xMirror) {
			oriFragmentPointer[0] = 1.0f - oriFragmentPointer[0];
			oriFragmentPointer[2] = 1.0f - oriFragmentPointer[2];
			oriFragmentPointer[4] = 1.0f - oriFragmentPointer[4];
			oriFragmentPointer[6] = 1.0f - oriFragmentPointer[6];
		}
		if (yMirror) {
			oriFragmentPointer[1] = 1.0f - oriFragmentPointer[1];
			oriFragmentPointer[3] = 1.0f - oriFragmentPointer[3];
			oriFragmentPointer[5] = 1.0f - oriFragmentPointer[5];
			oriFragmentPointer[7] = 1.0f - oriFragmentPointer[7];
		}
	}
	else {
		if (yMirror) {
			oriFragmentPointer[0] = 1.0f - oriFragmentPointer[0];
			oriFragmentPointer[2] = 1.0f - oriFragmentPointer[2];
			oriFragmentPointer[4] = 1.0f - oriFragmentPointer[4];
			oriFragmentPointer[6] = 1.0f - oriFragmentPointer[6];
		}
		if (xMirror) {
			oriFragmentPointer[1] = 1.0f - oriFragmentPointer[1];
			oriFragmentPointer[3] = 1.0f - oriFragmentPointer[3];
			oriFragmentPointer[5] = 1.0f - oriFragmentPointer[5];
			oriFragmentPointer[7] = 1.0f - oriFragmentPointer[7];
		}
	}
}

void DD::DDFilter::flipX() {
    if (mGLTextureBuffer) {
        for (int i = 0; i < 8; i += 2) {
            mGLTextureBuffer[i] = 1.f - mGLTextureBuffer[i];
        }
    }
}

void DD::DDFilter::flipY() {
    if (mGLTextureBuffer) {
        for (int i = 1; i < 8; i += 2) {
            mGLTextureBuffer[i] = 1.f - mGLTextureBuffer[i];
        }
    }
}

void DD::DDFilter::resetTextureMatrix(){
    mGLTextureBuffer = TextureUtil_getRotation(NO_ROTATION, false, true);
}

int DD::DDFilter::onDrawToTexture(GLuint textureId, GLuint frameBuffers, GLuint frameBufferTextures) {
	if (frameBuffers == NULL)
		return GLES2_NO_TEXTURE;
	if (!mIsInitialized) {
		return GLES2_NOT_INIT;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers);
	int ret = onDraw(textureId,mGLCubeBuffer,mGLTextureBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (ret == GLES2_ON_DRAW){
		ret = frameBufferTextures;
	}
	else {
		ret = 0;
	}

	return ret;
}

int DD::DDFilter::onDrawToTexture(GLuint textureId) {
	return onDrawToTexture(textureId, mFrameBuffers[0], mFrameBufferTextures[0]);
}

int DD::DDFilter::onDrawFrame(GLuint textureId) {
	return onDrawFrame(textureId, mGLCubeBuffer, mGLTextureBuffer);
}

void DD::DDFilter::setVertexTexturePosition(GLfloat* cubeBuffer, GLfloat* texBuffer) {
	if (cubeBuffer){
		if (mGLCubeBuffer) {
			delete[]mGLCubeBuffer;
			mGLCubeBuffer = NULL;
		}
		mGLCubeBuffer = cubeBuffer;
	}
	if (texBuffer){
		if (mGLTextureBuffer) {
			delete[]mGLTextureBuffer;
			mGLTextureBuffer = NULL;
		}
		mGLTextureBuffer = texBuffer;
	}
}

int DD::DDFilter::onDrawFrame(GLuint textureId, GLfloat* cubeBuffer, GLfloat* textureBuffer) {
	if (!mIsInitialized) {
		return GLES2_NOT_INIT;
	}
	int ret = onDraw(textureId, cubeBuffer, textureBuffer);
	if (ret == GLES2_ON_DRAW) {
		ret = textureId;
	}
	else {
		ret = 0;
	}
	return ret;
}

void DD::DDFilter::outputSizeChanged(int width, int height) {
	if (mOutputHeight == height && mOutputWidth == width){
		return;
	}
	mOutputWidth = width;
	mOutputHeight = height;
	if (mHasFrameBuffer){
		if (mFrameBuffers != NULL){
			destroyFramebuffers();
		}
		mFrameBuffers = new GLuint[1];
		mFrameBufferTextures = new GLuint[1];
		glGenFramebuffers(1, mFrameBuffers);
		glGenTextures(1, mFrameBufferTextures);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[0]);
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
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, mFrameBufferTextures[0], 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	onOutputSizeChanged(width, height);
}
