/**
 * @file lv_tvg_canvas.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_tvg_canvas.h"
#include "lvgl/src/libs/svg/lv_tvg_canvas.h"

#if LV_USE_VECTOR_GRAPHIC

#include "../../misc/lv_assert.h"
#include "../../misc/lv_log.h"
#include "../../stdlib/lv_mem.h"
#include "../../draw/sw/lv_draw_sw.h"

#include "lv_svg_token.h"
#include "lv_svg_parser.h"

#include <math.h>

#include "../../core/lv_obj_class_private.h"
#include "../../widgets/canvas/lv_canvas_private.h"
#include "../../draw/sw/lv_draw_sw.h"
#include "../../draw/lv_draw_private.h"
#include "../../misc/lv_types.h"
#include "../../misc/lv_area_private.h"

#include "lv_svg_render.h"

#include <limits.h>

#include "lvgl/src/libs/svg/lv_tvg_canvas_private.h"

/**********************
 *  STATIC VARIABLES
 **********************/

/*********************
*      DEFINES
*********************/

#define MY_CLASS (&lv_tvg_canvas_class)


/**********************
*      TYPEDEFS
**********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_tvg_canvas_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void lv_tvg_canvas_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj);


/**********************
 *  GLOBAL VARIABLES
 **********************/

const lv_obj_class_t lv_tvg_canvas_class = {
    .constructor_cb = lv_tvg_canvas_constructor,
    .destructor_cb = lv_tvg_canvas_destructor,
    .event_cb = NULL,
    .width_def = LV_DPI_DEF,
    .height_def = LV_DPI_DEF,
    .instance_size = sizeof(lv_tvg_canvas_t),
    .base_class = &lv_obj_class,
    .name = "tvg_canvas",
};


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/




/**********************
 *   STATIC FUNCTIONS
 **********************/



static void lv_tvg_canvas_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_tvg_canvas_t* tvg_canvas = (lv_tvg_canvas_t*)obj;

    tvg_canvas->dsc =NULL;
    tvg_canvas->list =NULL;
    tvg_canvas->tvg_canvas = tvg_swcanvas_create();

    LV_TRACE_OBJ_CREATE("finished");
}


static void lv_tvg_canvas_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj)
{
    LV_UNUSED(class_p);
    lv_tvg_canvas_t* tvg_canvas = (lv_tvg_canvas_t*)obj;

    tvg_canvas_destroy(tvg_canvas->tvg_canvas);
    tvg_canvas->tvg_canvas = NULL;
}

lv_vector_dsc_t* lv_tvg_canvas_prepare_draw(lv_obj_t* obj, lv_event_t* e)
{
    lv_tvg_canvas_t* tvg_canvas = (lv_tvg_canvas_t*)obj;

    lv_layer_t* layer = lv_event_get_layer(e);

    lv_area_t series_clip_area;
    bool mask_ret = lv_area_intersect(&series_clip_area, &obj->coords, &layer->_clip_area);
    if(mask_ret == false) return NULL;

    // obj->coords
    // == 
    // lv_area_t my_obj_coords;
    // lv_obj_get_coords(my_obj, &my_obj_coords);
    const int32_t w = obj->coords.x2 - obj->coords.x1 + 1;
    const int32_t h = obj->coords.y2 - obj->coords.y1 + 1;

    // use layer buffer and color info to set ...

    const int32_t buf_w = layer->draw_buf->header.w;
    const int32_t buf_h = layer->draw_buf->header.h;
    const lv_color_format_t lvColorFormat = layer->draw_buf->header.cf;
    int32_t stride_bytes = layer->draw_buf->header.stride;
    //stride_bytes = lv_draw_buf_width_to_stride(stride_bytes, lvColorFormat);
    Tvg_Colorspace tvgColorFormat = lv_lvgl_to_tvg(lvColorFormat);

    uint32_t stride_pixels = stride_bytes / lv_color_format_get_size(lvColorFormat);

    tvg_swcanvas_set_target(tvg_canvas->tvg_canvas, (PIXEL_TYPE*)layer->draw_buf->data, stride_pixels, w, h, tvgColorFormat);


    const int32_t buf_w2 = layer->buf_area.x2 - layer->buf_area.x1;
    const int32_t buf_h2 = layer->buf_area.y2 - layer->buf_area.y1;
    const lv_color_format_t cf2 = layer->color_format;

    lv_area_t viewPort = {0, 0, w, h};
    lv_draw_vector_set_viewport_tvg_canvas(&viewPort, tvg_canvas->tvg_canvas);

    lv_vector_dsc_t* dsc = lv_vector_dsc_create(layer);

    return dsc;
}





#endif /*LV_USE_SVG*/
