#import "iOSDDFilter.h"


typedef struct GPUTextureOptions {
    GLenum minFilter;
    GLenum magFilter;
    GLenum wrapS;
    GLenum wrapT;
    GLenum internalFormat;
    GLenum format;
    GLenum type;
} GPUTextureOptions;

typedef NS_ENUM(NSInteger, TXEFrameFormat) {
    TXE_FRAME_FORMAT_NONE = 0,
    TXE_FRAME_FORMAT_NV12 = 1,        //NV12格式
    TXE_FRAME_FORMAT_I420 = 2,        //I420格式
    TXE_FRAME_FORMAT_RGBA = 3,        //RGBA格式
};


@interface iOSDDFilter(){
    DDFilter* mFilter;
    CVOpenGLESTextureCacheRef coreVideoTextureCache;
    EAGLContext *context;
    GPUTextureOptions textureOptions;
    CVOpenGLESTextureRef renderTexture;
    GLuint texture;
    GLuint framebuffer;
    
    int width;
    int height;
}
@property (nonatomic, assign) TXEFrameFormat outputFormat;
@property (nonatomic, assign) CMSampleBufferRef sampleBuffer;
@property (nonatomic, assign) CVPixelBufferRef pixelBuffer;
//@property (nonatomic, strong) TXCRGB2YUVOutput* rgbFilter;
@end

@implementation iOSDDFilter

-(GLuint)getFrameBuffer{
    return framebuffer;
}

-(GLuint)getOutTexture{
    return texture;
}

-(CMSampleBufferRef)getOutSampleBuffer{
    return _sampleBuffer;
}

-(void)destroy{
    if (mFilter != NULL) {
        mFilter->destroy();
        delete mFilter;
        mFilter = NULL;
    }
    if (framebuffer)
    {
        glDeleteFramebuffers(1, &framebuffer);
        framebuffer = 0;
    }
    if (_pixelBuffer)
    {
        CFRelease(_pixelBuffer);
        _pixelBuffer = NULL;
    }
    if (renderTexture)
    {
        CFRelease(renderTexture);
        renderTexture = NULL;
    }
    if (_sampleBuffer){
        CFRelease(_sampleBuffer);
        _sampleBuffer = NULL;
    }
}

-(void)outputSizeChanged:(int)width Height:(int)height{
    if(self->width != width || self->height != height){
        self->width = width;
        self->height = height;
        if(mFilter != NULL){
            mFilter->outputSizeChanged(width, height);
        }
        [self genFrameBuffer:CGSizeMake(width, height)];
        
    }
}

-(id)initWithSize:(CGSize) size{
    return [self initWithSize:size vertexShader:NULL fragmentShader:NULL];
}

-(id)initWithSize:(CGSize) size fragmentShader:(NSString*) fragment{
    return [self initWithSize:size vertexShader:NULL fragmentShader:[fragment UTF8String]];
}

-(void)genFrameBuffer:(CGSize) size{
    if (framebuffer)
    {
        glDeleteFramebuffers(1, &framebuffer);
        framebuffer = 0;
    }
    if (renderTexture)
    {
        CFRelease(renderTexture);
        renderTexture = NULL;
    }
    if (_sampleBuffer){
        CFRelease(_sampleBuffer);
        _sampleBuffer = NULL;
    }
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    if (coreVideoTextureCache == NULL)
    {
#if defined(__IPHONE_6_0)
        CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, [EAGLContext currentContext], NULL, &coreVideoTextureCache);
#else
        CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, (__bridge void *)[EAGLContext currentContext], NULL, &_coreVideoTextureCache);
#endif
        
        if (err)
        {
            NSAssert(NO, @"Error at CVOpenGLESTextureCacheCreate %d", err);
        }
        
    }
    
    //创建pixel buffer与opengl纹理，二者建立联系
    CFDictionaryRef empty; // empty value for attr value.
    CFMutableDictionaryRef attrs;
    empty = CFDictionaryCreate(kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks); // our empty IOSurface properties dictionary
    attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(attrs, kCVPixelBufferIOSurfacePropertiesKey, empty);
    //kCVPixelFormatType_32BGRA
    //kCVPixelFormatType_420YpCbCr8BiPlanarFullRange  nv12
    CVReturn err = CVPixelBufferCreate(kCFAllocatorDefault, (int)size.width, (int)size.height, kCVPixelFormatType_32BGRA, attrs, &_pixelBuffer);
    if (err)
    {
        LOGE("FBO size: %f, %f", size.width, size.height);
        NSAssert(NO, @"Error at CVPixelBufferCreate %d", err);
    }
    err = CVOpenGLESTextureCacheCreateTextureFromImage (kCFAllocatorDefault, coreVideoTextureCache, _pixelBuffer,
                                                        NULL, // texture attributes
                                                        GL_TEXTURE_2D,
                                                        textureOptions.internalFormat, // opengl format
                                                        (int)size.width,
                                                        (int)size.height,
                                                        textureOptions.format, // native iOS format
                                                        textureOptions.type,
                                                        0,
                                                        &renderTexture);
    if (err)
    {
        NSAssert(NO, @"Error at CVOpenGLESTextureCacheCreateTextureFromImage %d", err);
    }
    
    CFRelease(attrs);
    CFRelease(empty);
    
    //创建sample buffer
    CMVideoFormatDescriptionRef videoInfo = NULL;
    CMVideoFormatDescriptionCreateForImageBuffer(NULL, _pixelBuffer, &videoInfo);
    CMTime duration = CMTimeMake(1, 60);
    NSTimeInterval systemUptime = [[NSProcessInfo processInfo] systemUptime];
    CMTime nowTime = CMTimeMake(systemUptime * 1000, 1000);
    CMSampleTimingInfo timing = {duration, nowTime, kCMTimeInvalid};
    
    CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault, _pixelBuffer, YES, NULL, NULL, videoInfo, &timing, &_sampleBuffer);
    CFRelease(videoInfo);
    
    //绑定gltexture 与 framebuffer
    glBindTexture(CVOpenGLESTextureGetTarget(renderTexture), CVOpenGLESTextureGetName(renderTexture));
    texture = CVOpenGLESTextureGetName(renderTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureOptions.wrapS);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureOptions.wrapT);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CVOpenGLESTextureGetName(renderTexture), 0);
}

-(id)initWithSize:(CGSize) size vertexShader:(const char*)vertex fragmentShader:(const char*) fragment{
    BOOL ret = TRUE;
    if(self = [super init]){
            GPUTextureOptions defaultTextureOptions;
            defaultTextureOptions.minFilter = GL_LINEAR;
            defaultTextureOptions.magFilter = GL_LINEAR;
            defaultTextureOptions.wrapS = GL_CLAMP_TO_EDGE;
            defaultTextureOptions.wrapT = GL_CLAMP_TO_EDGE;
            defaultTextureOptions.internalFormat = GL_RGBA;
            defaultTextureOptions.format = GL_BGRA;
            defaultTextureOptions.type = GL_UNSIGNED_BYTE;
        
            textureOptions = defaultTextureOptions;
        
        [self destroy];
        mFilter = new DDFilter(vertex, fragment);
        ret = mFilter->init();
        if(ret){
            mFilter->outputSizeChanged(size.width, size.height);
        }
        else{
            [self destroy];
            return nil;
        }
        [self genFrameBuffer:size];
    }
    width = size.width;
    height = size.height;
    return ret ? self : nil;
}

- (CMSampleBufferRef)getSampleBuffer
{
    glFinish();
    
//    CVPixelBufferLockBaseAddress(_pixelBuffer, 0);
//    Byte *byte = (Byte *)CVPixelBufferGetBaseAddress(_pixelBuffer);
//    CGSize bufferSize = CVImageBufferGetDisplaySize(_pixelBuffer);
//
//
//    Byte *y = (Byte*)CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 0);
//    memcpy(y, byte, bufferSize.width * bufferSize.height);
//    if(self.outputFormat == TXE_FRAME_FORMAT_I420){
//        Byte *u = (Byte*)CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 1);
//        Byte *v = (Byte*)CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 2);
//        memcpy(u, byte + (int)(bufferSize.width * bufferSize.height), bufferSize.width * bufferSize.height / 4);
//        memcpy(v, byte + (int)(bufferSize.width * bufferSize.height * 5 / 4), bufferSize.width * bufferSize.height / 4);
//    }
//    else if(self.outputFormat == TXE_FRAME_FORMAT_NV12){
//        Byte *u = (Byte*)CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 1);
//        memcpy(u, byte + (int)(bufferSize.width * bufferSize.height), bufferSize.width * bufferSize.height / 2);
//    }
//    else if(self.outputFormat == TXE_FRAME_FORMAT_RGBA){
//        memcpy(y + (int )(bufferSize.width * bufferSize.height), byte + (int )(bufferSize.width * bufferSize.height), bufferSize.width * bufferSize.height * 3);
//    }
//    CVPixelBufferUnlockBaseAddress(_pixelBuffer, 0);
//
    return _sampleBuffer;
}
@end

