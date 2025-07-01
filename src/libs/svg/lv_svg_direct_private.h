/**
 * @file lv_svg_direct_private.h
 *
 */
#ifndef LV_SVG_DIRECT_PRIVATE_H
#define LV_SVG_DIRECT_PRIVATE_H

/*********************
 *      INCLUDES
 *********************/
#include "lv_svg_direct.h"
#if LV_USE_SVG

#include "../../misc/lv_assert.h"
#include "../../misc/lv_log.h"
#include "../../stdlib/lv_mem.h"
#include "../../draw/sw/lv_draw_sw.h"

#include "../../core/lv_obj_class_private.h"
#include "lvgl/src/widgets/canvas/lv_canvas_private.h"
#include "../../misc/lv_types.h"
#include "../../misc/lv_area_private.h"


#include <limits.h>

/**********************
 *  STATIC VARIABLES
 **********************/

#define SVG_DIRECT_LOW_MEM 0

struct _lv_svg_direct_t {
    // inheritance parent
    lv_obj_t obj;

#if SVG_DIRECT_LOW_MEM
    const char* svg_data;
    uint32_t svg_data_len;
#else
    lv_svg_node_t* doc;
#endif
    /** Size (Width x Height) of the image (Handled by the library)*/
    lv_point_t svg_size_data;
    lv_point_t svg_size;

    lv_point_t widget_size;

    lv_vector_dsc_t* dsc;
    lv_svg_render_obj_t * list;

    Tvg_Canvas* tvg_canvas;

    lv_anim_t* anim;
    float animTime_ms;
    int32_t last_rendered_time;
};



#endif /*LV_USE_SVG*/

#endif /*LV_SVG_DIRECT_PRIVATE_H*/
