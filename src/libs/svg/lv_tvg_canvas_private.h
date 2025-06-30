/**
 * @file lv_tvg_canvas_private.h
 *
 */
#ifndef LV_TVG_CANVAS_PRIVATE_H
#define LV_TVG_CANVAS_PRIVATE_H

/*********************
 *      INCLUDES
 *********************/
#include "lv_tvg_canvas.h"
#if LV_USE_VECTOR_GRAPHIC

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


struct _lv_tvg_canvas_t {
    // inheritance parent
    lv_obj_t base;

    lv_vector_dsc_t* dsc;
    lv_svg_render_obj_t* list;

    Tvg_Canvas* tvg_canvas;
};



#endif /*LV_USE_VECTOR_GRAPHIC*/

#endif /*LV_TVG_CANVAS_PRIVATE_H*/
