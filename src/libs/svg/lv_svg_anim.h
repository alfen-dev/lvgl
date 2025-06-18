/**
 * @file lv_svg_anim.h
 *
 */

#ifndef LV_SVG_ANIM_H
#define LV_SVG_ANIM_H

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"
#if LV_USE_SVG
#if LV_USE_SVG_ANIMATION

#include "lv_svg.h"

#include "../../misc/lv_array.h"
#include "../../misc/lv_tree.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_svg_node_animate_step(lv_svg_node_t* node, float time_ms);
bool lv_svg_node_has_animation(lv_svg_node_t* node);


/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_SVG_ANIMATION */
#endif /*LV_USE_SVG*/

#endif /*LV_SVG_H*/
