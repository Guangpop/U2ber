/*
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at http://live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppView.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppDefine.hpp"
#include "LAppPal.hpp"
#include "LAppModel.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

//////////////////////////////// FFMPEG //////////////////////////////////

GLubyte *pixels = NULL;
static AVCodecContext *c = NULL;
static AVFrame *frame;
static AVPacket pkt;
static FILE *file;
static struct SwsContext *sws_context = NULL;
static uint8_t *rgb = NULL;
static unsigned int nframes = 0;


//////////////////////////////// FFMPEG //////////////////////////////////

using namespace Csm;
using namespace LAppDefine;

USING_NS_CC;

LAppView::LAppView(): DrawNode()
                    , _debugRects(NULL)
{}

void LAppView::onEnter()
{
    DrawNode::onEnter();

    // タッチ関係のイベント管理
    touchMgr = new TouchManager();

    // デバイス座標からスクリーン座標に変換するための
    deviceToScreen = new CubismMatrix44();

    // 画面の表示の拡大縮小や移動の変換を行う行列
    viewMatrix = new CubismViewMatrix();

    Size size = Director::getInstance()->getWinSize();

    float width = size.width;
    float height = size.height;
    float ratio = (float)height / width;
    float left = ViewLogicalLeft;
    float right = ViewLogicalRight;
    float bottom = -ratio;
    float top = ratio;

    viewMatrix->SetScreenRect(left, right, bottom, top); // デバイスに対応する画面の範囲。 Xの左端, Xの右端, Yの下端, Yの上端

    float screenW = abs(left - right);
    deviceToScreen->ScaleRelative(screenW / width, -screenW / width);
    deviceToScreen->TranslateRelative(-width / 2.0f, -height / 2.0f);

    // 表示範囲の設定
    viewMatrix->SetMaxScale(ViewMaxScale); // 限界拡大率
    viewMatrix->SetMinScale(ViewMinScale); // 限界縮小率

    // 表示できる最大範囲
    viewMatrix->SetMaxScreenRect(
        ViewLogicalMaxLeft,
        ViewLogicalMaxRight,
        ViewLogicalMaxBottom,
        ViewLogicalMaxTop
    );

    // イベントリスナー作成
    EventListenerTouchAllAtOnce* listener = EventListenerTouchAllAtOnce::create();

    // タッチメソッド設定
    listener->onTouchesBegan = CC_CALLBACK_2(LAppView::onTouchesBegan, this);
    listener->onTouchesMoved = CC_CALLBACK_2(LAppView::onTouchesMoved, this);
    listener->onTouchesEnded = CC_CALLBACK_2(LAppView::onTouchesEnded, this);

    // 優先度100でディスパッチャーに登録
    this->getEventDispatcher()->addEventListenerWithFixedPriority(listener, 100);
    
    ffmpeg_encoder_start("~/Downloads/tmp.mpg", AV_CODEC_ID_MPEG1VIDEO, 25, width, height);
}

void LAppView::onExit()
{
    DrawNode::onExit();

    ffmpeg_encoder_finish();
    free(rgb);
    
    delete touchMgr;
    delete deviceToScreen;
    delete viewMatrix;
}


//////////////////////////////// FFMPEG //////////////////////////////////

void LAppView::ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb) {
    const int in_linesize[1] = { 4 * c->width };
    sws_context = sws_getCachedContext(sws_context,
                                       c->width, c->height, AV_PIX_FMT_RGB32,
                                       c->width, c->height, AV_PIX_FMT_YUV420P,
                                       0, NULL, NULL, NULL);
    sws_scale(sws_context, (const uint8_t * const *)&rgb, in_linesize, 0,
              c->height, frame->data, frame->linesize);
}

void LAppView::ffmpeg_encoder_start(const char *filename, int codec_id, int fps, int width, int height) {
    AVCodec *codec;
    int ret;
    avcodec_register_all();
    codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    c->bit_rate = 400000;
    c->width = width;
    c->height = height;
    c->time_base.num = 1;
    c->time_base.den = fps;
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }
}

void LAppView::ffmpeg_encoder_finish(void) {
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    int got_output, ret;
    do {
        fflush(stdout);
        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, file);
            av_packet_unref(&pkt);
        }
    } while (got_output);
    fwrite(endcode, 1, sizeof(endcode), file);
    fclose(file);
    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
}

void LAppView::ffmpeg_encoder_encode_frame(uint8_t *rgb) {
    int ret, got_output;
    ffmpeg_encoder_set_frame_yuv_from_rgb(rgb);
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
    if (ret < 0) {
        fprintf(stderr, "Error encoding frame\n");
        exit(1);
    }
    if (got_output) {
        fwrite(pkt.data, 1, pkt.size, file);
        av_packet_unref(&pkt);
    }
}

void LAppView::ffmpeg_encoder_glread_rgb(uint8_t **rgb, GLubyte **pixels, unsigned int width, unsigned int height) {
    size_t i, j, k, cur_gl, cur_rgb, nvals;
    const size_t format_nchannels = 4;
    nvals = format_nchannels * width * height;
    *pixels = (GLubyte *)realloc(*pixels, nvals * sizeof(GLubyte));
    *rgb = (uint8_t *)realloc(*rgb, nvals * sizeof(uint8_t));
    /* Get RGBA to align to 32 bits instead of just 24 for RGB. May be faster for FFmpeg. */
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *pixels);
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            cur_gl  = format_nchannels * (width * (height - i - 1) + j);
            cur_rgb = format_nchannels * (width * i + j);
            for (k = 0; k < format_nchannels; k++)
                (*rgb)[cur_rgb + k] = (*pixels)[cur_gl + k];
        }
    }
}

void LAppView::record() {
    auto director = Director::getInstance();
    
}

//////////////////////////////// FFMPEG //////////////////////////////////


void LAppView::draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
    DrawNode::draw(renderer, transform, flags);

    _customCommand.init(_globalZOrder);
    _customCommand.func = CC_CALLBACK_0(LAppView::onDraw, this, transform, flags);
    renderer->addCommand(&_customCommand);
}

void LAppView::onDraw(const cocos2d::Mat4& transform, uint32_t flags)
{
    Director::getInstance()->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    Director::getInstance()->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, transform);

    LAppLive2DManager* Live2DMgr = LAppLive2DManager::GetInstance();
    Live2DMgr->OnUpdate();

    if (_debugRects)
    {
        drawDebugRects(Live2DMgr);
    }

    Director::getInstance()->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    
    auto size = Director::getInstance()->getWinSize();
    auto width = size.width;
    auto height = size.height;
    
    frame->pts = nframes;
    ffmpeg_encoder_glread_rgb(&rgb, &pixels, width, height);
    ffmpeg_encoder_encode_frame(rgb);
    
    nframes++;
}

void LAppView::onTouchesBegan(const std::vector<Touch*>& touches, Event* event)
{
    // タッチ開始
    size_t touchNum = touches.size();

    if (touchNum == 1)
    {
        Point pt = touches[0]->getLocationInView();
        if (DebugTouchLogEnable)LAppPal::PrintLog("[APP]touchesBegan x:%.0f y:%.0f", pt.x, pt.y);
        touchMgr->touchesBegan(pt.x, pt.y);
    }
}

void LAppView::onTouchesMoved(const std::vector<Touch*>& touches, Event* event)
{
    // タッチ中
    size_t touchNum = touches.size();

    float screenX = this->transformScreenX(touchMgr->getX());
    float screenY = this->transformScreenY(touchMgr->getY());
    float viewX = this->transformViewX(touchMgr->getX());
    float viewY = this->transformViewY(touchMgr->getY());

    if (touchNum == 1)
    {
        Point pt = touches[0]->getLocationInView();

        if (DebugTouchLogEnable)
            LAppPal::PrintLog("[APP]touchesMoved device{x:%.0f y:%.0f} screen{x:%.2f y:%.2f} view{x:%.2f y:%.2f}", pt.x, pt.y, screenX, screenY, viewX, viewY);

        touchMgr->touchesMoved(pt.x, pt.y);
    }
    LAppLive2DManager* live2DMgr = LAppLive2DManager::GetInstance();
    live2DMgr->OnDrag(viewX, viewY);
}

void LAppView::onTouchesEnded(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
{
    // タッチ終了
    LAppLive2DManager* live2DMgr = LAppLive2DManager::GetInstance();
    live2DMgr->OnDrag(0, 0);

    if (touches.size() == 1)
    {
        // シングルタップ
        float x = deviceToScreen->TransformX(touchMgr->getX()); // 論理座標変換した座標を取得。
        float y = deviceToScreen->TransformY(touchMgr->getY()); // 論理座標変換した座標を取得。
        if (DebugTouchLogEnable) LAppPal::PrintLog("[APP]touchesEnded x:%.2f y:%.2f", x, y);
        live2DMgr->OnTap(x, y);
    }
}

void LAppView::updateViewMatrix(float dx, float dy, float cx, float cy, float scale)
{
    LAppLive2DManager* live2DMgr = LAppLive2DManager::GetInstance();

    // 拡大縮小
    viewMatrix->AdjustScale(cx, cy, scale);

    // 移動
    viewMatrix->AdjustTranslate(dx, dy);

    live2DMgr->SetViewMatrix(viewMatrix);
}

float LAppView::transformViewX(float deviceX)
{
    float screenX = deviceToScreen->TransformX(deviceX); // 論理座標変換した座標を取得。
    return viewMatrix->InvertTransformX(screenX); // 拡大、縮小、移動後の値。
}

float LAppView::transformViewY(float deviceY)
{
    float screenY = deviceToScreen->TransformY(deviceY); // 論理座標変換した座標を取得。
    return viewMatrix->InvertTransformY(screenY); // 拡大、縮小、移動後の値。
}

float LAppView::transformScreenX(float deviceX)
{
    return deviceToScreen->TransformX(deviceX);
}

float LAppView::transformScreenY(float deviceY)
{
    return deviceToScreen->TransformY(deviceY);
}

void LAppView::setDebugRectsNode(DrawNode* debugRects)
{
    _debugRects = debugRects;
}

void LAppView::drawDebugRects(LAppLive2DManager* manager) const
{
    const Color4F hitAreaColor = Color4F(1.0f, 0, 0, 0.2f);
    const Color4F userDataAreaColor = Color4F(0, 0, 1.0f, 0.2f);

    CubismMatrix44 projection;
    const Size window = Director::getInstance()->getWinSize();
    const CubismVector2 windowSize = CubismVector2(window.width, window.height);
    projection.Scale(1, window.width / window.height);

    if (viewMatrix != NULL)
    {
        projection.MultiplyByMatrix(viewMatrix);
    }

    for (csmUint32 i = 0; i < manager->GetModelCount(); ++i)
    {
        _debugRects->clear();
        LAppModel* model = manager->GetModel(i);
        const Csm::csmVector<Csm::csmRectF>& userDataAreas = model->GetUserDataAreas(projection, windowSize);
        for (csmUint32 j = 0; j < userDataAreas.GetSize(); ++j)
        {
            _debugRects->drawSolidRect(Vec2(userDataAreas[j].X, userDataAreas[j].Y)
                                       , Vec2(userDataAreas[j].GetRight(), userDataAreas[j].GetBottom())
                                       , userDataAreaColor);
        }

        const Csm::csmVector<Csm::csmRectF>& hitAreas = model->GetHitAreas(projection, windowSize);
        for (csmUint32 j = 0; j < hitAreas.GetSize(); ++j)
        {
            _debugRects->drawSolidRect(Vec2(hitAreas[j].X, hitAreas[j].Y)
                                       , Vec2(hitAreas[j].GetRight(), hitAreas[j].GetBottom())
                                       , hitAreaColor);
        }
    }
}

LAppView* LAppView::createDrawNode()
{
    LAppView* ret = new(std::nothrow) LAppView();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
