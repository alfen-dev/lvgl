#include "../../lv_examples.h"

#if LV_BUILD_EXAMPLES
#if LV_USE_SVG

#include "../../../libs/svg/lv_svg_direct.h"
#include <string.h>

#define W 200
#define H 200
// equals PIXEL_TYPE_SIZE:
#define BYTES_PER_PIXEL 2
uint8_t tvgBuffer[W * H * BYTES_PER_PIXEL];
const uint8_t tvgBufferSize_bytes = W * H * BYTES_PER_PIXEL;

#define SVG_1 1
#define SVG_2 0


void lv_example_lvgl_svg_direct_1(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

		

#if SVG_1
    lv_obj_t* obj = lv_svg_direct_create(lv_screen_active());
#endif
#if SVG_2
    lv_obj_t* obj2 = lv_svg_direct_create(lv_screen_active());
#endif

static const char svg_data[] = 
"<svg viewBox=\"-150 -150 300 300\" width=\"100\" height=\"100\" >"
" <path fill=\"red\" d="
"\"M 125 60 L 60 125 H -60 L -125 60 V -60 L -60 -125 H 60 L 125 -60 V 60 Z\""
" />"
"<path stroke=\"white\" stroke-width=\"40\" d="
"\"m 65 -65 L -65 65 M -65 -65 L 65 65\""
"  >"
" </path>"
"</svg>";

static const char svg_data2[] = 
"<svg viewBox=\"-150 -150 300 300\">"
" <path fill=\"green\" d="
"\"M 125 60 L 60 125 H -60 L -125 60 V -60 L -60 -125 H 60 L 125 -60 V 60 Z\""
" />"
"<path stroke=\"white\" stroke-width=\"40\" d="
"\"m 65 -65 L -65 65 M -65 -65 L 65 65\""
"  >"
" </path>"
"</svg>";

#if SVG_1
    lv_svg_direct_set_src_data(obj, svg_data, sizeof(svg_data) / sizeof(char), (lv_point_t){400, 400}, true);
    lv_obj_set_size(obj, 300, 300);
#endif    
#if SVG_2
    lv_svg_direct_set_src_data(obj2, svg_data2, sizeof(svg_data2) / sizeof(char), (lv_point_t){300, 300}, true);
    lv_obj_set_size(obj2, 200, 200);
#endif    
    // bug: left is clipped at twice the position reducing the surface width so
    //     50->100 : width 150 (shows 3/4 of object of 200 width)
    //    100->200 : width 100 (shows 1/2 of object of 200 width)
#if SVG_1
    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 150, 100);
#endif    
#if SVG_2
    lv_obj_align(obj2, LV_ALIGN_TOP_LEFT, 250, 200);
#endif    

    lv_obj_invalidate(lv_screen_active());
}


#else

void lv_example_lvgl_svg_direct_1(void)
{
    /*fallback for online examples*/

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "SVG cannot be previewed online");
    lv_obj_center(label);
}

#endif /*SVG*/





#endif /*LV_BUILD_EXAMPLES*/
