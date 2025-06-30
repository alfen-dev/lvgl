/**
 * @file lv_tvg_canvas.h
 *
 */

#ifndef LV_TVG_CANVAS_H
#define LV_TVG_CANVAS_H

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
#if LV_USE_VECTOR_GRAPHIC

#include "../../misc/lv_array.h"
#include "../../misc/lv_tree.h"
/*********************
 *      DEFINES
 *********************/



 #include "../../lv_conf_internal.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_tvg_canvas_t lv_tvg_canvas_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Make the base object's class publicly available.
 */
LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_tvg_canvas_class;

// .base_class = &lv_tvg_canvas_class,

//lv_vector_dsc_t* lv_tvg_canvas_prepare_draw(lv_obj_t* obj, lv_event_t* e);
//void lv_tvg_canvas_end_draw(lv_obj_t* obj, lv_event_t* e, lv_vector_dsc_t* dsc);



#endif /*LV_USE_VECTOR_GRAPHIC*/

#endif /*LV_TVG_CANVAS_H*/
