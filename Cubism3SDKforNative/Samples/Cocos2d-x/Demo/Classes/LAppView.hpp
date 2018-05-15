/*
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at http://live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Math/CubismMatrix44.hpp>
#include <Math/CubismViewMatrix.hpp>
#include "TouchManager.h"
#include "2d/CCSprite.h"
#include "cocos2d.h"
#include <vector>
#include "LAppLive2DManager.hpp"

class LAppView : public cocos2d::DrawNode
{
public:
    LAppView();
    virtual ~LAppView() {}

    virtual void onEnter();
    virtual void onExit();

    virtual void draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags);
    void onDraw(const cocos2d::Mat4& transform, uint32_t flags);

    void onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);
    void onTouchesMoved(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);
    void onTouchesEnded(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);

    void updateViewMatrix(float dx, float dy, float cx, float cy, float scale);
    float transformViewX(float deviceX);
    float transformViewY(float deviceY);
    float transformScreenX(float deviceX);
    float transformScreenY(float deviceY);

    void setDebugRectsNode(DrawNode* debugRects);
    void drawDebugRects(LAppLive2DManager* manager) const;

    static LAppView* createDrawNode();
    
    virtual void ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb);
    
    virtual void ffmpeg_encoder_start(const char *filename, int codec_id, int fps, int width, int height);
    
    virtual void ffmpeg_encoder_finish(void);
    
    virtual void ffmpeg_encoder_encode_frame(uint8_t *rgb);
    
    virtual void ffmpeg_encoder_glread_rgb(uint8_t **rgb, GLubyte **pixels, unsigned int width, unsigned int height);
    
    
private:
    TouchManager* touchMgr;
    Csm::CubismMatrix44* deviceToScreen;
    Csm::CubismViewMatrix* viewMatrix;
    DrawNode* _debugRects;

protected:
    cocos2d::CustomCommand _customCommand;
};
