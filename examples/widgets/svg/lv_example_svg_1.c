#include "../../lv_examples.h"
#if LV_BUILD_EXAMPLES
#if LV_USE_SVG

#define W 200
#define H 200
// equals PIXEL_TYPE_SIZE:
#define BYTES_PER_PIXEL 2
uint8_t tvgBuffer[W * H * BYTES_PER_PIXEL];
const uint32_t tvgBufferSize_bytes = W * H * BYTES_PER_PIXEL;


void lv_example_lvgl_svg_1(void)
{
    lv_obj_t * obj = lv_svg_create(lv_screen_active());
    lv_svg_t* svg = (lv_svg_t*)obj;

    lv_svg_set_buffer(obj, W, H, tvgBuffer, tvgBufferSize_bytes, LV_COLOR_FORMAT_RGB565);

#if 0
    static const char svg_data[] = "<svg width=\"2.0833in\" height=\"2.0833in\" viewBox=\"0 0 200 200\">"
                             "<circle cx=\"100\" cy=\"100\" r=\"80\" fill=\"red\" stroke=\"blue\" stroke-width=\"10\"/></svg>";
#elif 0
// https://editsvgcode.com/
    static const char svg_data[] = 
"<svg width=\"200\" height=\"200\" viewBox=\"0 0 200 200\">"
"<rect x=\"10\" y=\"10\" width=\"180\" height=\"180\" fill=\"green\" stroke=\"blue\" stroke-width=\"10\"/>"
"  <ellipse cx=\"150\" cy=\"150\" rx=\"10\" ry=\"20\" fill=\"red\" stroke-width=\"0\">"
"    <animate"
"      attributeName=\"cx\""
"      begin=\"0s\""
"      dur=\"10s\""
"      from=\"150\""
"      to=\"10\""
"      repeatCount=\"indefinite\" />"
"    <animate"
"      attributeName=\"cy\""
"      begin=\"0s\""
"      dur=\"10s\""
"      from=\"150\""
"      to=\"130\""
"      repeatCount=\"indefinite\" />"
"  </ellipse>"
"  <rect x=\"30\" y=\"40\" height=\"30\" width=\"50\" stroke=\"yellow\" fill=\"red\" transform=\"rotate(30 55 55)\">"
//"  <rect x=\"30\" y=\"40\" height=\"30\" width=\"50\" stroke=\"yellow\" fill=\"red\" >"
"    <animateTransform"
"      attributeName=\"transform\""
"      begin=\"0s\""
"      dur=\"10s\""
"      type=\"rotate\""
"      from=\"0 55 55\""
"      to=\"360 55 55\""
"      repeatCount=\"indefinite\" />"
"  </rect>"
"</svg>"
;
#else
    static const char svg_data[] = 
"<svg viewBox=\"-150 -150 300 300\">"
" <path fill=\"red\" d="
"\"M 125 60 L 60 125 H -60 L -125 60 V -60 L -60 -125 H 60 L 125 -60 V 60 Z\""
" />"
"<path stroke=\"white\" stroke-width=\"40\" d="
"\"m 65 -65 L -65 65 M -65 -65 L 65 65\""
"  >"
"  <animateTransform"
"   attributeName=\"transform\""
"   begin=\"4s\""
"   dur=\"1s\""
"   type=\"rotate\""
"   from=\"0 0 0\""
"   to=\"90 0 0\""
"   repeatCount=\"indefinite\" />"
" </path>"
"</svg>"
;
#endif

    lv_svg_set_src_data(obj, svg_data, sizeof(svg_data) / sizeof(char), 200, 200);

    lv_obj_center(obj);
    //lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
}


#else

void lv_example_lvgl_svg_1(void)
{
    /*fallback for online examples*/

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "SVG cannot be previewed online");
    lv_obj_center(label);
}

#endif /*SVG*/





#endif /*LV_BUILD_EXAMPLES*/
