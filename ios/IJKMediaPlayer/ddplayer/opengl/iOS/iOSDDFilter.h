#ifndef iOSDDFilter_h
#define iOSDDFilter_h

#import <Foundation/Foundation.h>
#import "DDFilter.h"
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/EAGL.h>
#import <CoreMedia/CoreMedia.h>
using namespace DD;
@interface iOSDDFilter : NSObject

-(void)destroy;
-(id)initWithSize:(CGSize) size;
-(id)initWithSize:(CGSize) size fragmentShader:(NSString*) fragment;
-(id)initWithSize:(CGSize) size vertexShader:(const char*)vertex fragmentShader:(const char*) fragment;
-(void)outputSizeChanged:(int)width Height:(int)height;
-(GLuint)getFrameBuffer;
-(GLuint)getOutTexture;
-(CMSampleBufferRef)getOutSampleBuffer;
@end

#endif /* iOSDDFilter_h */
