/**
 * @file lv_svg_anim.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_svg_anim.h"
#if LV_USE_SVG

#include "../../misc/lv_assert.h"
#include "../../misc/lv_log.h"
#include "../../stdlib/lv_mem.h"

#include "lv_svg.h"
#include "lv_svg_token.h"
#include "lv_svg_parser.h"
#include "../../misc/lv_math.h"

#include <math.h>

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

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/



static lv_svg_node_t* lv_svg_node_has_child_of_type(lv_svg_node_t* node, lv_svg_tag_t type)
{
    lv_svg_node_t* node_type = NULL;
    lv_tree_node_t* tree_node = (lv_tree_node_t*)node;
    for(uint32_t i = 0; i < tree_node->child_cnt; i++) {
        if (((lv_svg_node_t*)tree_node->children[i])->type == type) {
            node_type = (lv_svg_node_t*)tree_node->children[i];
        }
    }
    return node_type;
}

static bool lv_svg_node_animate_attr_value(lv_svg_node_t* node, lv_svg_attr_type_t attr_id, float* value)
{
    bool found = false;
    lv_svg_attr_t* attr = lv_svg_get_attr(node, attr_id);
    if (attr != NULL) {
        if (attr->val_type == LV_SVG_ATTR_VALUE_PTR) {
            lv_svg_attr_values_list_t* attr_values = ((lv_svg_attr_values_list_t*)attr->value.val);
            // get first
            if ((attr_values != NULL) &&
                (attr_values->length > 0)) {
                found = true;
                float* vals = (float *)(&(attr_values->data));
                *value = vals[0];
            }
        }
        else if (attr->val_type == LV_SVG_ATTR_VALUE_DATA) {
            // get first
            found = true;
            *value = attr->value.fval;
        }
    }

    return found;
}

static bool lv_svg_node_animate_attr_values(lv_svg_node_t* node, lv_svg_attr_type_t attr_id, float* values, uint32_t nr_of_values)
{
    bool found = false;
    lv_svg_attr_t* attr = lv_svg_get_attr(node, attr_id);
    if (attr != NULL) {
        if (attr->val_type == LV_SVG_ATTR_VALUE_PTR) {
            lv_svg_attr_values_list_t* attr_values = ((lv_svg_attr_values_list_t*)attr->value.val);
            // get first nr_of_values
            if ((attr_values != NULL) &&
                (attr_values->length >= nr_of_values)) {
                found = true;
                float* vals = (float *)(&(attr_values->data));
                lv_memcpy(values, vals, nr_of_values*sizeof(float));
            }
        }
    }

    return found;
}


static void set_lv_svg_point_when_not_set(lv_svg_point_t** dst_point, lv_svg_point_t* src_point)
{
    if (*dst_point == NULL)
    {
        *dst_point = src_point;
    }
}

extern size_t lv_svg_attr_path_get_seg_size(uint32_t cmd);

static bool lv_svg_node_animate_attr_path(lv_svg_node_t* node, float fraction, lv_svg_point_t* point)
{
    bool found = false;

    // For now a simplified animation on a path.
    // instead of calculating the path, for simplicity the line 
    // between the first and last point is taken, and checked whether the path is closed.
    // When not closed, the object move on the line from start to end.
    // When closed it moves back and forth on the line.
    lv_svg_point_t* firstPoint = NULL;
    lv_svg_point_t* lastPoint = NULL;
    bool closed = false;

    // iterating the path is taken from 
    // 'static void _set_path_attr(lv_svg_render_obj_t * obj, lv_vector_draw_dsc_t * dsc, const lv_svg_attr_t * attr)'
    lv_svg_attr_t* attr = lv_svg_get_attr(node, LV_SVG_ATTR_PATH);
    if (attr != NULL) {
        lv_svg_attr_values_list_t * vals = (lv_svg_attr_values_list_t *)(attr->value.val);
        uint32_t len = vals->length;
        uint8_t* data_ptr = (uint8_t*)(&vals->data);

        for(uint32_t i = 0; i < len; i++) {
            lv_svg_attr_path_value_t* path_seg = (lv_svg_attr_path_value_t *)data_ptr;
            uint32_t cmd = path_seg->cmd;
            lv_svg_point_t* points = (lv_svg_point_t*)(&path_seg->data);
            switch(cmd) {
                case LV_SVG_PATH_CMD_MOVE_TO: 
                case LV_SVG_PATH_CMD_LINE_TO: {
                        set_lv_svg_point_when_not_set(&firstPoint, &points[0]);
                        lastPoint = &points[0];
                    }
                    break;
                case LV_SVG_PATH_CMD_QUAD_TO: {
                        set_lv_svg_point_when_not_set(&firstPoint, &points[0]);
                        lastPoint = &points[1];
                    }
                    break;
                case LV_SVG_PATH_CMD_CURVE_TO: {
                        set_lv_svg_point_when_not_set(&firstPoint, &points[2]);
                        lastPoint = &points[2];
                    }
                    break;
                case LV_SVG_PATH_CMD_CLOSE: {
                        closed = true;
                    }
                    break;
            }
            size_t mem_inc = lv_svg_attr_path_get_seg_size(cmd);
            data_ptr += mem_inc;
        }
    }
    if ((firstPoint != NULL) && (lastPoint != NULL) && (point != NULL)) {
        found = true;
        float fraction_ = fraction;
        if (closed)
        {
            fraction_ = 2*fraction;
            if (fraction_ > 1.0F)
            {
                fraction_ = 1 - fraction_;
            }
        }
        fraction_ = LV_CLAMP(fraction_, 0.0F, 1.0F);
        point->x = firstPoint->x + fraction_ * (lastPoint->x - firstPoint->x);
        point->y = firstPoint->y + fraction_ * (lastPoint->y - firstPoint->y);
    }
    return found;
}

static void lv_svg_node_animate_node_step(lv_svg_node_t* node, lv_svg_node_t* animate_node, float time_ms)
{
    lv_svg_attr_t* attrName_attr = lv_svg_get_attr(animate_node, LV_SVG_ATTR_ATTRIBUTE_NAME);
    
    if (attrName_attr != NULL) {
        lv_svg_attr_type_t nodeAttrId = (lv_svg_attr_type_t)attrName_attr->value.ival;
        lv_svg_attr_t* nodeAttr = lv_svg_get_attr(node, nodeAttrId);
        if (nodeAttr != NULL) {
            // Asume _process_length_value(...) values
            // These are of type float: nodeAttr->value.fval
            float from = 0;
            float to = 0;
            float begin_ms = 0;
            float end_ms = 0;
            float duration_ms = 0;
            if (!lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_BEGIN, &begin_ms)) {
                begin_ms = 0;
            }
            if (!lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_DUR, &duration_ms)) {
                duration_ms = 0;
            }
            if (!lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_END, &end_ms)) {
                end_ms = begin_ms + duration_ms;
            }

            float fraction = 0;
            int t = floor(time_ms/end_ms);
            float remain_ms = time_ms - (t*end_ms);

            lv_svg_attr_t* repeatCount_attr = lv_svg_get_attr(animate_node, LV_SVG_ATTR_REPEAT_COUNT);
            if ((repeatCount_attr != NULL) &&
                (repeatCount_attr->value.uval != 0)) {
                float total_ms = repeatCount_attr->value.uval * end_ms;
                if (time_ms >= total_ms) {
                    // force fraction to 1
                    remain_ms = end_ms;
                }
            }

            if (remain_ms <= begin_ms) {
                fraction = 0;
            }
            else if ((remain_ms >= (begin_ms + duration_ms)) || 
                     // prevent division by zero
                     (duration_ms < 1)) {
                fraction = 1;
            }
            else {
                fraction = ((remain_ms - begin_ms) / duration_ms);
            }
            fraction = LV_CLAMP(fraction, 0.0F, 1.0F);

            if (lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_FROM, &from) &&
                lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_TO, &to)) {
                float p = from + (fraction * (to - from));
                if (nodeAttr->val_type == LV_SVG_ATTR_VALUE_DATA) {
                    nodeAttr->value.fval = p;
                }
            }
        }      
    }
}

static float lv_svg_node_animate_get_fraction(lv_svg_node_t* animate_node, float time_ms)
{
    float begin_ms = 0;
    float end_ms = 0;
    float duration_ms = 0;
    if (!lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_BEGIN, &begin_ms)) {
        begin_ms = 0;
    }
    if (!lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_DUR, &duration_ms)) {
        duration_ms = 0;
    }
    if (!lv_svg_node_animate_attr_value(animate_node, LV_SVG_ATTR_END, &end_ms)) {
        end_ms = begin_ms + duration_ms;
    }

    float fraction = 0;
    int t = floor(time_ms/end_ms);
    float remain_ms = time_ms - (t*end_ms);

    lv_svg_attr_t* repeatCount_attr = lv_svg_get_attr(animate_node, LV_SVG_ATTR_REPEAT_COUNT);
    if ((repeatCount_attr != NULL) &&
        (repeatCount_attr->value.uval != 0)) {
        float total_ms = repeatCount_attr->value.uval * end_ms;
        if (time_ms >= total_ms) {
            // force fraction to 1
            remain_ms = end_ms;
        }
    }

    if (remain_ms <= begin_ms) {
        fraction = 0;
    }
    else if ((remain_ms >= (begin_ms + duration_ms)) || 
                // prevent division by zero
                (duration_ms < 1)) {
        fraction = 1;
    }
    else {
        fraction = ((remain_ms - begin_ms) / duration_ms);
    }
    fraction = LV_CLAMP(fraction, 0.0F, 1.0F);

    return fraction;
}


static lv_svg_attr_t* lv_svg_node_get_or_create_attr_transform(lv_svg_node_t* node)
{
    lv_svg_attr_t* nodeAttr = lv_svg_get_attr(node, LV_SVG_ATTR_TRANSFORM);
    if (nodeAttr == NULL) {
        nodeAttr = lv_svg_add_attr(node,  LV_SVG_ATTR_TRANSFORM);
        nodeAttr->val_type = LV_SVG_ATTR_VALUE_PTR;
        nodeAttr->class_type = LV_SVG_ATTR_VALUE_INITIAL;

        lv_svg_matrix_t * matrix = lv_malloc_zeroed(sizeof(lv_svg_matrix_t));
        LV_ASSERT_MALLOC(matrix);
        // identity
        matrix->m[0][0] = matrix->m[1][1] = matrix->m[2][2] = 1.0f;
        nodeAttr->value.val = matrix;

        nodeAttr = lv_svg_get_attr(node, LV_SVG_ATTR_TRANSFORM);
    }
    return nodeAttr;
}


static void lv_svg_node_animate_transform_node_step(lv_svg_node_t* node, lv_svg_node_t* animate_node, float time_ms)
{
    lv_svg_attr_t* attrType_attr = lv_svg_get_attr(animate_node, LV_SVG_ATTR_TRANSFORM_TYPE);
    lv_svg_attr_t* attrName_attr = lv_svg_get_attr(animate_node, LV_SVG_ATTR_ATTRIBUTE_NAME);
    
    if ((attrName_attr != NULL) && (attrType_attr!=NULL)) {
        lv_svg_attr_type_t nodeAttrId = (lv_svg_attr_type_t)attrName_attr->value.ival;
        lv_svg_attr_t* nodeAttr = lv_svg_get_attr(node, nodeAttrId);
        if ((nodeAttr == NULL) && (nodeAttrId == LV_SVG_ATTR_TRANSFORM)) {
            // Add missing attribute which is animated.
            // For now only "transform"
            nodeAttr = lv_svg_node_get_or_create_attr_transform(node);
        }
        if (nodeAttr != NULL) {
            const float fraction = lv_svg_node_animate_get_fraction(animate_node, time_ms);

            if (attrType_attr->value.ival == LV_SVG_TRANSFORM_TYPE_MATRIX)
            {
                float from[6] = {1, 0, 0, 1, 0, 0};
                float to[6] = {1, 0, 0, 1, 0, 0};

                if (lv_svg_node_animate_attr_values(animate_node, LV_SVG_ATTR_FROM, from, 6) &&
                    lv_svg_node_animate_attr_values(animate_node, LV_SVG_ATTR_TO, to, 6)) {
                    lv_svg_matrix_t matrix = {.m = 
                    { { from[0] + (fraction * (to[0] - from[0])),
                        from[2] + (fraction * (to[2] - from[2])),
                        from[4] + (fraction * (to[4] - from[4])) },
                      { from[1] + (fraction * (to[1] - from[1])),
                        from[3] + (fraction * (to[3] - from[3])),
                        from[5] + (fraction * (to[5] - from[5])) },
                      {0.0f, 0.0f, 1.0f}}};
                    lv_svg_matrix_t* matrix_ = (lv_svg_matrix_t*)nodeAttr->value.val;
                    lv_memcpy(matrix_, &matrix, sizeof(lv_svg_matrix_t));
                }
            }
            else if (attrType_attr->value.ival == LV_SVG_TRANSFORM_TYPE_ROTATE)
            {
                // Asume _process_length_value(...) values
                // These are of type float: nodeAttr->value.fval
                float from[3] = {0, 0, 0};
                float to[3] = {0, 0, 0};

                if (lv_svg_node_animate_attr_values(animate_node, LV_SVG_ATTR_FROM, from, 3) &&
                    lv_svg_node_animate_attr_values(animate_node, LV_SVG_ATTR_TO, to, 3)) {

                    if (nodeAttr->val_type == LV_SVG_ATTR_VALUE_PTR) {
                        //LV_SVG_TRANSFORM_TYPE_ROTATE

                        // Identity
                        lv_svg_matrix_t matrix = {.m = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

                        float degree = from[0] + (fraction * (to[0] - from[0]));
                        float cx     = from[1] + (fraction * (to[1] - from[1]));
                        float cy     = from[2] + (fraction * (to[2] - from[2]));

                        float radian = degree / 180.0f * (float)M_PI;
                        float cos_r = cosf(radian);
                        float sin_r = sinf(radian);

                        lv_svg_matrix_t rtm = {{
                                {cos_r, -sin_r, 0.0f},
                                {sin_r, cos_r, 0.0f},
                                {0.0f, 0.0f, 1.0f},
                            }
                        };
                        lv_svg_matrix_t tlm = {{
                                {1.0f, 0.0f, cx},
                                {0.0f, 1.0f, cy},
                                {0.0f, 0.0f, 1.0f},
                            }
                        };
                        lv_svg_matrix_multiply(&matrix, &tlm);
                        lv_svg_matrix_multiply(&matrix, &rtm);
                        tlm.m[0][2] = -cx;
                        tlm.m[1][2] = -cy;
                        lv_svg_matrix_multiply(&matrix, &tlm);

                        lv_svg_matrix_t* matrix_ = (lv_svg_matrix_t*)nodeAttr->value.val;
                        lv_memcpy(matrix_, &matrix, sizeof(lv_svg_matrix_t));
                    }
                }
            }
        }      
    }
}

static void lv_svg_node_animate_motion_node_step(lv_svg_node_t* node, lv_svg_node_t* animate_node, float time_ms)
{
    // use LV_SVG_ATTR_TRANSFORM to move the object
    lv_svg_attr_t* nodeAttr = lv_svg_node_get_or_create_attr_transform(node);
    if (nodeAttr != NULL) {
        const float fraction = lv_svg_node_animate_get_fraction(animate_node, time_ms);

        lv_svg_point_t point = {.x =0.0F,.y =0.0F};
        lv_svg_node_animate_attr_path(animate_node, fraction, &point);

        if (nodeAttr->val_type == LV_SVG_ATTR_VALUE_PTR) {
    
            //LV_SVG_TRANSFORM_TYPE_TRANSLATE

            lv_svg_matrix_t matrix = {{
                    {1.0f, 0.0f, point.x},
                    {0.0f, 1.0f, point.y},
                    {0.0f, 0.0f, 1.0f},
                }
            };

            lv_svg_matrix_t* matrix_ = (lv_svg_matrix_t*)nodeAttr->value.val;
            lv_memcpy(matrix_, &matrix, sizeof(lv_svg_matrix_t));
        }      
    }
}


void lv_svg_node_animate_step(lv_svg_node_t* node, float time_ms)
{
    lv_tree_node_t* tree_node = (lv_tree_node_t*)node;
    for (uint32_t i = 0; i < tree_node->child_cnt; i++) {
        lv_svg_node_t* child_node = (lv_svg_node_t*)tree_node->children[i];
        if (child_node->type == LV_SVG_TAG_ANIMATE) {
            lv_svg_node_animate_node_step(node, child_node, time_ms);
        }
        else if (child_node->type == LV_SVG_TAG_ANIMATE_TRANSFORM) {
            lv_svg_node_animate_transform_node_step(node, child_node, time_ms);
        }
        else if (child_node->type == LV_SVG_TAG_ANIMATE_MOTION) {
            lv_svg_node_animate_motion_node_step(node, child_node, time_ms);
        }
        else {
            lv_svg_node_animate_step(child_node, time_ms);
        }
    }
}

bool lv_svg_node_has_animation(lv_svg_node_t* node)
{
    bool has_animation = false;
    lv_tree_node_t* tree_node = (lv_tree_node_t*)node;
    uint32_t i = 0;
    while ((!has_animation) && (i < tree_node->child_cnt)) {
        lv_svg_node_t* child_node = (lv_svg_node_t*)tree_node->children[i];
        has_animation = ((child_node->type == LV_SVG_TAG_ANIMATE) ||
                         (child_node->type == LV_SVG_TAG_ANIMATE_TRANSFORM) ||
                         (child_node->type == LV_SVG_TAG_ANIMATE_MOTION) ||     
                         lv_svg_node_has_animation(child_node));
        i++;
    }
    return has_animation;
}



/**********************
 *   STATIC FUNCTIONS
 **********************/
#endif /*LV_USE_SVG*/
