/**
 * @file lv_svg.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_svg.h"
#if LV_USE_SVG

#include "../../misc/lv_assert.h"
#include "../../misc/lv_log.h"
#include "../../stdlib/lv_mem.h"
#include "../../draw/sw/lv_draw_sw.h"

#include "lv_svg_token.h"
#include "lv_svg_parser.h"

#include <math.h>

/*********************
*      DEFINES
*********************/

/**********************
*      TYPEDEFS
**********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_svg_node_attr_set(lv_svg_node_t * node, lv_svg_attr_type_t id, float value);
static void lv_svg_node_set_size(lv_svg_node_t * node, const lv_point_t size);


static void lv_svg_node_constructor(const lv_tree_class_t * class_p, lv_tree_node_t * node)
{
    LV_UNUSED(class_p);
    lv_svg_node_t * t = (lv_svg_node_t *)node;
    t->xml_id = NULL;
    t->type = LV_SVG_TAG_INVALID;
    lv_array_init(&t->attrs, 4, sizeof(lv_svg_attr_t));
    t->render_obj = NULL;
}

static void lv_svg_node_destructor(const lv_tree_class_t * class_p, lv_tree_node_t * node)
{
    LV_UNUSED(class_p);
    lv_svg_node_t * t = (lv_svg_node_t *)node;
    if(t->xml_id) {
        lv_free(t->xml_id);
    }
    for(uint32_t i = 0; i < lv_array_size(&t->attrs); i++) {
        lv_svg_attr_t * attr = lv_array_at(&t->attrs, i);
        if(attr->val_type == LV_SVG_ATTR_VALUE_PTR) {
            lv_free(attr->value.val);
        }
    }
    lv_array_deinit(&t->attrs);
}

static bool svg_token_process_cb(_lv_svg_token_t * token, void * data)
{
    _lv_svg_parser_t * parser = (_lv_svg_parser_t *)data;
    return _lv_svg_parser_token(parser, token);
}



/**********************
 *  STATIC VARIABLES
 **********************/
const lv_tree_class_t lv_svg_node_class = {
    .base_class = &lv_tree_node_class,
    .instance_size = sizeof(lv_svg_node_t),
    .constructor_cb = lv_svg_node_constructor,
    .destructor_cb = lv_svg_node_destructor,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_svg_node_t * lv_svg_load_data(const char * svg_data, uint32_t data_len)
{
    LV_ASSERT_NULL(svg_data);
    LV_ASSERT(data_len > 0);

    _lv_svg_parser_t parser;
    _lv_svg_parser_init(&parser);

    if(_lv_svg_tokenizer(svg_data, data_len, svg_token_process_cb, &parser)) {
        if(_lv_svg_parser_is_finish(&parser)) {
            lv_svg_node_t * doc = parser.doc_root;
            parser.doc_root = NULL;
            _lv_svg_parser_deinit(&parser);
#if 0 //LV_USE_SVG_DEBUG
            _lv_svg_dump_tree(doc, 0);
#endif
            return doc;
        }
        else {
            _lv_svg_parser_deinit(&parser);
            LV_LOG_ERROR("svg document parser raise errors!");
            return NULL;
        }
    }
    else {
        _lv_svg_parser_deinit(&parser);
        LV_LOG_ERROR("svg document tokenizer raise errors!");
        return NULL;
    }
}

lv_svg_node_t * lv_svg_node_create(lv_svg_node_t * parent)
{
    lv_tree_node_t * node = lv_tree_node_create(&lv_svg_node_class, (lv_tree_node_t *)parent);
    return (lv_svg_node_t *)node;
}

void lv_svg_node_delete(lv_svg_node_t * node)
{
    lv_tree_node_delete((lv_tree_node_t *)node);
}


lv_svg_attr_t * lv_svg_get_attr(lv_svg_node_t * node, lv_svg_attr_type_t id)
{
    lv_svg_attr_t * attr = NULL;

    uint32_t len = lv_array_size(&node->attrs);
    for(uint32_t i = 0; i < len; i++) {
        lv_svg_attr_t* attr_ = lv_array_at(&node->attrs, i);
        if (attr_->id == id)
        {
            attr= attr_;
            break;
        }
    }

    return attr;
}

void lv_svg_get_size(lv_svg_node_t * node, lv_point_t *size)
{
    int32_t w_ = -1;
    int32_t h_ = -1;
    lv_svg_attr_t* width_attr = lv_svg_get_attr(node, LV_SVG_ATTR_WIDTH);
    lv_svg_attr_t* height_attr = lv_svg_get_attr(node, LV_SVG_ATTR_HEIGHT);
    lv_svg_attr_t* viewBox_attr = lv_svg_get_attr(node, LV_SVG_ATTR_VIEWBOX);

    if (viewBox_attr != NULL) {
        if (viewBox_attr->class_type != LV_SVG_ATTR_VALUE_NONE) {
            if (viewBox_attr->val_type == LV_SVG_ATTR_VALUE_PTR) {
                float* vals = viewBox_attr->value.val;
                // [x, y, w, h]
                w_ = vals[2];
                h_ = vals[3];
            }
        }
    }
    if (height_attr != NULL) {
        if (height_attr->class_type != LV_SVG_ATTR_VALUE_NONE) {
            h_ = roundf(height_attr->value.fval);
        }
    }
    if (width_attr != NULL) {
        if (width_attr->class_type != LV_SVG_ATTR_VALUE_NONE) {
            w_ = roundf(width_attr->value.fval);
        }
    }
    if (size != NULL) {
        if (w_ > 0) {
            size->x = w_;
        } 
        if (h_ > 0) {
            size->y = h_;
        }
    } 
}


/**********************
 *   STATIC FUNCTIONS
 **********************/













#include "../../core/lv_obj_class_private.h"
#include "../../widgets/canvas/lv_canvas_private.h"
#include "../../draw/sw/lv_draw_sw.h"
#include "../../draw/lv_draw_private.h"
#include "../../misc/lv_types.h"

#include "lv_svg_render.h"

#if LV_USE_SVG_ANIMATION
#include "lv_svg_anim.h"
#endif

#include <limits.h>

/**********************
 *  STATIC VARIABLES
 **********************/
struct _lv_svg_t {
    // inheritance parent
    lv_canvas_t canvas;

    lv_svg_node_t * doc;
    lv_vector_dsc_t* dsc;
    lv_svg_render_obj_t * list;

    Tvg_Canvas * tvg_canvas;
    lv_anim_t * anim;
    float animTime_ms;
    int32_t last_rendered_time;

};

/*********************
*      DEFINES
*********************/

#define MY_CLASS (&lv_svg_class)


/**********************
*      TYPEDEFS
**********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_svg_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_svg_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void svg_anim_cb(void * var, int32_t v);
static void svg_update(lv_obj_t* obj, int32_t v, bool force_draw);
static void svg_update_draw(lv_svg_t* svg, int32_t v);


/**********************
 *  GLOBAL VARIABLES
 **********************/

const lv_obj_class_t lv_svg_class = {
    .constructor_cb = lv_svg_constructor,
    .destructor_cb = lv_svg_destructor,
    .width_def = LV_DPI_DEF,
    .height_def = LV_DPI_DEF,
    .instance_size = sizeof(lv_svg_t),
    .base_class = &lv_canvas_class,
    .name = "svg",
};

#define MY_CLASS (&lv_svg_class)


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


lv_obj_t * lv_svg_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


void lv_svg_set_buffer(lv_obj_t * obj, int32_t w, int32_t h, void * buf, uint32_t bufferSize_bytes, lv_color_format_t lvColorFormat)
{
    lv_svg_t * svg = (lv_svg_t *)obj;

    int32_t stride_bytes = lv_draw_buf_width_to_stride(w, lvColorFormat);
    Tvg_Colorspace tvgColorFormat = lv_lvgl_to_tvg(lvColorFormat);

    buf = lv_draw_buf_align(buf, lvColorFormat);

    uint32_t stride_pixels = stride_bytes / lv_color_format_get_size(lvColorFormat);
    tvg_swcanvas_set_target(svg->tvg_canvas, buf, stride_pixels, w, h, tvgColorFormat);
    lv_canvas_set_buffer(obj, buf, w, h, lvColorFormat);

    /* Rendered output images are premultiplied */
    lv_draw_buf_t * draw_buf = lv_canvas_get_draw_buf(obj);
    lv_draw_buf_set_flag(draw_buf, LV_IMAGE_FLAGS_PREMULTIPLIED);

    /*Force updating when the buffer changes*/
    svg_update(obj, (int32_t)0, true);
}

lv_anim_t * lv_svg_get_anim(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_svg_t * svg = (lv_svg_t *)obj;
    return svg->anim;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static void lv_svg_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_svg_t * svg = (lv_svg_t *)obj;

    svg->doc =NULL;
    svg->dsc =NULL;
    svg->list =NULL;
    svg->tvg_canvas = tvg_swcanvas_create();
    svg->anim = NULL;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, svg_anim_cb);
    lv_anim_set_var(&a, obj);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    svg->anim = lv_anim_start(&a);
    svg->animTime_ms = 0;

    lv_draw_buf_t * draw_buf = lv_canvas_get_draw_buf(obj);
    if(draw_buf) {
       lv_color_format_t cf = draw_buf->header.cf;
       LV_LOG("cf %d 0x%02X", cf, cf);		 
	}

    //lv_display_t * disp = lv_refr_get_disp_refreshing();
    //if(layer != disp->layer_head) {
//
    //}



    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_svg_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_svg_t* svg = (lv_svg_t*)obj;

    if (svg->anim != NULL) {
        lv_anim_delete(obj, svg_anim_cb);
        svg->anim = NULL;
    }

    tvg_canvas_destroy(svg->tvg_canvas);

    lv_svg_node_delete(svg->doc);

}


static void svg_update(lv_obj_t* obj, int32_t v, bool force_draw)
{
    lv_svg_t* svg = (lv_svg_t*)obj;

    /*Do not render not visible animations.*/
    if(lv_obj_is_visible(obj) || force_draw) {
        svg_update_draw(svg, v);
        if(svg->anim) {
            svg->last_rendered_time = svg->anim->act_time;
        }
    }
    else {
        /*Artificially keep the animation on the last rendered frame's time
         *To avoid a jump when the widget becomes visible*/
        if(svg->anim) {
            svg->anim->act_time = svg->last_rendered_time;
        }
    }
}


static void svg_anim_cb(void * var, int32_t v)
{
    lv_obj_t* obj = (lv_obj_t*)var;
	svg_update(obj, v, false);
}

static void lv_svg_node_set_size(lv_svg_node_t * node, const lv_point_t size)
{
    lv_svg_node_attr_set(node, LV_SVG_ATTR_WIDTH, size.x);
    lv_svg_node_attr_set(node, LV_SVG_ATTR_HEIGHT, size.y);
}

lv_point_t lv_svg_node_fit_size(lv_svg_node_t* node, const lv_point_t size, bool only_smaller)
{
    int32_t w = INT_MAX;
    int32_t h = INT_MAX;
    lv_svg_attr_t* attr_w = lv_svg_get_attr(node, LV_SVG_ATTR_WIDTH);
    lv_svg_attr_t* attr_h = lv_svg_get_attr(node, LV_SVG_ATTR_HEIGHT);
    if ((attr_w != NULL) && (attr_w->value.fval > 0) &&
        (attr_h != NULL) && (attr_h->value.fval > 0))
    {
        float ratioW = 1.0F;
        float ratioH = 1.0F;
        w = LV_MIN(w, attr_w->value.fval);
        if (size.x > 0) {
            if (only_smaller) {
                ratioW = LV_MIN(1.0F, (((float)size.x) / ((float)w)));
            }
            else {
                ratioW = (((float)size.x) / ((float)w));
            }
        }
        h = LV_MIN(h, attr_h->value.fval);
        if (size.y > 0) {
            if (only_smaller) {
                ratioH = LV_MIN(1.0F, (((float)size.y) / ((float)h)));
            }
            else {
                ratioH = (((float)size.y) / ((float)h));
            }
        }
        float ratio = LV_MIN(ratioW, ratioH);
        w = w * ratio;
        h = h * ratio;
    }
    lv_point_t result;
    result.x = ((size.x > 0) ? LV_MIN(w, size.x) : w);
    result.y = ((size.y > 0) ? LV_MIN(h, size.y) : h);

    lv_svg_node_set_size(node, result);

    lv_svg_render_fix(node);

    return result;
}

// widget_size == (0, 0) will not scale svg to widget size
void lv_svg_set_src_data(lv_obj_t* obj, const char * svg_data, uint32_t svg_data_len, const lv_point_t widget_size)
{
    lv_svg_t * svg = (lv_svg_t *)obj;

    lv_svg_node_delete(svg->doc);
    svg->doc = lv_svg_load_data(svg_data, svg_data_len);
    svg->animTime_ms = 0;

    lv_svg_node_fit_size(svg->doc, widget_size, true);

    lv_point_t svg_size;
    lv_svg_get_size(svg->doc, &svg_size);

    lv_image_t * img = (lv_image_t *)svg;

    img->w = svg_size.x;
    img->h = svg_size.y;

    if (lv_svg_node_has_animation(svg->doc)) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, svg_anim_cb);
        lv_anim_set_var(&a, obj);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        svg->anim = lv_anim_start(&a);
    }

    /*Force updating when the buffer changes*/
    svg_update(obj, 0, true);
}


static void svg_update_draw(lv_svg_t* svg, int32_t v)
{
    lv_obj_t* obj = (lv_obj_t*)svg;
    lv_image_t* img = (lv_image_t*)svg;

#if LV_USE_SVG_ANIMATION
    if ((svg->doc != NULL) && (svg->anim != NULL)) {
        lv_svg_node_animate_step(svg->doc, svg->animTime_ms);
    }
#endif

    lv_draw_buf_t * draw_buf = lv_canvas_get_draw_buf(obj);
    if(draw_buf) {
        lv_draw_buf_clear(draw_buf, NULL);
        /*Drop old cached image*/
        lv_image_cache_drop(lv_image_get_src(obj));

        Tvg_Colorspace tvg_cf = lv_lvgl_to_tvg(draw_buf->header.cf);
        lv_color_t bg_c = lv_obj_get_style_bg_color(lv_screen_active(), LV_PART_MAIN);
        lv_opa_t bg_a = lv_obj_get_style_bg_opa(lv_screen_active(), LV_PART_MAIN);
        lv_color32_t color = lv_color_to_32(bg_c, bg_a);

        lv_layer_t dummyLayer = 
        {
            .draw_buf = draw_buf,
            .buf_area = {
                .x1 = 0,
                .y1 = 0,
                .x2 = img->w + 0,
                .y2 = img->h + 0,
            },
            .color_format = draw_buf->header.cf,
            ._clip_area = {
                .x1 = 0,
                .y1 = 0,
                .x2 = img->w + 0,
                .y2 = img->h + 0,
            },
            .phy_clip_area = {
                .x1 = 0,
                .y1 = 0,
                .x2 = img->w + 0,
                .y2 = img->h + 0,
            },
            .draw_task_head = NULL,
            .parent = NULL,
            .next = NULL,
            .all_tasks_added = false,
            .user_data = NULL,
        };

    }
    lv_obj_invalidate(obj);

    if (v > 0) {
        svg->animTime_ms += LV_DEF_REFR_PERIOD;
    }
}


lv_svg_attr_t * lv_svg_add_attr(lv_svg_node_t * node, lv_svg_attr_type_t id)
{
    lv_result_t result = lv_array_push_back(&node->attrs, NULL);
    LV_ASSERT(LV_RESULT_OK == result);       
    lv_svg_attr_t* nodeAttr = lv_array_at(&node->attrs, node->attrs.size - 1);
    LV_ASSERT(nodeAttr != NULL);       
    nodeAttr->id = id;
    nodeAttr->value.val = NULL;

    return nodeAttr;
}


static void lv_svg_node_attr_set(lv_svg_node_t * node, lv_svg_attr_type_t id, float value)
{
    lv_svg_attr_t* attr = lv_svg_get_attr(node, id);
    if (attr == NULL)
    {
        attr = lv_svg_add_attr(node, id);
        attr->val_type = LV_SVG_ATTR_VALUE_DATA;
        attr->class_type = LV_SVG_ATTR_VALUE_INITIAL;
    }
    attr->value.fval = value;
}


typedef bool (*lv_svg_node_iterator_cb_t)(lv_svg_node_t* node, void* user_data);

bool lv_svg_node_iterate(lv_svg_node_t* node, lv_svg_node_iterator_cb_t cb, void* user_data)
{
    bool result = cb(node, user_data);

    lv_tree_node_t* tree_node = (lv_tree_node_t*)node;
    uint32_t i = 0;
    while (result && (i < tree_node->child_cnt)) {
        lv_svg_node_t* child_node = ((lv_svg_node_t*)tree_node->children[i]);
        if (!lv_svg_node_iterate(child_node, cb, user_data)) {
            result = false;
        }
        else {
            i++;
        }
    }
    return result;
}

typedef struct lv_svg_attr_iterator_t_ {
    lv_svg_attr_iterator_cb_t cb;
    void* user_data;
} lv_svg_attr_iterator_t;

static bool node_attr_iterator(lv_svg_node_t* node, void* user_data)
{
    bool result = true;
    lv_svg_attr_iterator_t* svg_attr_iterator = (lv_svg_attr_iterator_t*)user_data;
    const uint32_t len = lv_array_size(&node->attrs);
    uint32_t i = 0;
    while (result && (i < len)) {
        lv_svg_attr_t* attr = lv_array_at(&node->attrs, i);
        if (!svg_attr_iterator->cb(node, attr, svg_attr_iterator->user_data)) {
            result = false;
        }
        else {
            i++;
        }
    }
    return result;
}

bool lv_svg_attr_iterate(lv_svg_node_t* node, lv_svg_attr_iterator_cb_t cb, void* user_data)
{
    lv_svg_attr_iterator_t svg_attr_iterator = {cb, (lv_svg_attr_iterator_t*)user_data};
    
    return lv_svg_node_iterate(node, node_attr_iterator, &svg_attr_iterator);
}

#endif /*LV_USE_SVG*/
