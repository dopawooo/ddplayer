//
//  dd_face.m
//  IJKMediaPlayer
//
//  Copyright © 2017年 bilibili. All rights reserved.
//


#import "dd_face.h"
#import "MGFaceLicenseHandle.h"
#import "MGFacepp.h"
#import "MGFaceModelArray.h"
#import "MarkVideoViewController.h"
#import <AVFoundation/AVMediaFormat.h>
#import<AVFoundation/AVCaptureDevice.h>
#import<AssetsLibrary/AssetsLibrary.h>
#import <CoreMotion/CoreMotion.h>

static DDFacePPHelper* _instance = nil;

@interface DDFacePPHelper()<MGVideoDelegate>{
    MGFacepp *markManager;
    dispatch_queue_t detectImageQueue;
    CGSize size;
    CMMotionManager *motionManager;
    MGVideoManager *videoManager;
    BOOL hasVideoFormatDescription;
    id<FaceDetectDelegate> delegate;
}
@end


@implementation DDFacePPHelper

+(id)getInstance{
    if (_instance == nil) {
        _instance = [[DDFacePPHelper alloc] init];
    }
    return _instance;
}

-(BOOL)initFacePPSDK{
    
    self.pointsNum = 81;
    self.debug = true;
    self.orientation = 90;
    //size = CGSizeMake(640, 480);
    size = CGSizeMake(1280,720);
    /** 进行联网授权版本判断，联网授权就需要进行网络授权 */
    BOOL needLicense = [MGFaceLicenseHandle getNeedNetLicense];
    
    if (needLicense) {
        [MGFaceLicenseHandle licenseForNetwokrFinish:^(bool License, NSDate *sdkDate) {
            NSLog(@"本次联网授权是否成功 %d, SDK 过期时间：%@", License, sdkDate);
        }];
    }

    NSDictionary *tempDic = @{@"0":@"Track",
                              @"1":@"Detect",
                              @"2":@"Pose3d",
                              @"3":@"EyeStatus",
                              @"4":@"MouseStatus",
                              @"5":@"Minority",
                              @"6":@"Blurness",
                              @"7":@"AgeGender",
                              @"8":@"ExtractFeature",};
    
    
    NSString *modelPath = [[NSBundle mainBundle] pathForResource:KMGFACEMODELNAME ofType:@""];
    NSData *modelData = [NSData dataWithContentsOfFile:modelPath];
    
    MGAlgorithmInfo *sdkInfo = [MGFacepp getSDKAlgorithmInfoWithModel:modelData];
    
    NSMutableString *tempString = [NSMutableString stringWithString:@""];
    
    [tempString appendFormat:@"\n%@: %@",NSLocalizedString(@"debug_message1", nil), sdkInfo.needNetLicense?@"YES":@"NO"];
    [tempString appendFormat:@"\n%@: %@",NSLocalizedString(@"debug_message2", nil),sdkInfo.version];
    [tempString appendFormat:@"\n%@: {\n", NSLocalizedString(@"debug_message3", nil)];
    
    for (int i = 0; i < sdkInfo.SDKAbility.count; i++) {
        NSNumber *tempValue = (NSNumber *)sdkInfo.SDKAbility[i];
        NSInteger function = [tempValue integerValue];
        
        NSString *temp = [tempDic valueForKey:[NSString stringWithFormat:@"%zi", function]];
        [tempString appendFormat:@"    %@", temp];
        [tempString appendString:@",\n"];
    }
    [tempString appendString:@"}"];
    
//    [self.messageView setText:tempString];
    NSLog(tempString);
    
    detectImageQueue = dispatch_queue_create("com.megvii.image.detect", DISPATCH_QUEUE_SERIAL);
    
    int trackingMode = 2;
    
    MGDetectROI detectROI = MGDetectROIMake(0, 0, 0, 0);
    
    markManager = [[MGFacepp alloc] initWithModel:modelData
                                    faceppSetting:^(MGFaceppConfig *config) {
                                        config.minFaceSize = 100;
                                        config.interval = 60;
                                        config.orientation = 90;
                                        config.oneFaceTracking = NO;
                                        config.detectionMode = (trackingMode == 1 ? MGFppDetectionModeTracking : (MGFppDetectionMode)(trackingMode+1));
                                        config.detectROI = detectROI;
                                        config.pixelFormatType = PixelFormatTypeRGBA;
                                    }];
    if(markManager == nil){
        return false;
    }
    
    AVCaptureDevicePosition devicePosition = [self getCamera:NO];
    //videoManager = [MGVideoManager videoPreset:AVCaptureSessionPreset640x480
    videoManager = [MGVideoManager videoPreset:AVCaptureSessionPreset1280x720
                                devicePosition:devicePosition
                                   videoRecord:NO
                                    videoSound:NO];
    
    videoManager.videoDelegate = self;
    
    motionManager = [[CMMotionManager alloc] init];
    motionManager.accelerometerUpdateInterval = 0.3f;
    
    NSOperationQueue *motionQueue = [[NSOperationQueue alloc] init];
    [motionQueue setName:@"com.megvii.gryo"];
    [motionManager startAccelerometerUpdatesToQueue:motionQueue
                                        withHandler:^(CMAccelerometerData * _Nullable accelerometerData, NSError * _Nullable error) {
                                            
                                            if (fabs(accelerometerData.acceleration.z) > 0.7) {
                                                self.orientation = 90;
                                            }else{
                                                
                                                if (AVCaptureDevicePositionBack == devicePosition) {
                                                    if (fabs(accelerometerData.acceleration.x) < 0.4) {
                                                        self.orientation = 90;
                                                    }else if (accelerometerData.acceleration.x > 0.4){
                                                        self.orientation = 180;
                                                    }else if (accelerometerData.acceleration.x < -0.4){
                                                        self.orientation = 0;
                                                    }
                                                }else{
                                                    if (fabs(accelerometerData.acceleration.x) < 0.4) {
                                                        self.orientation = 90;
                                                    }else if (accelerometerData.acceleration.x > 0.4){
                                                        self.orientation = 0;
                                                    }else if (accelerometerData.acceleration.x < -0.4){
                                                        self.orientation = 180;
                                                    }
                                                }
                                                
                                                if (accelerometerData.acceleration.y > 0.6) {
                                                    self.orientation = 270;
                                                }
                                            }
                                        }];
    
    return TRUE;
}

- (void)alertView:(NSString*)log
{
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"警告" message:@"人脸识别模块启动失败" delegate:nil cancelButtonTitle:@"取消" otherButtonTitles:@"确定", nil];
    [alert show];
}

- (void)startFaceDetect{
    if(videoManager == nil){
        
        return;
    }
    AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (authStatus == AVAuthorizationStatusAuthorized) {
        [videoManager startRunning];
    }
    else if (authStatus ==AVAuthorizationStatusRestricted ||//此应用程序没有被授权访问的照片数据。可能是家长控制权限
             authStatus ==AVAuthorizationStatusDenied)  //用户已经明确否认了这一照片数据的应用程序访问
    {
        // 无权限 引导去开启
        NSURL *url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
        if ([[UIApplication sharedApplication]canOpenURL:url]) {
            [[UIApplication sharedApplication]openURL:url];
        }
        [self startFaceDetect];
    }
    else if(AVAuthorizationStatusNotDetermined == authStatus){
        dispatch_semaphore_t sig = dispatch_semaphore_create(0);
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
            dispatch_async(dispatch_get_main_queue(), ^(){
                [self startFaceDetect];
            });
            dispatch_semaphore_signal(sig);
        }];
        dispatch_semaphore_wait(sig, DISPATCH_TIME_FOREVER);
    }
}

-(void)stopFaceDetect{
    [videoManager stopRunning];
}

-(void)setDelegate:(id<FaceDetectDelegate>) d{
    delegate = d;
}

- (AVCaptureDevicePosition)getCamera:(BOOL)index{
    AVCaptureDevicePosition tempVideo;
    if (index == NO) {
        tempVideo = AVCaptureDevicePositionFront;
    }else{
        tempVideo = AVCaptureDevicePositionBack;
    }
    return tempVideo;
}

-(void)MGCaptureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection{
    @synchronized(self) {
//        if (hasVideoFormatDescription == NO) {
//            [self setupVideoPipelineWithInputFormatDescription:[videoManager formatDescription]];
//        }
        
        [self rotateAndDetectSampleBuffer:sampleBuffer];
    }
}

- (void)MGCaptureOutput:(AVCaptureOutput *)captureOutput error:(NSError *)error{
    NSLog(@"%@", error);
    if (error.code == 101) {
        UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"alert_message2", nil)
                                                            message:NSLocalizedString(@"alert_message2", nil)
                                                           delegate:nil cancelButtonTitle:NSLocalizedString(@"alert_message3", nil)
                                                  otherButtonTitles:nil, nil];
        [alertView show];
    }
    
}

- (void)rotateAndDetectSampleBuffer:(CMSampleBufferRef)sampleBuffer{
    
    if (markManager.status != MGMarkWorking) {
        
        CMSampleBufferRef detectSampleBufferRef = NULL;
        CMSampleBufferCreateCopy(kCFAllocatorDefault, sampleBuffer, &detectSampleBufferRef);
        
        void(^detectTask)()=^{
            @autoreleasepool {

                if ([markManager getFaceppConfig].orientation != self.orientation) {
                    [markManager updateFaceppSetting:^(MGFaceppConfig *config) {
                        config.orientation = self.orientation;
                    }];
                }

                MGImageData *imageData = [[MGImageData alloc] initWithSampleBuffer:detectSampleBufferRef];

                [markManager beginDetectionFrame];

                NSDate *date1, *date2, *date3;
                date1 = [NSDate date];

                NSArray *tempArray = [markManager detectWithImageData:imageData];

                date2 = [NSDate date];
                double timeUsed = [date2 timeIntervalSinceDate:date1] * 1000;

                MGFaceModelArray *faceModelArray = [[MGFaceModelArray alloc] init];
                faceModelArray.getFaceInfo = self.faceInfo;
                faceModelArray.faceArray = [NSMutableArray arrayWithArray:tempArray];
                faceModelArray.timeUsed = timeUsed;
                faceModelArray.get3DInfo = self.show3D;
                faceModelArray.getFaceInfo = self.faceInfo;
                [faceModelArray setDetectRect:self.detectRect];

                if (faceModelArray.count >= 1) {
                    MGFaceInfo *faceInfo = faceModelArray.faceArray[0];
                    [markManager GetGetLandmark:faceInfo isSmooth:YES pointsNumber:self.pointsNum];
                    if(delegate != nil){
                        [delegate onFaceDetected:faceInfo.points VideoSize:size];
                    }
                }

                date3 = [NSDate date];
                double timeUsed3D = [date3 timeIntervalSinceDate:date2] * 1000;
                faceModelArray.AttributeTimeUsed = timeUsed3D;

                [markManager endDetectionFrame];
                CFRelease(detectSampleBufferRef);
            }

        };
        /* 进入检测人脸专用线程 */
        dispatch_async(detectImageQueue, detectTask);
    }
}

@end
