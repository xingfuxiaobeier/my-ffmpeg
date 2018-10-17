/*
 * Copyright (c) 2015 Derek Buitenhuis
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/opt.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"
#include <string.h>
    
#define DEFAULT_LENGTH 300

typedef struct ReverseContext {
    int nb_frames;
    int nb_gop_frames;
    FFFrameQueue *frame_queue;
    AVFrame **gop_frames;
    unsigned int frames_size;
    unsigned int pts_size;
    unsigned int gop_frames_size;
    unsigned int gop_pts_size;
    // int64_t *pts;
    int64_t *gop_pts;
    int flush_idx;
    int cached_gop_ok;
    int64_t max_pts;
    int first_in_filter_tag;
} ReverseContext;

static av_cold int init(AVFilterContext *ctx)
{
    ReverseContext *s = ctx->priv;

    // s->pts = av_fast_realloc(NULL, &s->pts_size,
    //                          DEFAULT_LENGTH * sizeof(*(s->pts)));
    // if (!s->pts)
    //     return AVERROR(ENOMEM);

    s->gop_pts = av_fast_realloc(NULL, &s->gop_pts_size,
                             DEFAULT_LENGTH * sizeof(*(s->gop_pts)));
    if (!s->gop_pts)
        return AVERROR(ENOMEM);

    // s->frames = av_fast_realloc(NULL, &s->frames_size,
    //                             DEFAULT_LENGTH * sizeof(*(s->frames)));
    // if (!s->frames) {
    //     av_freep(&s->pts);
    //     return AVERROR(ENOMEM);
    // }
    // 
    if (s->frame_queue != NULL)
    {
        ff_framequeue_free(s->frame_queue);
        av_freep(s->frame_queue);
    }
    s->frame_queue = (FFFrameQueue *) av_malloc(sizeof(FFFrameQueue));
    memset(s->frame_queue, 0, sizeof(FFFrameQueue));
    ff_framequeue_init(s->frame_queue, NULL);

    s->gop_frames = av_fast_realloc(NULL, &s->gop_frames_size,
                                DEFAULT_LENGTH * sizeof(*(s->gop_frames)));
    if (!s->gop_frames) {
        av_freep(&s->gop_pts);
        return AVERROR(ENOMEM);
    }

    s->first_in_filter_tag = 0;

    return 0;
}

static av_cold void uninit(AVFilterContext *ctx)
{
    ReverseContext *s = ctx->priv;

    // av_freep(&s->pts);
    if (s->frame_queue != NULL)
    {
        ff_framequeue_free(s->frame_queue);
        av_freep(s->frame_queue);   
    }
    av_freep(&s->gop_pts);
    av_freep(&s->gop_frames);
    s->first_in_filter_tag = -1;

}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    ReverseContext *s    = ctx->priv;
    // void *ptr;

    // if (s->nb_frames + 1 > s->pts_size / sizeof(*(s->pts))) {
    //     ptr = av_fast_realloc(s->pts, &s->pts_size, s->pts_size * 2);
    //     if (!ptr)
    //         return AVERROR(ENOMEM);
    //     s->pts = ptr;
    // }

    // if (s->nb_frames + 1 > s->frames_size / sizeof(*(s->frames))) {
    //     ptr = av_fast_realloc(s->frames, &s->frames_size, s->frames_size * 2);
    //     if (!ptr)
    //         return AVERROR(ENOMEM);
    //     s->frames = ptr;
    // }

    //check framequeue is null
    if (s->frame_queue== NULL)
    {
        av_log(NULL, AV_LOG_INFO, "filter frame --- , frame_queue == NULL, return -1. \n");
        return -1;
    }

    av_log(NULL, AV_LOG_INFO, "filter frame --- , insert frame begin, frame queue size : %d, nb_frames : %d, pts : %lld, key_frame : %d \n", ff_framequeue_queued_frames(s->frame_queue), s->nb_frames, in->pts, in->key_frame);


    if (ff_framequeue_queued_frames(s->frame_queue) <= 0 && s->nb_frames <= 0)
    {
        ff_framequeue_add(s->frame_queue, in);
        s->nb_frames++;
        av_log(NULL, AV_LOG_INFO, "filter frame --- , frame queue size <= 0 and nb_frames <= 0, current : %d, %d \n", ff_framequeue_queued_frames(s->frame_queue), s->nb_frames);
    } else {
        av_log(NULL, AV_LOG_INFO, "filter frame --- , frame queue size(%d) > 0 and nb_frames(%d) > 0, check key_frame : %d, pts : %lld \n", ff_framequeue_queued_frames(s->frame_queue), s->nb_frames, in->key_frame, in->pts);        
        if (in->key_frame == 1)
        {
            av_log(NULL, AV_LOG_INFO, "filter frame --- , key frame(next gop) comes, check nb_gop_frames : %d \n", s->nb_gop_frames);
            //key frame, next gop
            int first_key_frame = 0;
            AVFrame *current_frame = NULL;
            if (s->nb_gop_frames <= 0)
            {
                current_frame = ff_framequeue_peek(s->frame_queue, 0);
                if (current_frame->key_frame == 1)
                {
                    ff_framequeue_take(s->frame_queue);
                }
                s->gop_frames[s->nb_gop_frames] = current_frame;
                s->gop_pts[s->nb_gop_frames] = current_frame->pts;
                s->nb_gop_frames++;
                s->nb_frames--;
                av_log(NULL, AV_LOG_INFO, "filter frame --- , key frame(next gop) comes, take frame from queue to gop_frames, nb_frames : %d, nb_gop_frames : %d, queue size : %d, pts : %lld \n", s->nb_frames, s->nb_gop_frames, ff_framequeue_queued_frames(s->frame_queue), current_frame->pts);

                //
                AVFrame *max_pts_frame = NULL;
                do {
                    current_frame = ff_framequeue_peek(s->frame_queue, 0);   
                    if (current_frame != NULL)
                    {
                        if (current_frame->key_frame != 1)
                        {
                            ff_framequeue_take(s->frame_queue);
                            s->gop_frames[s->nb_gop_frames] = current_frame;
                            s->gop_pts[s->nb_gop_frames] = current_frame->pts;
                            s->nb_gop_frames++;
                            s->nb_frames--;
                            max_pts_frame = current_frame;    
                            av_log(NULL, AV_LOG_INFO, "filter frame --- , not key frame(next gop) comes, take frame from queue to gop_frames, nb_frames : %d, nb_gop_frames : %d, queue size : %d, pts : %lld \n", s->nb_frames, s->nb_gop_frames, ff_framequeue_queued_frames(s->frame_queue), current_frame->pts);
                        } else {
                            av_log(NULL, AV_LOG_INFO, "filter frame --- , not key frame(next gop) comes, reach next key_frame, nb_frames : %d, nb_gop_frames : %d, queue size : %d, pts : %lld \n", s->nb_frames, s->nb_gop_frames, ff_framequeue_queued_frames(s->frame_queue), current_frame->pts);
                        } 
                    

                    }
                
                } while (ff_framequeue_queued_frames(s->frame_queue) > 0 
                    && current_frame != NULL
                    && current_frame->key_frame != 1 
                    && s->nb_gop_frames > 0 
                    && s->nb_frames >= 0);
                s->cached_gop_ok = 1;  
                av_log(NULL, AV_LOG_INFO, "filter frame --- , cached_gop_ok == 1 \n");

                if (s->first_in_filter_tag == 0)
                {
                    if (max_pts_frame != NULL)
                    {
                        s->max_pts = max_pts_frame->pts;   
                        av_log(NULL, AV_LOG_INFO, "filter frame --- , print max pts : %lld", s->max_pts);
                    }
                    s->first_in_filter_tag = -1;
                }

            }

            ff_framequeue_add(s->frame_queue, in);
            s->nb_frames++;
            
        } else {
            //no key frame
            ff_framequeue_add(s->frame_queue, in);
            s->nb_frames++;
            av_log(NULL, AV_LOG_INFO, "filter frame --- , not key frame comes, add it to queue, nb_frames : %d, queue size : %d, pts : %lld \n", s->nb_frames, ff_framequeue_queued_frames(s->frame_queue), in->pts);

        }
    }
    
    // s->frames[s->nb_frames] = in;
    // s->pts[s->nb_frames]    = in->pts;
    // s->nb_frames++;

    return 0;
}

#if CONFIG_GOPREVERSE_FILTER

static int request_frame(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    ReverseContext *s = ctx->priv;
    int ret;
    ret = ff_request_frame(ctx->inputs[0]);

    av_log(NULL, AV_LOG_INFO, "request frame --- , current cached_gop_ok : %d, nb_gop_frames : %d \n", s->cached_gop_ok, s->nb_gop_frames);
    if (s->cached_gop_ok == 1 && s->nb_gop_frames > 0) {
        AVFrame *out = s->gop_frames[s->nb_gop_frames - 1];
        // out->pts     = s->gop_pts[s->flush_idx++];
        out->pts     = s->max_pts - s->gop_pts[s->nb_gop_frames - 1];
        ret          = ff_filter_frame(outlink, out);
        s->nb_gop_frames--;
    }
    if (s->nb_gop_frames == 0)
    {
        s->cached_gop_ok = 0;
        s->flush_idx = 0;
        av_log(NULL, AV_LOG_INFO, "request frame --- , nb_gop_frames == 0 \n");
    }

    return ret;
}

static const AVFilterPad reverse_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad reverse_outputs[] = {
    {
        .name          = "default",
        .type          = AVMEDIA_TYPE_VIDEO,
        .request_frame = request_frame,
    },
    { NULL }
};

AVFilter ff_vf_gopreverse = {
    .name        = "gopreverse",
    .description = NULL_IF_CONFIG_SMALL("Reverse a gop."),
    .priv_size   = sizeof(ReverseContext),
    .init        = init,
    .uninit      = uninit,
    .inputs      = reverse_inputs,
    .outputs     = reverse_outputs,
};

#endif /* CONFIG_REVERSE_FILTER */
