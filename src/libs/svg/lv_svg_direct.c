/**
 * @file lv_svg_direct.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_svg_direct.h"
#if LV_USE_SVG

#include "../../misc/lv_assert.h"
#include "../../misc/lv_log.h"
#include "../../stdlib/lv_mem.h"
#include "../../draw/sw/lv_draw_sw.h"

#include "lv_svg.h"
#include "lv_svg_token.h"
#include "lv_svg_parser.h"

#include <math.h>

#include "../../core/lv_obj_class_private.h"
#include "../../widgets/canvas/lv_canvas_private.h"
#include "../../draw/sw/lv_draw_sw.h"
#include "../../draw/lv_draw_private.h"
#include "../../draw/lv_draw_vector_private.h"
#include "../../misc/lv_types.h"
#include "../../misc/lv_area_private.h"

#include "lv_svg_render.h"

#if LV_USE_SVG_ANIMATION
#include "lv_svg_anim.h"
#endif

#include <limits.h>

#include "lv_svg_direct_private.h"

#include "../../libs/thorvg/src/bindings/capi/thorvg_capi.h"

/**********************
 *  STATIC VARIABLES
 **********************/

/*********************
*      DEFINES
*********************/

#define MY_CLASS (&lv_svg_direct_class)


/**********************
*      TYPEDEFS
**********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_svg_direct_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_svg_direct_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_svg_direct_draw(lv_obj_t* obj, lv_event_t* e);
static void lv_svg_direct_event(const lv_obj_class_t * class_p, lv_event_t * e);


/**********************
 *  GLOBAL VARIABLES
 **********************/

const lv_obj_class_t lv_svg_direct_class = {
    .constructor_cb = lv_svg_direct_constructor,
    .destructor_cb = lv_svg_direct_destructor,
    .event_cb = lv_svg_direct_event,
    .width_def = LV_DPI_DEF,
    .height_def = LV_DPI_DEF,
    .instance_size = sizeof(lv_svg_direct_t),
    .base_class = &lv_obj_class,
    .name = "svg_direct",
};


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


lv_obj_t * lv_svg_direct_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}



/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_svg_direct_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_result_t res;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_svg_direct_t * svg = (lv_svg_direct_t *)obj;

    if(code == LV_EVENT_DRAW_MAIN) {
        //LOG_INFO("LV_EVENT_DRAW_MAIN");
        lv_svg_direct_draw(obj, e);
    }
    else {
        if (code == LV_EVENT_SIZE_CHANGED) {
            svg->widget_size.x = lv_obj_get_width(obj);
            svg->widget_size.y = lv_obj_get_height(obj);

#if SVG_DIRECT_LOW_MEM
#else
            if (svg->doc != NULL) {
			    lv_svg_node_fit_size(svg->doc, svg->widget_size, true);
            }
#endif


        }

        /*Call the ancestor's event handler*/
        res = lv_obj_event_base(MY_CLASS, e);
        if (res != LV_RESULT_OK) {
            return;
        }

        if (code == LV_EVENT_GET_SELF_SIZE) {
            lv_svg_direct_t* self = (lv_svg_direct_t *)obj;
            lv_point_t * self_size = lv_event_get_param(e);

            self_size->x = self->widget_size.x;
            self_size->y = self->widget_size.y;
        }
    }
}


static void lv_svg_direct_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_svg_direct_t * svg = (lv_svg_direct_t *)obj;

#if SVG_DIRECT_LOW_MEM
    svg->svg_data =NULL;
    svg->svg_data_len =0;
#else
    svg->doc =NULL;
#endif
    svg->dsc =NULL;
    svg->list =NULL;
    svg->tvg_canvas = tvg_swcanvas_create();
    svg->svg_size.x = 0;
    svg->svg_size.y = 0;
    svg->svg_size_data.x = 0;
    svg->svg_size_data.y = 0;
    svg->widget_size.x = 0;
    svg->widget_size.y = 0;



    //lv_draw_buf_t * draw_buf = lv_canvas_get_draw_buf(obj);
    //if(draw_buf) {
    //   lv_color_format_t cf = draw_buf->header.cf;
    //   LV_LOG("cf %d 0x%02X", cf, cf);		 
	//}

    //lv_display_t * disp = lv_refr_get_disp_refreshing();
    //if(layer != disp->layer_head) {
//
    //}



    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_svg_direct_anim(lv_obj_t* obj, int32_t v)
{
    lv_svg_direct_t* svg = (lv_svg_direct_t*)obj;

    if (svg->anim != NULL) {
        /*Do not render not visible animations.*/
        if(lv_obj_is_visible(obj)) {
            svg->animTime_ms += LV_DEF_REFR_PERIOD;
            lv_obj_invalidate(obj);
        }
        else {
            /*Artificially keep the animation on the last rendered frame's time
             *To avoid a jump when the widget becomes visible*/
            svg->anim->act_time = svg->last_rendered_time;
        }
    }
}

static void lv_svg_direct_anim_cb(void * var, int32_t v)
{
    lv_obj_t* obj = (lv_obj_t*)var;
    lv_svg_direct_anim(obj, v);
}

static void lv_svg_direct_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj)
{
    LV_UNUSED(class_p);
    lv_svg_direct_t* svg = (lv_svg_direct_t*)obj;

    if (svg->anim != NULL) {
        lv_anim_delete(obj, lv_svg_direct_anim_cb);
        svg->anim = NULL;
    }

    tvg_canvas_destroy(svg->tvg_canvas);

#if SVG_DIRECT_LOW_MEM
    svg->svg_data =NULL;
    svg->svg_data_len =0;
#else
    lv_svg_node_delete(svg->doc);
    svg->doc =NULL;
#endif
}

static lv_point_t lv_svg_direct_set_size_(lv_obj_t* obj, lv_svg_node_t * doc, const lv_point_t widget_size, const bool only_smaller)
{
    lv_svg_direct_t * svg = (lv_svg_direct_t *)obj;
    svg->widget_size = widget_size;
    lv_point_t size = (lv_point_t){0, 0};

    if (doc != NULL) {
        lv_point_t svg_size = (lv_point_t){0, 0};
        lv_svg_get_size(doc, &svg_size);
        svg->svg_size.x = svg_size.x;
        svg->svg_size.y = svg_size.y;

        // fit and keep aspect ration
        svg->widget_size = lv_svg_node_fit_size(doc, widget_size, only_smaller);

        lv_svg_get_size(doc, &size);
        svg->svg_size.x = size.x;
        svg->svg_size.y = size.y;
        lv_obj_set_content_width(obj, size.x);
        lv_obj_set_content_height(obj, size.y);
    }

    return size;
}

lv_point_t lv_svg_direct_set_size(lv_obj_t* obj, const lv_point_t widget_size, const bool only_smaller)
{
    lv_svg_direct_t * svg = (lv_svg_direct_t *)obj;
    lv_point_t svg_size = (lv_point_t){0, 0};

    lv_svg_node_t * doc;
#if SVG_DIRECT_LOW_MEM
    doc = lv_svg_load_data(svg->svg_data, svg->svg_data_len);
#else
    doc = svg->doc;
#endif
    if (doc != NULL) {

        svg_size = lv_svg_direct_set_size_(obj, doc, widget_size, only_smaller);

#if SVG_DIRECT_LOW_MEM
        lv_svg_node_delete(doc);
#endif
    }
    
    return svg_size;
}

void lv_svg_direct_set_src_data(lv_obj_t* obj, const char * svg_data, uint32_t svg_data_len, const lv_point_t widget_size, const bool only_smaller)
{
    lv_svg_direct_t * svg = (lv_svg_direct_t *)obj;
    lv_point_t svg_size = (lv_point_t){0, 0};

    lv_svg_node_t * doc;
#if SVG_DIRECT_LOW_MEM
    svg->svg_data = svg_data;
    svg->svg_data_len = svg_data_len;
    doc = lv_svg_load_data(svg->svg_data, svg->svg_data_len);
#else
    lv_svg_node_delete(svg->doc);
    svg->doc = lv_svg_load_data(svg_data, svg_data_len);
    doc = svg->doc;
#endif
    svg->svg_size_data.x = 0;
    svg->svg_size_data.y = 0;

    if (doc != NULL) {
        lv_svg_get_size(doc, &svg_size);
        svg->svg_size_data.x = svg_size.x;
        svg->svg_size_data.y = svg_size.y;

        svg_size = lv_svg_direct_set_size_(obj, doc, widget_size, only_smaller);

        if (svg->anim != NULL) {
            lv_anim_delete(obj, lv_svg_direct_anim_cb);
            svg->anim = NULL;
        }

        if ((svg->widget_size.x == 0) || (svg->widget_size.y == 0)) {
            svg->widget_size.x = svg->svg_size.x;
            svg->widget_size.y = svg->svg_size.y;
        }

        if (lv_svg_node_has_animation(doc)) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_exec_cb(&a, lv_svg_direct_anim_cb);
            lv_anim_set_var(&a, obj);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            svg->anim = lv_anim_start(&a);
        }

#if SVG_DIRECT_LOW_MEM
        lv_svg_node_delete(doc);
#endif
    }

    /*Force updating when the buffer changes*/
    lv_obj_invalidate(obj);

}


static void lv_svg_direct_draw(lv_obj_t* obj, lv_event_t* e)
{
    lv_svg_direct_t* svg = (lv_svg_direct_t*)obj;

    lv_layer_t * layer = lv_event_get_layer(e);

    lv_area_t series_clip_area;
    bool mask_ret = lv_area_intersect(&series_clip_area, &obj->coords, &layer->_clip_area);
    if(mask_ret == false) {
        LV_LOG_WARN("lv_svg_direct_draw not");
        return;
    }

    //LV_LOG_WARN("lv_svg_direct_draw: (%d, %d) [%d x %d]", series_clip_area.x1, series_clip_area.y1, series_clip_area.x2 - series_clip_area.x1, series_clip_area.y2 - series_clip_area.y1);
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

    tvg_swcanvas_set_target(svg->tvg_canvas, (PIXEL_TYPE*)layer->draw_buf->data, stride_pixels, w, h, tvgColorFormat);


    const int32_t buf_w2 = layer->buf_area.x2 - layer->buf_area.x1;
    const int32_t buf_h2 = layer->buf_area.y2 - layer->buf_area.y1;
    
    const lv_color_format_t cf2 = layer->color_format;



#if 0    

//    lv_area_t viewPort = {obj->coords.x1, obj->coords.y1, obj->coords.x2, obj->coords.y2};
    lv_area_t viewPort = {0, 0, w, h};
    lv_draw_vector_set_viewport_tvg_canvas(&viewPort, svg->tvg_canvas);
#endif


    lv_vector_dsc_t* dsc = lv_vector_dsc_create(layer);

    //lv_vector_dsc_t:
    //    lv_layer_t* layer;
    //    lv_vector_draw_dsc_t current_dsc;
    //    lv_draw_vector_task_dsc_t tasks; // as also required for lv_draw_add_task
    //
    //    lv_draw_vector_task_dsc_t:
    //        lv_draw_dsc_base_t base;
    //        lv_ll_t* task_list; /*draw task list.*/
        

#if SVG_DIRECT_LOW_MEM
    lv_svg_node_t* doc = lv_svg_load_data(svg->svg_data, svg->svg_data_len);

    lv_svg_node_fit_size(doc, svg->object_size, true);
#else
    lv_svg_node_t* doc = svg->doc;
#endif

    if ((doc != NULL) && (svg->anim != NULL)) {
        lv_svg_node_animate_step(doc, svg->animTime_ms);
        svg->last_rendered_time = svg->anim->act_time;
    }

    lv_svg_render_obj_t* list = lv_svg_render_create(doc);

    lv_draw_svg_render(dsc, list);


    lv_draw_task_t * t = lv_draw_add_task(layer, &(obj->coords), LV_DRAW_TASK_TYPE_VECTOR);
    t->type = LV_DRAW_TASK_TYPE_VECTOR;
    t->draw_dsc = lv_malloc(sizeof(lv_draw_vector_task_dsc_t));
    
    // copy contents/move list:
    lv_memcpy(t->draw_dsc, &(dsc->tasks), sizeof(lv_draw_vector_task_dsc_t));
    lv_draw_finalize_task_creation(layer, t);
    dsc->tasks.task_list = NULL;

#if SVG_DIRECT_LOW_MEM
    lv_svg_node_delete(doc);
#endif
    lv_svg_render_delete(list);

    lv_vector_dsc_delete(dsc);

}




#endif /*LV_USE_SVG*/
