#ifndef __DD_FILTER_H__
#define __DD_FILTER_H__
#include "DDGLUtil.h"
#include <list>
#include <vector>
#include "Mutex.h"
#include <functional>

namespace DD {
	extern GLfloat TextureUtil_CUBE[];
	extern GLfloat TextureUtil_NO_ROTATION[];
	extern GLfloat TextureUtil_ROTATED_90[];
	extern GLfloat TextureUtil_ROTATED_180[];
	extern GLfloat TextureUtil_ROTATED_270[];
	enum TextureUtil_ROTATION{
		NO_ROTATION,
		ROTATION_90,
		ROTATION_180,
		ROTATION_270,
	};
	GLfloat* TextureUtil_getRotation(TextureUtil_ROTATION rotation, bool flipHorizon, bool filpVertical);

	class PointF {
	public:
		float x;
		float y;
		PointF() {}
		PointF(float x, float y) {
			this->x = x;
			this->y = y;
		}
	};
	class DDFilter {
	public:		
		DDFilter(const char* vertexShader = NULL, const char* fragmentShader = NULL);

		//以下接口为父类统一接口，禁止实现为虚函数且不推荐重载！子类重载后使用时请小心！
		bool init();
		void destroy();

		void outputSizeChanged(int width, int height);
		int onDrawToTexture(GLuint textureId, GLuint frameBuffers, GLuint frameBufferTextures);
		int onDrawToTexture(GLuint textureId);
		int onDrawFrame(GLuint textureId);
		int onDrawFrame(GLuint textureId, GLfloat* cubeBuffer, GLfloat* textureBuffer);
		void setVertexTexturePosition(GLfloat* cubeBuffer, GLfloat* texBuffer);

		void setHasFrameBuffer(bool hasFrameBuffer);
		void destroyFramebuffers();
		int getProgramID() {
			return mGLProgId;
		}
		void setInteger(GLint location, GLint intValue);		
		void setFloat(GLint location, GLfloat floatValue);		
		void setFloatArray(GLint location, GLsizei count, GLfloat* floatValue);		
		void setFloatVec2(GLint location, GLfloat* arrayValue);		
		void setFloatVec3(GLint location, GLfloat* arrayValue);
		void setFloatVec4(GLint location, GLfloat* arrayValue);
		void setPoint(GLint location, PointF point);
		void setUniformMatrix3f(GLint location, GLfloat* matrix);		
		void setUniformMatrix4f(GLint location, GLfloat* matrix);
        
        void flipX();
        void flipY();
        void resetTextureMatrix();

		void runOnDraw(std::function<void()> task);

		static int createTexture(int width = 0, int heigth = 0);		

		void scaleClipAndRotate(int oriWidth, int oriHeight, float ratio, int orientaion, bool xMirror, bool yMirror);
	protected:
		//以下四个接口提供给子类实现特殊功能
		virtual int onDraw(GLuint textureId, GLfloat* cubeBuffer, GLfloat* textureBuffer);
		virtual bool onInit() {
			return true;
		}
		virtual void onDestroy() {

		}
		virtual void onOutputSizeChanged(int width, int height) {

		}
		virtual void onDrawArraysPre() {

		}

		void runPendingOnDrawTasks();
		int mGLProgId;
		int mOutputWidth;
		int mOutputHeight;
		int mIntputWidth;
		int mIntputHeight;
		bool mIsInitialized;
		int mGLAttribPosition;
		int mGLUniformTexture;
		int mGLAttribTextureCoordinate;
		bool mHasFrameBuffer;
		GLuint* mFrameBuffers;
		GLuint* mFrameBufferTextures;
		GLfloat* mGLCubeBuffer;
		GLfloat* mGLTextureBuffer;

		const char* mVertexShader;
		const char* mFragmentShader;
		static const char defaultVetexShaderStr[];
		static const char defaultFragmentShaderStr[];

		std::list<std::function<void()>> mOndrawTasks;
		XPLock	mDrawLock;
	private:
	};
}

#endif
