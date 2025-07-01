/**
 * @file lv_svg_direct.h
 *
 */

#ifndef LV_SVG_DIRECT_H
#define LV_SVG_DIRECT_H

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
#if LV_USE_SVG

#include "../../misc/lv_array.h"
#include "../../misc/lv_tree.h"
/*********************
 *      DEFINES
 *********************/



#include "lv_svg.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_svg_direct_t lv_svg_direct_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Make the base object's class publicly available.
 */
LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_svg_direct_class;

lv_obj_t * lv_svg_direct_create(lv_obj_t * parent);
void lv_svg_direct_set_src_data(lv_obj_t* obj, const char * svg_data, uint32_t svg_data_len, const lv_point_t widget_size, const bool only_smaller);

lv_point_t lv_svg_direct_set_size(lv_obj_t* obj, const lv_point_t widget_size, const bool only_smaller);


#endif /*LV_USE_SVG*/

#endif /*LV_SVG_DIRECT_H*/
