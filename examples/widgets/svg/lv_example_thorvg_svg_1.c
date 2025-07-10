#include "../../lv_examples.h"
#if LV_BUILD_EXAMPLES
#if LV_USE_SVG


void lv_example_thorvg_svg_1(void)  
{

    #define W 300
    #define H 300
    #define BYTES_PER_PIXEL 2
    static uint8_t tvgBuffer[W * H * BYTES_PER_PIXEL];
    static const uint32_t tvgBufferSize_bytes = W * H * BYTES_PER_PIXEL;
    
    lv_obj_t * lottie = lv_lottie_create(lv_screen_active());
    //lv_lottie_set_src_data(lottie, lv_example_lottie_approve, lv_example_lottie_approve_size);


    // static const char svg_data[] = "<svg width=\"4.166667in\" height=\"3.5in\" viewBox=\"0 0 400 336\">"
    //                          "<circle cx=\"200\" cy=\"168\" r=\"168\" fill=\"red\" stroke=\"blue\" stroke-width=\"10\"/></svg>";

    static const char svg_data[] = "<svg width=\"100\" height=\"100\">"
"<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" stroke=\"blue\" stroke-width=\"10\"/>"
"  <circle cx=\"50\" cy=\"50\" r=\"10\" style=\"fill:red;\">"
//"    <animate"
//"      attributeName=\"cx\""
//"      begin=\"0s\""
//"      dur=\"10s\""
//"      from=\"50\""
//"      to=\"10\""
//"      repeatCount=\"indefinite\" />"
"  </circle>"
"</svg>";

    static const size_t svg_data_size = sizeof(svg_data);



#if LV_DRAW_BUF_ALIGN == 4 && LV_DRAW_BUF_STRIDE_ALIGN == 1
    /*If there are no special requirements, just declare a buffer
      x4 because the Lottie is rendered in ARGB8888 format*/
    //static uint8_t buf[300 * 300 * 2];
    lv_lottie_set_buffer(lottie, W, H, tvgBuffer, LV_COLOR_FORMAT_RGB565);
#else
    /*For GPUs and special alignment/strid setting use a draw_buf instead*/
    LV_DRAW_BUF_DEFINE(draw_buf, W, H, LV_COLOR_FORMAT_ARGB8888);
    lv_lottie_set_draw_buf(lottie, &draw_buf);
#endif


    // This MUST after the lv_lottie_set_draw_buf !!! 
    lv_lottie_set_svg_src_data(lottie, svg_data, svg_data_size);

    lv_obj_center(lottie);
}


#else

void lv_example_thorvg_svg_1(void)
{
    /*fallback for online examples*/

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "ThorVG SVG cannot be previewed online");
    lv_obj_center(label);
}

#endif /*SVG*/





#endif /*LV_BUILD_EXAMPLES*/
