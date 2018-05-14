#ifndef  _APP_DELEGATE_H_
#define  _APP_DELEGATE_H_

#include "cocos2d.h"
#include "LAppAllocator.hpp"
#include <CubismFramework.hpp>

class EventListenerCustom;

/**
@brief    The cocos2d Application.

Private inheritance here hides part of interface from Director.
*/
class AppDelegate : private cocos2d::Application
{
public:
    AppDelegate();
    virtual ~AppDelegate();

    virtual void initGLContextAttrs();

    /**
    @brief    Implement Director and Scene init code here.
    @return true    Initialize success, app continue.
    @return false   Initialize failed, app terminate.
    */
    virtual bool applicationDidFinishLaunching();

    /**
    @brief  Called when the application moves to the background
    @param  the pointer of the application
    */
    virtual void applicationDidEnterBackground();

    /**
    @brief  Called when the application reenters the foreground
    @param  the pointer of the application
    */
    virtual void applicationWillEnterForeground();
    
    virtual void record();
    
    virtual void ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb);

    virtual void ffmpeg_encoder_start(const char *filename, int codec_id, int fps, int width, int height);
    
    virtual void ffmpeg_encoder_finish(void);
    
    virtual void ffmpeg_encoder_encode_frame(uint8_t *rgb);
    
    virtual void ffmpeg_encoder_glread_rgb(uint8_t **rgb, GLubyte **pixels, unsigned int width, unsigned int height);
    
private:
    cocos2d::EventListenerCustom* _recreatedEventlistener;

    LAppAllocator                  _cubismAllocator;
    Csm::CubismFramework::Option   _cubismOption;
};

#endif // _APP_DELEGATE_H_

