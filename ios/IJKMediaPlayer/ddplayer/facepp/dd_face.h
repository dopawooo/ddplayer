//
//  dd_face.h
//  IJKMediaPlayer
//
//  Copyright © 2017年 bilibili. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreMedia/CoreMedia.h>
#import <UIKit/UIKit.h>

@protocol FaceDetectDelegate<NSObject>
@required
-(void) onFaceDetected:(NSArray <NSValue *>*)points VideoSize:(CGSize) size;
@end

@interface DDFacePPHelper :NSObject

@property (nonatomic, assign) CGRect detectRect;
@property (nonatomic, assign) CGSize videoSize;
@property (nonatomic, assign) BOOL debug;
@property (nonatomic, assign) BOOL show3D;
@property (nonatomic, assign) BOOL faceInfo;
@property (nonatomic, assign) int orientation;
@property (nonatomic, assign) int pointsNum;

+(id)getInstance;
-(void)setDelegate:(id<FaceDetectDelegate>) delegate;
-(void)startFaceDetect;
-(void)stopFaceDetect;
-(BOOL)initFacePPSDK;
@end
