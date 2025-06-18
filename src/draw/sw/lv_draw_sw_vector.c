/**
 * @file lv_draw_img.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lv_image_decoder_private.h"
#include "../lv_draw_vector_private.h"
#include "../lv_draw_private.h"
#include "lv_draw_sw.h"

#if LV_USE_VECTOR_GRAPHIC && LV_USE_THORVG
#if LV_USE_THORVG_EXTERNAL
    #include <thorvg_capi.h>
#else
    #include "../../libs/thorvg/src/bindings/capi/thorvg_capi.h"
#endif
#include "../../stdlib/lv_string.h"
#include "blend/lv_draw_sw_blend_private.h"
#include "blend/lv_draw_sw_blend_to_rgb565.h"
#include "blend/lv_draw_sw_blend_to_rgb888.h"

#include "../../widgets/canvas/lv_canvas.h"

#include "../../core/lv_obj_private.h"
#include "../../misc/lv_area_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    float x;
    float y;
    float w;
    float h;
} _tvg_rect;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} _tvg_color;

typedef struct {
    Tvg_Canvas * canvas;
    lv_point_t translate;
} _tvg_draw_state;
/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

static void lv_area_to_tvg(_tvg_rect * rect, const lv_area_t * area)
{
    rect->x = area->x1;
    rect->y = area->y1;
    rect->w = lv_area_get_width(area);
    rect->h = lv_area_get_height(area);
}

static void lv_color_to_tvg(_tvg_color * color, const lv_color32_t * c, lv_opa_t opa)
{
    color->r = c->red;
    color->g = c->green;
    color->b = c->blue;
    color->a = LV_OPA_MIX2(c->alpha, opa);
}

static void lv_matrix_to_tvg(Tvg_Matrix * tm, const lv_matrix_t * m)
{
    tm->e11 = m->m[0][0];
    tm->e12 = m->m[0][1];
    tm->e13 = m->m[0][2];
    tm->e21 = m->m[1][0];
    tm->e22 = m->m[1][1];
    tm->e23 = m->m[1][2];
    tm->e31 = m->m[2][0];
    tm->e32 = m->m[2][1];
    tm->e33 = m->m[2][2];
}

static void _set_paint_matrix(Tvg_Paint * obj, const Tvg_Matrix * m)
{
    tvg_paint_set_transform(obj, m);
}

static void _set_paint_shape(Tvg_Paint * obj, const lv_vector_path_t * p)
{
    uint32_t pidx = 0;
    lv_vector_path_op_t * op = lv_array_front(&p->ops);
    uint32_t size = lv_array_size(&p->ops);
    for(uint32_t i = 0; i < size; i++) {
        switch(op[i]) {
            case LV_VECTOR_PATH_OP_MOVE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&p->points, pidx);
                    tvg_shape_move_to(obj, pt->x, pt->y);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_LINE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&p->points, pidx);
                    tvg_shape_line_to(obj, pt->x, pt->y);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_QUAD_TO: {
                    lv_fpoint_t * pt1 = lv_array_at(&p->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&p->points, pidx + 1);

                    lv_fpoint_t * last_pt = lv_array_at(&p->points, pidx - 1);

                    lv_fpoint_t cp[2];
                    cp[0].x = (last_pt->x + 2 * pt1->x) * (1.0f / 3.0f);
                    cp[0].y = (last_pt->y + 2 * pt1->y) * (1.0f / 3.0f);
                    cp[1].x = (pt2->x + 2 * pt1->x) * (1.0f / 3.0f);
                    cp[1].y = (pt2->y + 2 * pt1->y) * (1.0f / 3.0f);

                    tvg_shape_cubic_to(obj, cp[0].x, cp[0].y, cp[1].x, cp[1].y, pt2->x, pt2->y);
                    pidx += 2;
                }
                break;
            case LV_VECTOR_PATH_OP_CUBIC_TO: {
                    lv_fpoint_t * pt1 = lv_array_at(&p->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&p->points, pidx + 1);
                    lv_fpoint_t * pt3 = lv_array_at(&p->points, pidx + 2);

                    tvg_shape_cubic_to(obj, pt1->x, pt1->y, pt2->x, pt2->y, pt3->x, pt3->y);
                    pidx += 3;
                }
                break;
            case LV_VECTOR_PATH_OP_CLOSE: {
                    tvg_shape_close(obj);
                }
                break;
        }
    }
}

static Tvg_Stroke_Cap lv_stroke_cap_to_tvg(lv_vector_stroke_cap_t cap)
{
    switch(cap) {
        case LV_VECTOR_STROKE_CAP_SQUARE:
            return TVG_STROKE_CAP_SQUARE;
        case LV_VECTOR_STROKE_CAP_ROUND:
            return TVG_STROKE_CAP_ROUND;
        case LV_VECTOR_STROKE_CAP_BUTT:
            return TVG_STROKE_CAP_BUTT;
        default:
            return TVG_STROKE_CAP_SQUARE;
    }
}

static Tvg_Stroke_Join lv_stroke_join_to_tvg(lv_vector_stroke_join_t join)
{
    switch(join) {
        case LV_VECTOR_STROKE_JOIN_BEVEL:
            return TVG_STROKE_JOIN_BEVEL;
        case LV_VECTOR_STROKE_JOIN_ROUND:
            return TVG_STROKE_JOIN_ROUND;
        case LV_VECTOR_STROKE_JOIN_MITER:
            return TVG_STROKE_JOIN_MITER;
        default:
            return TVG_STROKE_JOIN_BEVEL;
    }
}

static Tvg_Stroke_Fill lv_spread_to_tvg(lv_vector_gradient_spread_t sp)
{
    switch(sp) {
        case LV_VECTOR_GRADIENT_SPREAD_PAD:
            return TVG_STROKE_FILL_PAD;
        case LV_VECTOR_GRADIENT_SPREAD_REPEAT:
            return TVG_STROKE_FILL_REPEAT;
        case LV_VECTOR_GRADIENT_SPREAD_REFLECT:
            return TVG_STROKE_FILL_REFLECT;
        default:
            return TVG_STROKE_FILL_PAD;
    }
}

static void _setup_gradient(Tvg_Gradient * gradient, const lv_vector_gradient_t * grad,
                            const lv_matrix_t * matrix)
{
    Tvg_Color_Stop * stops = (Tvg_Color_Stop *)lv_malloc(sizeof(Tvg_Color_Stop) * grad->stops_count);
    LV_ASSERT_MALLOC(stops);
    for(uint16_t i = 0; i < grad->stops_count; i++) {
        const lv_grad_stop_t * s = &(grad->stops[i]);

        stops[i].offset = s->frac / 255.0f;
        stops[i].r = s->color.red;
        stops[i].g = s->color.green;
        stops[i].b = s->color.blue;
        stops[i].a = s->opa;
    }

    tvg_gradient_set_color_stops(gradient, stops, grad->stops_count);
    tvg_gradient_set_spread(gradient, lv_spread_to_tvg(grad->spread));
    Tvg_Matrix mtx;
    lv_matrix_to_tvg(&mtx, matrix);
    tvg_gradient_set_transform(gradient, &mtx);
    lv_free(stops);
}

static void _set_paint_stroke_gradient(Tvg_Paint * obj, const lv_vector_gradient_t * g, const lv_matrix_t * m)
{
    Tvg_Gradient * grad = NULL;
    if(g->style == LV_VECTOR_GRADIENT_STYLE_RADIAL) {
        grad = tvg_radial_gradient_new();
        tvg_radial_gradient_set(grad, g->cx, g->cy, g->cr, g->cx, g->cy, 0);
        _setup_gradient(grad, g, m);
        tvg_shape_set_stroke_gradient(obj, grad);
    }
    else {
        grad = tvg_linear_gradient_new();
        tvg_linear_gradient_set(grad, g->x1, g->y1, g->x2, g->y2);
        _setup_gradient(grad, g, m);
        tvg_shape_set_stroke_gradient(obj, grad);
    }
}

static void _set_paint_stroke(Tvg_Paint * obj, const lv_vector_stroke_dsc_t * dsc)
{
    if(dsc->style == LV_VECTOR_DRAW_STYLE_SOLID) {
        _tvg_color c;
        lv_color_to_tvg(&c, &dsc->color, dsc->opa);
        tvg_shape_set_stroke_color(obj, c.r, c.g, c.b, c.a);
    }
    else {   /*gradient*/
        _set_paint_stroke_gradient(obj, &dsc->gradient, &dsc->matrix);
    }

    tvg_shape_set_stroke_width(obj, dsc->width);
    tvg_shape_set_stroke_miterlimit(obj, dsc->miter_limit);
    tvg_shape_set_stroke_cap(obj, lv_stroke_cap_to_tvg(dsc->cap));
    tvg_shape_set_stroke_join(obj, lv_stroke_join_to_tvg(dsc->join));

    if(!lv_array_is_empty(&dsc->dash_pattern)) {
        float * dash_array = lv_array_front(&dsc->dash_pattern);
        tvg_shape_set_stroke_dash(obj, dash_array, dsc->dash_pattern.size, 0);
    }
}

static Tvg_Fill_Rule lv_fill_rule_to_tvg(lv_vector_fill_t rule)
{
    switch(rule) {
        case LV_VECTOR_FILL_NONZERO:
            return TVG_FILL_RULE_NON_ZERO;
        case LV_VECTOR_FILL_EVENODD:
            return TVG_FILL_RULE_EVEN_ODD;
        default:
            return TVG_FILL_RULE_NON_ZERO;
    }
}

static void _set_paint_fill_gradient(Tvg_Paint * obj, const lv_vector_gradient_t * g, const lv_matrix_t * m)
{
    Tvg_Gradient * grad = NULL;
    if(g->style == LV_VECTOR_GRADIENT_STYLE_RADIAL) {
        grad = tvg_radial_gradient_new();
        tvg_radial_gradient_set(grad, g->cx, g->cy, g->cr, g->cx, g->cy, 0);
        _setup_gradient(grad, g, m);
        tvg_shape_set_gradient(obj, grad);
    }
    else {
        grad = tvg_linear_gradient_new();
        tvg_linear_gradient_set(grad, g->x1, g->y1, g->x2, g->y2);
        _setup_gradient(grad, g, m);
        tvg_shape_set_gradient(obj, grad);
    }
}

static void _set_paint_fill_pattern(Tvg_Paint * obj, Tvg_Canvas * canvas, const lv_draw_image_dsc_t * p,
                                    const lv_matrix_t * m)
{
    lv_image_decoder_dsc_t decoder_dsc;
    lv_image_decoder_args_t args = { 0 };
    args.premultiply = 1;
    lv_result_t res = lv_image_decoder_open(&decoder_dsc, p->src, &args);
    if(res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        return;
    }

    if(!decoder_dsc.decoded) {
        lv_image_decoder_close(&decoder_dsc);
        LV_LOG_ERROR("Image not ready");
        return;
    }

    const uint8_t * src_buf = decoder_dsc.decoded->data;
    const lv_image_header_t * header = &decoder_dsc.decoded->header;
    lv_color_format_t cf = header->cf;

#if PIXEL_TYPE_SIZE == 4
    if(cf != LV_COLOR_FORMAT_ARGB8888 && cf != LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED) {
        lv_image_decoder_close(&decoder_dsc);
        LV_LOG_ERROR("Not support image format");
        return;
    }
    Tvg_Colorspace cs = TVG_COLORSPACE_UNKNOWN;
    if(cf == LV_COLOR_FORMAT_ARGB8888) {
    	cs = TVG_COLORSPACE_ARGB8888;
    }
    else if(cf == LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED) {
    	cs = TVG_COLORSPACE_ARGB8888S;
    }
#elif PIXEL_TYPE_SIZE == 2
    if(cf != LV_COLOR_FORMAT_RGB565) {
        lv_image_decoder_close(&decoder_dsc);
        LV_LOG_ERROR("Not support image format");
        return;
    }
    Tvg_Colorspace cs = TVG_COLORSPACE_RGB565;    
#endif
    Tvg_Paint * img = tvg_picture_new();
    tvg_picture_load_raw(img, (PIXEL_TYPE*)src_buf, header->w, header->h, cs, true);
    Tvg_Paint * clip_path = tvg_paint_duplicate(obj);
    // @deprecated Use Paint::clip()
    tvg_paint_set_mask_method(img, clip_path, TVG_MASK_METHOD_ALPHA);
    tvg_paint_set_opacity(img, p->opa);

    Tvg_Matrix mtx;
    lv_matrix_to_tvg(&mtx, m);
    tvg_paint_set_transform(img, &mtx);
    tvg_canvas_push(canvas, img);
    lv_image_decoder_close(&decoder_dsc);
}

static void _set_paint_fill(Tvg_Paint * obj, Tvg_Canvas * canvas, const lv_vector_fill_dsc_t * dsc,
                            const lv_matrix_t * matrix)
{
    tvg_shape_set_fill_rule(obj, lv_fill_rule_to_tvg(dsc->fill_rule));

    if(dsc->style == LV_VECTOR_DRAW_STYLE_SOLID) {
        _tvg_color c;
        lv_color_to_tvg(&c, &dsc->color, dsc->opa);
        tvg_shape_set_fill_color(obj, c.r, c.g, c.b, c.a);
    }
    else if(dsc->style == LV_VECTOR_DRAW_STYLE_PATTERN) {
        lv_matrix_t imx = *matrix;

        if(dsc->fill_units == LV_VECTOR_FILL_UNITS_OBJECT_BOUNDING_BOX) {
            /* Convert to object bounding box coordinates */
            Tvg_Point boundingBox[4];
            tvg_paint_get_obb(obj, boundingBox);
            lv_matrix_translate(&imx, boundingBox[0].x, boundingBox[0].y);
        }

        lv_matrix_multiply(&imx, &dsc->matrix);

        _set_paint_fill_pattern(obj, canvas, &dsc->img_dsc, &imx);
    }
    else if(dsc->style == LV_VECTOR_DRAW_STYLE_GRADIENT) {
        _set_paint_fill_gradient(obj, &dsc->gradient, &dsc->matrix);
    }
}

static Tvg_Blend_Method lv_blend_to_tvg(lv_vector_blend_t blend)
{
    switch(blend) {
        case LV_VECTOR_BLEND_SRC_OVER:
            return TVG_BLEND_METHOD_NORMAL;
        case LV_VECTOR_BLEND_SCREEN:
            return TVG_BLEND_METHOD_SCREEN;
        case LV_VECTOR_BLEND_MULTIPLY:
            return TVG_BLEND_METHOD_MULTIPLY;
        case LV_VECTOR_BLEND_ADDITIVE:
            return TVG_BLEND_METHOD_ADD;
        case LV_VECTOR_BLEND_SRC_IN:
        case LV_VECTOR_BLEND_DST_OVER:
        case LV_VECTOR_BLEND_DST_IN:
        case LV_VECTOR_BLEND_SUBTRACTIVE:
        /*not support yet.*/
        case LV_VECTOR_BLEND_NONE:
        default:
            return TVG_BLEND_METHOD_NORMAL;
    }
}

static void _set_paint_blend_mode(Tvg_Paint * obj, lv_vector_blend_t blend)
{
    tvg_paint_set_blend_method(obj, lv_blend_to_tvg(blend));
}

static void _blend_draw_buf(lv_draw_buf_t * draw_buf, const lv_area_t * dst_area, const lv_draw_buf_t * new_buf,
                            const lv_area_t * src_area)
{
    lv_draw_sw_blend_image_dsc_t fill_dsc;
    fill_dsc.dest_w = src_area->x2;
    fill_dsc.dest_h = src_area->y2;
    fill_dsc.dest_stride = draw_buf->header.stride;
    fill_dsc.dest_buf = draw_buf->data;

    fill_dsc.opa = LV_OPA_100;
    fill_dsc.blend_mode = LV_BLEND_MODE_NORMAL;
    fill_dsc.src_stride = new_buf->header.stride;
    fill_dsc.src_color_format = new_buf->header.cf;
    fill_dsc.src_buf = new_buf->data;

    fill_dsc.mask_buf = NULL;
    fill_dsc.mask_stride = 0;

    fill_dsc.relative_area = *dst_area;
    fill_dsc.src_area  = *src_area;

    switch(draw_buf->header.cf) {
#if LV_DRAW_SW_SUPPORT_RGB565
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_RGB565A8:
            lv_draw_sw_blend_image_to_rgb565(&fill_dsc);
            break;
#endif
#if LV_DRAW_SW_SUPPORT_RGB888
        case LV_COLOR_FORMAT_RGB888:
            lv_draw_sw_blend_image_to_rgb888(&fill_dsc, 3);
            break;
#endif
        default:
            break;
    }
}

static void _task_draw_cb(void * ctx, const lv_vector_path_t * path, const lv_vector_draw_dsc_t * dsc)
{
    _tvg_draw_state * state = (_tvg_draw_state *)ctx;
    Tvg_Canvas * canvas = state->canvas;

    Tvg_Paint * obj = tvg_shape_new();

    _tvg_rect rc;
    lv_area_to_tvg(&rc, &dsc->scissor_area);

    if(!path) {  /*clear*/
        _tvg_color c;
        lv_color_to_tvg(&c, &dsc->fill_dsc.color, dsc->fill_dsc.opa);

        Tvg_Matrix mtx = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
        };
        mtx.e13 += (float)(state->translate.x);
        mtx.e23 += (float)(state->translate.y);
        tvg_shape_append_rect(obj, 0, 0, rc.w, rc.h, 0, 0, true);
        tvg_shape_set_fill_color(obj, c.r, c.g, c.b, c.a);
        _set_paint_matrix(obj, &mtx);
    }
    else {
        // untranslated viewport on target buffer
        tvg_canvas_set_viewport(canvas, 0, 0, (int32_t)rc.w, (int32_t)rc.h);

        lv_matrix_t matrix;
        lv_matrix_identity(&matrix);
        lv_matrix_translate(&matrix, (float)state->translate.x, (float)state->translate.y);
        lv_matrix_multiply(&matrix, &dsc->matrix);
        Tvg_Matrix mtx;
        lv_matrix_to_tvg(&mtx, &matrix);
        _set_paint_matrix(obj, &mtx);

        _set_paint_shape(obj, path);

        _set_paint_fill(obj, canvas, &dsc->fill_dsc, &matrix);
        _set_paint_stroke(obj, &dsc->stroke_dsc);
        _set_paint_blend_mode(obj, dsc->blend_mode);
    }
    tvg_canvas_push(canvas, obj);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_draw_sw_vector(lv_draw_task_t * t, lv_draw_vector_task_dsc_t * dsc, const lv_area_t * coords, lv_color32_t color)
{
    if(dsc->task_list == NULL)
        return;

    lv_layer_t * layer = dsc->base.layer;
    lv_draw_buf_t * draw_buf = layer->draw_buf;
    if(draw_buf == NULL)
        return;

    void * buf = draw_buf->data;
    int32_t width = lv_area_get_width(&layer->buf_area);
    int32_t height = lv_area_get_height(&layer->buf_area);
    uint32_t stride_bytes = draw_buf->header.stride;

    lv_color_format_t cf = draw_buf->header.cf;

#if PIXEL_TYPE_SIZE == 4
    if ((cf != LV_COLOR_FORMAT_RGB888) &&
        (cf != LV_COLOR_FORMAT_ARGB8888) &&
        (cf != LV_COLOR_FORMAT_XRGB8888) &&
        (cf != LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED)) {
        LV_LOG_ERROR("unsupported layer color: %d", cf);
        return;
    }
    PIXEL_TYPE tvg_color = lv_color_32_to_u32(color);
#elif PIXEL_TYPE_SIZE == 2
    if(cf != LV_COLOR_FORMAT_RGB565) {
        LV_LOG_ERROR("unsupported layer color: %d", cf);
        return;
    }
    lv_color_t c16 = {.blue = color.blue, .green = color.green, .red = color.red};
    PIXEL_TYPE tvg_color = lv_color_to_u16(c16);
#endif

    Tvg_Colorspace tvg_cf = lv_lvgl_to_tvg(cf);
    uint32_t stride_pixels = stride_bytes / PIXEL_TYPE_SIZE;
    uint32_t data_stride = t->target_layer->draw_buf->header.stride;

    lv_area_t draw_area;
    if(!lv_area_intersect(&draw_area, &(t->target_layer->buf_area), &t->clip_area)) {
        // No intersection
    }

    lv_point_t tvg_offset = {draw_area.x1 - coords->x1, draw_area.y1 - coords->y1};
    lv_point_t buf_area_offset = {draw_area.x1 - t->target_layer->buf_area.x1, draw_area.y1 - t->target_layer->buf_area.y1};

    Tvg_Canvas * canvas = tvg_swcanvas_create();

    uint32_t* tvg_data = lv_draw_layer_go_to_xy(t->target_layer, buf_area_offset.x, buf_area_offset.y);

    tvg_swcanvas_set_target(canvas, tvg_data, stride_pixels, lv_area_get_width(&draw_area), lv_area_get_height(&draw_area), tvg_cf);
    
    tvg_canvas_set_viewport(canvas, 0, 0, lv_area_get_width(&draw_area), lv_area_get_height(&draw_area));

    _tvg_draw_state state = {canvas, {-tvg_offset.x, -tvg_offset.y}};

    lv_ll_t * task_list = dsc->task_list;
    dsc->task_list = NULL;
    lv_vector_for_each_destroy_tasks(task_list, _task_draw_cb, &state);

    if(tvg_canvas_draw(canvas, true, tvg_color) == TVG_RESULT_SUCCESS) {
        tvg_canvas_sync(canvas);
    }

    tvg_canvas_destroy(canvas);
}


void lv_draw_vector_set_viewport_tvg_canvas(lv_area_t* area, Tvg_Canvas * canvas)
{
    _tvg_rect rc;
    lv_area_to_tvg(&rc, area);
    tvg_canvas_set_viewport(canvas, (int32_t)rc.x, (int32_t)rc.y, (int32_t)rc.w, (int32_t)rc.h);
}

Tvg_Colorspace lv_lvgl_to_tvg(lv_color_format_t lvColorFormat)
{
    Tvg_Colorspace result = TVG_COLORSPACE_UNKNOWN;
    switch (lvColorFormat)
    {
#if PIXEL_TYPE_SIZE == 4
        case LV_COLOR_FORMAT_RGB888  : result = TVG_COLORSPACE_ABGR8888  ; break;
        case LV_COLOR_FORMAT_ARGB8888: result = TVG_COLORSPACE_ARGB8888  ; break;
        case LV_COLOR_FORMAT_XRGB8888: result = TVG_COLORSPACE_ARGB8888  ; break;
        case LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED: result = TVG_COLORSPACE_ARGB8888S  ; break;
#elif PIXEL_TYPE_SIZE == 2
        case LV_COLOR_FORMAT_RGB565: result = TVG_COLORSPACE_RGB565    ; break;
#elif PIXEL_TYPE_SIZE == 1
        case LV_COLOR_FORMAT_L8    : result = TVG_COLORSPACE_GRAYSCALE8; break;
#endif        
        default: 
            // not supported
            LV_LOG_ERROR("Failed to convert LVGL %d for pixel size %d", lvColorFormat, PIXEL_TYPE_SIZE);

            LV_ASSERT(false);
            break;
    }
    return result;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_SW*/
