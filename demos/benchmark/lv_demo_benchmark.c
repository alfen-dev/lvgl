/**
 * @file lv_demo_benchmark.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_benchmark.h"
#include "../widgets/lv_demo_widgets.h"

#if LV_USE_DEMO_BENCHMARK

#if LV_FONT_MONTSERRAT_14 == 0 && LV_DEMO_BENCHMARK_ALIGNED_FONTS == 0
    #error "LV_FONT_MONTSERRAT_14 or LV_DEMO_BENCHMARK_ALIGNED_FONTS is required for lv_demo_benchmark. Enable it in lv_conf.h."
#endif

#if LV_FONT_MONTSERRAT_20 == 0 && LV_DEMO_BENCHMARK_ALIGNED_FONTS == 0
    #error "LV_FONT_MONTSERRAT_20 or LV_DEMO_BENCHMARK_ALIGNED_FONTS is required for lv_demo_benchmark. Enable it in lv_conf.h."
#endif

#if LV_FONT_MONTSERRAT_24 == 0 && LV_DEMO_BENCHMARK_ALIGNED_FONTS == 0
    #error "LV_FONT_MONTSERRAT_24 of LV_DEMO_BENCHMARK_ALIGNED_FONTS is required for lv_demo_benchmark. Enable it in lv_conf.h."
#endif

#if LV_FONT_MONTSERRAT_26 == 0 && LV_DEMO_BENCHMARK_ALIGNED_FONTS == 0
    #error "LV_FONT_MONTSERRAT_26 or LV_DEMO_BENCHMARK_ALIGNED_FONTS is required for lv_demo_benchmark. Enable it in lv_conf.h."
#endif

#if LV_USE_DEMO_WIDGETS == 0
    #error "LV_USE_DEMO_WIDGETS needs to be enabled"
#endif

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN && LV_MEM_SIZE < 128 * 1024
    #warning "It's recommended to have at least 128kB RAM for the benchmark"
#endif

#include "../../lvgl_private.h"

/**********************
 *      DEFINES
 **********************/
#if LV_USE_PERF_MONITOR_LOG_MODE == 1
    #define HEADER_HEIGHT   0
#else
    #define HEADER_HEIGHT   48
#endif
#define FALL_HEIGHT     80
#define PAD_BASIC       8

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/

static void load_scene(uint32_t scene);
static void next_scene_timer_cb(lv_timer_t * timer);

#if LV_USE_PERF_MONITOR
    static void sysmon_perf_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif


static void summary_create(lv_demo_benchmark_summary_t * summary);


static void table_draw_task_event_cb(lv_event_t * e);
static void rnd_reset(void);
static int32_t rnd_next(int32_t min, int32_t max);
static lv_color_t rnd_color(void);
static void shake_anim_y_cb(void * var, int32_t v);
static void fall_anim(lv_obj_t * obj, int32_t y_max);
static void scroll_anim(lv_obj_t * obj, int32_t y_max);
static void scroll_anim_y_cb(void * var, int32_t v);
static void color_anim_cb(void * var, int32_t v);
static void color_anim(lv_obj_t * obj);
static void arc_anim(lv_obj_t * obj);

static lv_obj_t * card_create(void);

static void svg_cb(void);

extern void orb_waves_cb(void);
extern void orb_waves_end_cb(void);

static void empty_screen_cb(void)
{
    color_anim(lv_screen_active());
}

static void moving_wallpaper_cb(void)
{
    lv_obj_set_style_pad_all(lv_screen_active(), 0, 0);
    LV_IMAGE_DECLARE(img_benchmark_lvgl_logo_rgb);

    lv_obj_t * img = lv_image_create(lv_screen_active());
    lv_obj_set_size(img, lv_pct(150), lv_pct(150));
    lv_image_set_src(img, &img_benchmark_lvgl_logo_rgb);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_TILE);
    fall_anim(img, - lv_display_get_vertical_resolution(NULL) / 3);
}

static void single_rectangle_cb(void)
{
    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_center(obj);
    lv_obj_set_size(obj, lv_pct(30), lv_pct(30));

    color_anim(obj);

}

static void multiple_rectangles_cb(void)
{
    lv_obj_set_flex_flow(lv_screen_active(), LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(lv_screen_active(), LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);

    uint32_t i;
    for(i = 0; i < 9; i++) {
        lv_obj_t * obj = lv_obj_create(lv_screen_active());
        lv_obj_remove_style_all(obj);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_size(obj, lv_pct(25), lv_pct(25));

        color_anim(obj);
    }
}

static void multiple_rgb_images_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    LV_IMAGE_DECLARE(img_benchmark_lvgl_logo_rgb);
    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 160;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 160;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * obj = lv_image_create(lv_screen_active());
            lv_image_set_src(obj, &img_benchmark_lvgl_logo_rgb);
            if(x == 0) lv_obj_add_flag(obj, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

            fall_anim(obj, 80);
        }
    }
}

static void multiple_argb_images_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    LV_IMAGE_DECLARE(img_benchmark_lvgl_logo_argb);
    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 160;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 160;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * obj = lv_image_create(lv_screen_active());
            lv_image_set_src(obj, &img_benchmark_lvgl_logo_argb);
            if(x == 0) lv_obj_add_flag(obj, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

            fall_anim(obj, 80);
        }
    }
}

static void rotated_argb_image_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    LV_IMAGE_DECLARE(img_benchmark_lvgl_logo_argb);
    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 240;   /*240 instead of 160 to have less rotated images*/
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 240;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * obj = lv_image_create(lv_screen_active());
            lv_image_set_src(obj, &img_benchmark_lvgl_logo_argb);
            if(x == 0) lv_obj_add_flag(obj, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

            lv_image_set_rotation(obj, lv_rand(100, 3500));
            fall_anim(obj, 80);
        }
    }
}

static void multiple_labels_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);

    int32_t hor_res = lv_display_get_horizontal_resolution(NULL);
    int32_t ver_res = lv_display_get_vertical_resolution(NULL);

#if LV_DEMO_BENCHMARK_ALIGNED_FONTS
    if(hor_res * ver_res > 800 * 480) lv_obj_set_style_text_font(scr, &lv_font_benchmark_montserrat_26_aligned, 0);
    else if(hor_res * ver_res > 320 * 240) lv_obj_set_style_text_font(scr, &lv_font_benchmark_montserrat_20_aligned, 0);
    else lv_obj_set_style_text_font(scr, &lv_font_benchmark_montserrat_14_aligned, 0);
#else
    if(hor_res * ver_res > 800 * 480) lv_obj_set_style_text_font(scr, &lv_font_montserrat_26, 0);
    else if(hor_res * ver_res > 320 * 240) lv_obj_set_style_text_font(scr, &lv_font_montserrat_20, 0);
    else lv_obj_set_style_text_font(scr, &lv_font_montserrat_14, 0);
#endif

    lv_point_t s;
    lv_text_get_size(&s, "Hello LVGL!", lv_obj_get_style_text_font(scr, 0), 0, 0, LV_COORD_MAX,
                     LV_TEXT_FLAG_NONE);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / (s.x * 3 / 2);
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / (s.y  * 3);

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * obj = lv_label_create(lv_screen_active());
            if(x == 0) lv_obj_add_flag(obj, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            lv_label_set_text_static(obj, "Hello LVGL!");
            color_anim(obj);
        }
    }
}

static void screen_sized_text_cb(void)
{
    const char * txt =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque fringilla, lorem dapibus fringilla feugiat, justo arcu volutpat magna, vitae ultricies metus tortor nec est. Fusce ut tellus arcu. Fusce eu rutrum metus, nec porta felis. Sed sed ligula laoreet, sodales lacus blandit, elementum justo. Sed posuere quam ut pellentesque ullamcorper. In quis consequat magna. Etiam quis turpis nec lorem dictum finibus. Donec mattis enim dolor, consequat lacinia nisi scelerisque id. Nulla euismod, purus sit amet accumsan tempus, lorem lectus euismod dolor, sit amet facilisis nisl quam elementum nisi. Curabitur et massa eget lorem lacinia scelerisque eget vitae felis. Nulla facilisi.\n\n"
        "Vivamus auctor sit amet ante id rhoncus. Duis a dolor neque. Mauris eu ornare tortor. Vivamus consequat, ipsum a volutpat congue, sem libero laoreet nulla, malesuada efficitur leo orci a est. Donec tincidunt nulla nibh, quis pretium mi fermentum quis. Fusce a mattis libero. Curabitur in felis suscipit, ultrices diam imperdiet, vestibulum arcu. Praesent id faucibus turpis. Pellentesque sed massa tincidunt, interdum purus tempus, pellentesque risus. Fusce feugiat magna eget nisl eleifend efficitur. Mauris ut convallis justo. Integer malesuada rutrum orci non tincidunt.\n\n"
        "Nullam aliquet leo sit amet volutpat tincidunt. Mauris ac accumsan nibh. Morbi accumsan commodo leo, at hendrerit massa hendrerit et. Aliquam nec sodales ex. Morbi at aliquet sem. Sed at magna ut felis mollis dictum ut ac orci. Nunc id lorem lacus. Vivamus id accumsan dolor, sed suscipit nulla. Pellentesque dictum erat non bibendum tempor. Fusce arcu risus, eleifend in lacus a, iaculis fermentum sapien. Praesent sodales libero vitae massa suscipit tincidunt. Aliquam quis arcu urna. Nunc sit amet mi leo.\n\n"
        "Aliquam erat volutpat. Sed viverra pharetra ipsum, sed various arcu various nec. Curabitur rutrum odio et pretium fermentum. Maecenas vitae ligula nisi. Maecenas nec dapibus erat. Suspendisse vel accumsan erat. Proin congue diam at turpis accumsan eleifend.\n\n"
        "Etiam suscipit metus magna, in vulputate magna cursus eget. Donec vel rhoncus turpis. Phasellus vitae enim quis sapien pharetra aliquam quis a quam. In mauris nulla, euismod quis orci et, interdum finibus lorem. Aenean quis dolor eget est ultricies consectetur eu nec metus. Nullam at pulvinar elit. Aenean blandit faucibus sodales. Vivamus vel porta enim, et pharetra libero. Donec aliquet pretium erat viverra fermentum. Fusce sit amet porta mi. Nullam non elit ex. In luctus, nunc id iaculis ullamcorper, eros quam eleifend elit, quis dictum sem justo eu eros. Nulla vitae faucibus lectus. Nunc blandit, mi eget suscipit scelerisque, lorem nunc tincidunt tellus, eget gravida libero metus sed nunc.\n\n"
        "Morbi erat libero, commodo sit amet turpis eget, efficitur pulvinar dolor. Pellentesque vehicula, velit eget auctor scelerisque, sem risus aliquam lectus, sit amet dapibus massa ex non magna. Donec magna leo, laoreet quis erat vitae, consequat aliquet tellus. Etiam vitae lectus erat. Mauris interdum feugiat aliquet. Nunc justo augue, mattis id finibus eu, sagittis id enim. Vivamus malesuada mauris sed nibh luctus, porta bibendum quam ornare. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Vivamus malesuada magna nec diam tempus, laoreet imperdiet magna faucibus. Aliquam erat volutpat.\n\n"
        "Aenean mattis lobortis quam in venenatis. Sed euismod convallis lectus vel euismod. Vestibulum consequat luctus neque. Quisque consequat bibendum neque eget mollis. Vivamus viverra vehicula eros vel dapibus. Nullam id lectus aliquam, sagittis mi efficitur, interdum mauris. Nunc at felis lobortis, lobortis erat a, euismod augue. In id purus malesuada, tempus magna at, porta mi. Sed tristique nunc eget placerat luctus. Pellentesque posuere non purus vitae malesuada. Curabitur hendrerit dolor metus, nec posuere orci placerat ac.\n\n";

    lv_obj_t * scr = lv_screen_active();
    int32_t hor_res = lv_display_get_horizontal_resolution(NULL);
    int32_t ver_res = lv_display_get_vertical_resolution(NULL);

#if LV_DEMO_BENCHMARK_ALIGNED_FONTS
    if(hor_res * ver_res > 800 * 480) lv_obj_set_style_text_font(scr, &lv_font_benchmark_montserrat_26_aligned, 0);
    else if(hor_res * ver_res > 320 * 240) lv_obj_set_style_text_font(scr, &lv_font_benchmark_montserrat_20_aligned, 0);
    else lv_obj_set_style_text_font(scr, &lv_font_benchmark_montserrat_14_aligned, 0);
#else
    if(hor_res * ver_res > 800 * 480) lv_obj_set_style_text_font(scr, &lv_font_montserrat_26, 0);
    else if(hor_res * ver_res > 320 * 240) lv_obj_set_style_text_font(scr, &lv_font_montserrat_20, 0);
    else lv_obj_set_style_text_font(scr, &lv_font_montserrat_14, 0);
#endif

    lv_obj_t * obj = lv_label_create(scr);
    lv_obj_set_width(obj, lv_pct(100));
    lv_label_set_text_static(obj, txt);

    lv_obj_update_layout(obj);

    scroll_anim(scr, lv_obj_get_scroll_bottom(scr));
}

static void multiple_arcs_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);

    LV_IMAGE_DECLARE(img_benchmark_lvgl_logo_argb);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 160;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 160;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {

            lv_obj_t * obj = lv_arc_create(lv_screen_active());
            if(x == 0) lv_obj_add_flag(obj, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            lv_obj_set_size(obj, 100, 100);
            lv_obj_center(obj);

            lv_arc_set_bg_angles(obj, 0, 360);

            lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB);
            lv_obj_set_style_arc_width(obj, 10, LV_PART_INDICATOR);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(obj, rnd_color(), LV_PART_INDICATOR);
            arc_anim(obj);
        }
    }
}

static void containers_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 350;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 170;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * card = card_create();
            if(x == 0) lv_obj_add_flag(card, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            fall_anim(card, 30);
        }
    }
}

static void containers_with_overlay_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 350;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 170;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * card = card_create();
            if(x == 0) lv_obj_add_flag(card, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            fall_anim(card, 30);
        }
    }

    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
    color_anim(lv_layer_top());
}

static void containers_with_opa_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 350;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 170;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * card = card_create();
            if(x == 0) lv_obj_add_flag(card, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            lv_obj_set_style_opa(card, LV_OPA_50, 0);
            fall_anim(card, 30);
        }
    }
}

static void containers_with_opa_layer_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_pad_bottom(scr, FALL_HEIGHT + PAD_BASIC, 0);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 350;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / 170;

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * card = card_create();
            lv_obj_set_style_opa_layered(card, LV_OPA_50, 0);
            if(x == 0) lv_obj_add_flag(card, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            fall_anim(card, 30);
        }
    }
}

static void containers_with_scrolling_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 32, LV_PART_MAIN);

    int32_t hor_cnt = ((int32_t)lv_obj_get_content_width(scr)) / 400;
    int32_t ver_cnt = ((int32_t)lv_obj_get_content_height(scr)) / (120 + 32);

    if(hor_cnt < 1) hor_cnt = 1;
    if(ver_cnt < 1) ver_cnt = 1;

    ver_cnt *= 2; /*To make it scroll*/
    if(ver_cnt < 20) ver_cnt = 20; /*The test with many widgets*/

    int32_t y;
    for(y = 0; y < ver_cnt; y++) {
        int32_t x;
        for(x = 0; x < hor_cnt; x++) {
            lv_obj_t * card = card_create();
            if(x == 0) lv_obj_add_flag(card, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        }
    }

    lv_obj_update_layout(scr);
    scroll_anim(scr, lv_obj_get_scroll_bottom(scr));
}

static void widgets_demo_cb(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_pad_hor(scr, 0, 0);
    lv_obj_set_style_pad_bottom(scr, 0, 0);
    lv_demo_widgets();
    lv_demo_widgets_start_slideshow();

}

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_demo_benchmark_scene_dsc_t scenes[] = {
    {.name = "Waves Orb",                  .scene_time =  3000, .create_cb = orb_waves_cb                , .destruct_cb = orb_waves_end_cb},
    {.name = "Animated SVG",               .scene_time =  3000, .create_cb = svg_cb                      , .destruct_cb = NULL},
    {.name = "Empty screen",               .scene_time =  3000, .create_cb = empty_screen_cb             , .destruct_cb = NULL},
    {.name = "Moving wallpaper",           .scene_time =  3000, .create_cb = moving_wallpaper_cb         , .destruct_cb = NULL},
    {.name = "Single rectangle",           .scene_time =  3000, .create_cb = single_rectangle_cb         , .destruct_cb = NULL},
    {.name = "Multiple rectangles",        .scene_time =  3000, .create_cb = multiple_rectangles_cb      , .destruct_cb = NULL},
    {.name = "Multiple RGB images",        .scene_time =  3000, .create_cb = multiple_rgb_images_cb      , .destruct_cb = NULL},
    {.name = "Multiple ARGB images",       .scene_time =  3000, .create_cb = multiple_argb_images_cb     , .destruct_cb = NULL},
    {.name = "Rotated ARGB images",        .scene_time =  3000, .create_cb = rotated_argb_image_cb       , .destruct_cb = NULL},
    {.name = "Multiple labels",            .scene_time =  3000, .create_cb = multiple_labels_cb          , .destruct_cb = NULL},
    {.name = "Screen sized text",          .scene_time =  5000, .create_cb = screen_sized_text_cb        , .destruct_cb = NULL},
    {.name = "Multiple arcs",              .scene_time =  3000, .create_cb = multiple_arcs_cb            , .destruct_cb = NULL},

    {.name = "Containers",                 .scene_time =  3000, .create_cb = containers_cb               , .destruct_cb = NULL},
    {.name = "Containers with overlay",    .scene_time =  3000, .create_cb = containers_with_overlay_cb  , .destruct_cb = NULL},
    {.name = "Containers with opa",        .scene_time =  3000, .create_cb = containers_with_opa_cb      , .destruct_cb = NULL},
    {.name = "Containers with opa_layer",  .scene_time =  3000, .create_cb = containers_with_opa_layer_cb, .destruct_cb = NULL},
    {.name = "Containers with scrolling",  .scene_time =  5000, .create_cb = containers_with_scrolling_cb, .destruct_cb = NULL},

    {.name = "Widgets demo",               .scene_time = 20000, .create_cb = widgets_demo_cb             , .destruct_cb = NULL},

    {.name = "", .create_cb = NULL, .destruct_cb = NULL}
};

static uint32_t scene_act;
static uint32_t rnd_act;
static lv_demo_benchmark_on_end_cb_t on_demo_end_cb;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_demo_benchmark(void)
{
    scene_act = 0;

    lv_obj_t * scr = lv_screen_active();
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_color(scr, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_pad_all(lv_screen_active(), 8, 0);
    lv_obj_set_style_pad_top(lv_screen_active(), HEADER_HEIGHT, 0);
    lv_obj_set_style_pad_gap(lv_screen_active(), 8, 0);

    lv_obj_t * title = lv_label_create(lv_layer_top());
    lv_obj_set_style_bg_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_set_width(title, lv_pct(100));

    load_scene(scene_act);

    lv_timer_create(next_scene_timer_cb, scenes[0].scene_time, NULL);

#if LV_USE_PERF_MONITOR
    lv_display_t * disp = lv_display_get_default();
    lv_subject_add_observer_obj(&disp->perf_sysmon_backend.subject, sysmon_perf_observer_cb, title, NULL);
#if LV_USE_PERF_MONITOR_LOG_MODE
    lv_obj_add_flag(title, LV_OBJ_FLAG_HIDDEN);
#endif
#else
    lv_label_set_text(title, "LV_USE_PERF_MONITOR is not enabled");
#endif
}

void lv_demo_benchmark_set_end_cb(lv_demo_benchmark_on_end_cb_t cb)
{
    on_demo_end_cb = cb;
}

void lv_demo_benchmark_summary_display(const lv_demo_benchmark_summary_t * summary)
{
    LV_ASSERT_NULL(summary)
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_pad_hor(lv_screen_active(), 0, 0);
    lv_obj_t * table = lv_table_create(lv_screen_active());
    lv_obj_set_width(table, lv_pct(100));
    lv_obj_set_style_max_height(table, lv_pct(100), 0);
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_set_style_text_color(table, lv_palette_darken(LV_PALETTE_BLUE_GREY, 2), LV_PART_ITEMS);
    lv_obj_set_style_border_color(table, lv_palette_darken(LV_PALETTE_BLUE_GREY, 2), LV_PART_ITEMS);
    lv_obj_add_event_cb(table, table_draw_task_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);

    lv_table_set_cell_value(table, 0, 0, "Name");
    lv_table_set_cell_value(table, 0, 1, "Avg. CPU");
    lv_table_set_cell_value(table, 0, 2, "Avg. FPS");
    lv_table_set_cell_value(table, 0, 3, "Avg. time (render + flush)");
    /* csv log */
    LV_LOG("Benchmark Summary (%d.%d.%d %s)\r\n",
           LVGL_VERSION_MAJOR,
           LVGL_VERSION_MINOR,
           LVGL_VERSION_PATCH,
           LVGL_VERSION_INFO);
    LV_LOG("Name, Avg. CPU, Avg. FPS, Avg. time, render time, flush time\r\n");

    lv_obj_update_layout(table);
    const int32_t col_w = lv_obj_get_content_width(table) / 4;

    lv_table_set_column_width(table, 0, col_w);
    lv_table_set_column_width(table, 1, col_w);
    lv_table_set_column_width(table, 2, col_w);
    lv_table_set_column_width(table, 3, col_w);

    for(size_t i = 0; scenes[i].create_cb; i++) {
        lv_table_set_cell_value(table, i + 2, 0, scenes[i].name);

        if(scenes[i].measurement_cnt == 0) {
            lv_table_set_cell_value(table, i + 2, 1, "N/A");
            lv_table_set_cell_value(table, i + 2, 2, "N/A");
            lv_table_set_cell_value(table, i + 2, 3, "N/A");
        }
        else {
            const int32_t cnt = scenes[i].measurement_cnt;
            lv_table_set_cell_value_fmt(table, i + 2, 1, "%"LV_PRIu32" %%", scenes[i].cpu_avg_usage / cnt);
            lv_table_set_cell_value_fmt(table, i + 2, 2, "%"LV_PRIu32" FPS", scenes[i].fps_avg / cnt);

            const uint32_t render_time = scenes[i].render_avg_time / cnt;
            const uint32_t flush_time = scenes[i].flush_avg_time / cnt;
            const uint32_t total_time =  render_time + flush_time;
            lv_table_set_cell_value_fmt(table, i + 2, 3, "%"LV_PRIu32" ms (%"LV_PRIu32" + %"LV_PRIu32")",
                                        total_time, render_time, flush_time);

            /* csv log */
            LV_LOG("%s, %"LV_PRIu32"%%, %"LV_PRIu32", %"LV_PRIu32", %"LV_PRIu32", %"LV_PRIu32"\r\n",
                   scenes[i].name,
                   scenes[i].cpu_avg_usage / cnt,
                   scenes[i].fps_avg / cnt,
                   render_time + flush_time,
                   render_time,
                   flush_time);
        }
    }

    lv_table_set_cell_value(table, 1, 0, "All scenes avg.");
    if(summary->valid_scene_cnt < 1) {
        lv_table_set_cell_value(table, 1, 1, "N/A");
        lv_table_set_cell_value(table, 1, 2, "N/A");
        lv_table_set_cell_value(table, 1, 3, "N/A");
    }
    else {
        lv_table_set_cell_value_fmt(table, 1, 1, "%"LV_PRIu32" %%", summary->total_avg_cpu / summary->valid_scene_cnt);
        lv_table_set_cell_value_fmt(table, 1, 2, "%"LV_PRIu32" FPS", summary->total_avg_fps / summary->valid_scene_cnt);

        const uint32_t render_time = summary->total_avg_render_time / summary->valid_scene_cnt;
        const uint32_t flush_time = summary->total_avg_flush_time / summary->valid_scene_cnt;
        const uint32_t total_time = render_time + flush_time;
        lv_table_set_cell_value_fmt(table, 1, 3, "%"LV_PRIu32" ms (%"LV_PRIu32" + %"LV_PRIu32")",
                                    total_time, render_time, flush_time);
        /* csv log */
        LV_LOG("All scenes avg.,%"LV_PRIu32"%%, %"LV_PRIu32", %"LV_PRIu32", %"LV_PRIu32", %"LV_PRIu32"\r\n",
               summary->total_avg_cpu / summary->valid_scene_cnt,
               summary->total_avg_fps / summary->valid_scene_cnt,
               total_time,
               render_time, flush_time);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void load_scene(uint32_t scene)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_text_color(scr, lv_color_black(), 0);
    lv_obj_set_style_text_font(scr, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_pad_all(scr, PAD_BASIC, 0);
    lv_obj_set_style_pad_gap(scr, PAD_BASIC, 0);
    lv_obj_set_style_pad_top(scr, HEADER_HEIGHT, 0);
    lv_obj_set_layout(scr, LV_LAYOUT_NONE);
    lv_obj_set_flex_align(lv_screen_active(), LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_anim_delete(scr, scroll_anim_y_cb);
    lv_anim_delete(scr, shake_anim_y_cb);
    lv_anim_delete(scr, color_anim_cb);

    lv_anim_delete(lv_layer_top(), color_anim_cb);
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);

    rnd_reset();
    if(scenes[scene].create_cb) scenes[scene].create_cb();
}

static void unload_scene(uint32_t scene)
{
    if(scenes[scene].destruct_cb) scenes[scene].destruct_cb();
}

static void next_scene_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);

    unload_scene(scene_act);

    scene_act++;

    load_scene(scene_act);
    if(scenes[scene_act].scene_time == 0) {
        lv_demo_benchmark_summary_t summary;

        lv_timer_delete(timer);
        summary_create(&summary);
        /*
         * Don't display the summary if the user sets a callback function
         * He can always call this function himself inside the callback
         */
        if(on_demo_end_cb) {
            on_demo_end_cb(&summary);
        }
        else {
            lv_demo_benchmark_summary_display(&summary);
        }
    }
    else {
        lv_timer_set_period(timer, scenes[scene_act].scene_time);
    }
}

#if LV_USE_PERF_MONITOR
static void sysmon_perf_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    const lv_sysmon_perf_info_t * info = lv_subject_get_pointer(subject);
    char scene_name[64];

    if(scenes[scene_act].name[0] != '\0') {
        lv_snprintf(scene_name, sizeof(scene_name), "%s: ", scenes[scene_act].name);
    }
    else {
        scene_name[0] = '\0';
    }

#if !LV_USE_PERF_MONITOR_LOG_MODE
    lv_obj_t * label = lv_observer_get_target(observer);
    lv_label_set_text_fmt(label,
                          "%s"
                          "%" LV_PRIu32" FPS, %" LV_PRIu32 "%% CPU\n"
                          "refr. %" LV_PRIu32" ms = %" LV_PRIu32 "ms render + %" LV_PRIu32" ms flush",
                          scene_name,
                          info->calculated.fps, info->calculated.cpu,
                          info->calculated.render_avg_time + info->calculated.flush_avg_time,
                          info->calculated.render_avg_time, info->calculated.flush_avg_time);
#else
    LV_UNUSED(observer);
#endif

    /*Ignore the first call as it contains data from the previous scene*/
    if(scenes[scene_act].measurement_cnt != 0) {
        scenes[scene_act].cpu_avg_usage += info->calculated.cpu;
        scenes[scene_act].fps_avg += info->calculated.fps;
        scenes[scene_act].render_avg_time += info->calculated.render_avg_time;
        scenes[scene_act].flush_avg_time += info->calculated.flush_avg_time;
    }
    scenes[scene_act].measurement_cnt++;

}
#endif

static void table_draw_task_event_cb(lv_event_t * e)
{
    lv_draw_task_t * t = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * draw_dsc_base = t->draw_dsc;
    if(draw_dsc_base->part != LV_PART_ITEMS) return;

    int32_t row = draw_dsc_base->id1;
    if(row == 0) {
        lv_draw_fill_dsc_t * draw_dsc_fill = lv_draw_task_get_fill_dsc(t);
        if(draw_dsc_fill) {
            draw_dsc_fill->color = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);
        }
        lv_draw_label_dsc_t * draw_dsc_label = lv_draw_task_get_label_dsc(t);
        if(draw_dsc_label) {
            draw_dsc_label->color = lv_color_white();
        }
    }
    else if(row == 1) {
        lv_draw_border_dsc_t * draw_dsc_border = lv_draw_task_get_border_dsc(t);
        if(draw_dsc_border) {
            draw_dsc_border->color = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);
            draw_dsc_border->width = 2;
            draw_dsc_border->side = LV_BORDER_SIDE_BOTTOM;
        }
        lv_draw_label_dsc_t * draw_dsc_label = lv_draw_task_get_label_dsc(t);
        if(draw_dsc_label) {
            draw_dsc_label->color = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);
        }
    }

}

static void summary_create(lv_demo_benchmark_summary_t * summary)
{
    lv_memset(summary, 0, sizeof(*summary));

    summary->scenes = scenes;

    for(size_t i = 0; scenes[i].create_cb; i++) {
        /*the first measurement was ignored as it contains data from the previous scene*/
        if(scenes[i].measurement_cnt > 1) {
            const int32_t cnt = --scenes[i].measurement_cnt;
            summary->valid_scene_cnt++;
            summary->total_avg_cpu += scenes[i].cpu_avg_usage / cnt;
            summary->total_avg_fps += scenes[i].fps_avg / cnt;
            summary->total_avg_render_time += scenes[i].render_avg_time / cnt;
            summary->total_avg_flush_time += scenes[i].flush_avg_time / cnt;
        }
    }
}



/*----------------
 * SCENE HELPERS
 *----------------*/

static void color_anim_cb(void * var, int32_t v)
{
    LV_UNUSED(v);
    lv_color_t c = rnd_color();
    lv_obj_set_style_bg_color(var, c, 0);
    lv_obj_set_style_text_color(var, rnd_color(), 0);
}

static void color_anim(lv_obj_t * obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, color_anim_cb);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, 100);      /*New value in each ms*/
    lv_anim_set_var(&a, obj);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void arc_anim_cb(void * var, int32_t v)
{
    lv_arc_set_value(var, v);
}

static void arc_anim(lv_obj_t * obj)
{
    uint32_t t1 = rnd_next(1000, 3000);
    uint32_t t2 = rnd_next(1000, 3000);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, arc_anim_cb);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, t1);
    lv_anim_set_reverse_duration(&a, t2);
    lv_anim_set_var(&a, obj);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void scroll_anim_y_cb(void * var, int32_t v)
{
    lv_obj_scroll_to_y(var, v, LV_ANIM_OFF);
}

static void scroll_anim(lv_obj_t * obj, int32_t y_max)
{
    uint32_t t = lv_anim_speed(lv_display_get_dpi(NULL));

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, scroll_anim_y_cb);
    lv_anim_set_values(&a, 0, y_max);
    lv_anim_set_duration(&a, t);
    lv_anim_set_reverse_duration(&a, t);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);

}
static void shake_anim_y_cb(void * var, int32_t v)
{
    lv_obj_set_style_translate_y(var, v, 0);
}

static void fall_anim(lv_obj_t * obj, int32_t y_max)
{
    uint32_t t1 = rnd_next(300, 3000);
    uint32_t t2 = rnd_next(300, 3000);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, shake_anim_y_cb);
    lv_anim_set_values(&a, 0, y_max);
    lv_anim_set_duration(&a, t1);
    lv_anim_set_reverse_duration(&a, t2);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static lv_obj_t * card_create(void)
{
    lv_obj_t * panel = lv_obj_create(lv_screen_active());
    lv_obj_set_size(panel, 270, 120);
    lv_obj_set_style_pad_all(panel, 8, 0);

    LV_IMAGE_DECLARE(img_benchmark_avatar);
    lv_obj_t * child = lv_image_create(panel);
    lv_obj_align(child, LV_ALIGN_LEFT_MID, 0, 0);
    lv_image_set_src(child, &img_benchmark_avatar);

    child = lv_label_create(panel);
    lv_label_set_text_static(child, "John Smith");

#if LV_DEMO_BENCHMARK_ALIGNED_FONTS
    lv_obj_set_style_text_font(child, &lv_font_benchmark_montserrat_24_aligned, 0);
#else
    lv_obj_set_style_text_font(child, &lv_font_montserrat_24, 0);
#endif

    lv_obj_set_pos(child, 100, 0);

    child = lv_label_create(panel);
    lv_label_set_text_static(child, "A DIY enthusiast");

#if LV_DEMO_BENCHMARK_ALIGNED_FONTS
    lv_obj_set_style_text_font(child, &lv_font_benchmark_montserrat_14_aligned, 0);
#else
    lv_obj_set_style_text_font(child, &lv_font_montserrat_14, 0);
#endif

    lv_obj_set_pos(child, 100, 30);

    child = lv_button_create(panel);
    lv_obj_set_pos(child, 100, 50);

    child = lv_label_create(child);
    lv_label_set_text_static(child, "Connect");

    return panel;
}

static void rnd_reset(void)
{
    rnd_act = 0;
}

static int32_t rnd_next(int32_t min, int32_t max)
{
    static const uint32_t rnd_map[] = {
        0xbd13204f, 0x67d8167f, 0x20211c99, 0xb0a7cc05,
        0x06d5c703, 0xeafb01a7, 0xd0473b5c, 0xc999aaa2,
        0x86f9d5d9, 0x294bdb29, 0x12a3c207, 0x78914d14,
        0x10a30006, 0x6134c7db, 0x194443af, 0x142d1099,
        0x376292d5, 0x20f433c5, 0x074d2a59, 0x4e74c293,
        0x072a0810, 0xdd0f136d, 0x5cca6dbc, 0x623bfdd8,
        0xb645eb2f, 0xbe50894a, 0xc9b56717, 0xe0f912c8,
        0x4f6b5e24, 0xfe44b128, 0xe12d57a8, 0x9b15c9cc,
        0xab2ae1d3, 0xb4dc5074, 0x67d457c8, 0x8e46b00c,
        0xa29a1871, 0xcee40332, 0x80f93aa1, 0x85286096,
        0x09bd6b49, 0x95072088, 0x2093924b, 0x6a27328f,
        0xa796079b, 0xc3b488bc, 0xe29bcce0, 0x07048a4c,
        0x7d81bd99, 0x27aacb30, 0x44fc7a0e, 0xa2382241,
        0x8357a17d, 0x97e9c9cc, 0xad10ff52, 0x9923fc5c,
        0x8f2c840a, 0x20356ba2, 0x7997a677, 0x9a7f1800,
        0x35c7562b, 0xd901fe51, 0x8f4e053d, 0xa5b94923,
        0xd2c5eedd, 0x24f0cc9b, 0x3aa7b571, 0xd289a1c9,
        0x79c7dc3,  0x5bf68c86, 0xc9f55239, 0x42052cfb,
        0x63dae9df, 0x75c9e11f, 0x407f9151, 0x104ebc63,
        0xb4b52591, 0x53a46b7a, 0x9398d144, 0x9a7c6c3d,
        0x76b35b78, 0xa028e33e, 0xbfe586e4, 0xf3f79731,
        0x99591738, 0xd7b0a847, 0x1ffb1936, 0xfeeea2e4,
        0xbc896279, 0xa8241a72, 0x871124fa, 0x27bb9866,
        0x41794272, 0x92f5dc59, 0x98c9d185, 0x6fc5905b,
        0xf0ba9f1a, 0x771cad1b, 0xf6285752, 0xb5ffcbc5,
        0x6fd2b63c, 0x2c404190, 0x209469e6, 0x628531b1,
        0x98a726bc, 0xcc6c0d97, 0x86c2e7b9, 0x7bc12e1f,
        0xf9a67e10, 0xd5bf101f, 0xa1aaaf35, 0x69b078fc,
        0x71d698b2, 0x9a954baa, 0xe7423a82, 0xdd9898e1,
        0xf4980e5c, 0x4f3607b9, 0x9ce35d27, 0xb4b764e0,
        0xa1fa3ad3, 0x220ad165, 0x282216b4, 0x7e583888,
        0xf8315b2b, 0x81c27062, 0x8eb89a85,     /*Intentionally incomplete line to make the length of array more arbitrary*/

    };

    if(min == max) return min;

    if(min > max) {
        int32_t t = min;
        min = max;
        max = t;
    }

    int32_t d = max - min;
    int32_t r = (rnd_map[rnd_act] % d) + min;

    rnd_act++;
    if(rnd_act >= sizeof(rnd_map) / sizeof(rnd_map[0])) rnd_act = 0;

    return r;
}

static lv_color_t rnd_color(void)
{
    return lv_palette_main(rnd_next(0, LV_PALETTE_LAST - 1));
}

#if 0
"    <linearGradient id=\"background-3-grad\" x1=\"20%\" x2=\"80%\" y1=\"60%\" y2=\"30%\">"
"     <stop offset=\"0%\" stop-color=\"#c72f3b\" />"
"     <stop offset=\"50%\" stop-color=\"#fff\" />"
"     <stop offset=\"100%\" stop-color=\"#c72f3b\" />"
"   <animate"
"     attributeName=\"x1\""
"     begin=\"0s\""
"     dur=\"8s\""
"     from=\"-60%\""
"     to=\"100%\""
"     repeatCount=\"indefinite\" />"
"   <animate"
"     attributeName=\"x2\""
"     begin=\"0s\""
"     dur=\"8s\""
"     from=\"0%\""
"     to=\"160%\""
"     repeatCount=\"indefinite\" />"
"   </linearGradient>"
#endif

const char* svg =
"<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" id=\"background\" width=\"400\" height=\"300\" viewBox=\"0 0 800 600\">"
"  <defs>"
"    <linearGradient id=\"background-3-grad\" x1=\"0%\" x2=\"100%\" y1=\"60%\" y2=\"30%\">"
"     <stop offset=\"0%\" stop-color=\"#c72f3b\">"
"       <animate"
"         attributeName=\"offset\""
"         begin=\"0s\""
"         dur=\"3s\""
"         from=\"0%\""
"         to=\"80%\""
"         repeatCount=\"indefinite\" />"
"     </stop>"
"     <stop offset=\"50%\" stop-color=\"#fff\">"
"       <animate"
"         attributeName=\"offset\""
"         begin=\"0s\""
"         dur=\"8s\""
"         from=\"10%\""
"         to=\"90%\""
"         repeatCount=\"indefinite\" />"
"     </stop>"
"     <stop offset=\"100%\" stop-color=\"#c72f3b\">"
"       <animate"
"         attributeName=\"offset\""
"         begin=\"0s\""
"         dur=\"3s\""
"         from=\"20%\""
"         to=\"100%\""
"         repeatCount=\"indefinite\" />"
"     </stop>"
"   </linearGradient>"
"  </defs>"
"  <rect width=\"800\" height=\"600\" x=\"0\" y=\"0\" rx=\"0\" ry=\"0\" fill=\"#000022\"/>"
"  <g class=\"artwork\" id=\"artwork\" data-name=\"artwork\">"
"    <circle class=\"gun-charge\" id=\"charging-circle\" cx=\"568\" cy=\"300\" r=\"164\" style=\"fill: #4193f2\"/>"
"    <g id=\"beam\">"
"    </g>"
"    <g class=\"raygun\" id=\"raygun\">"
"      <path class=\"gun-trigger\" id=\"gun-trigger\" d=\"M463.91,390.15a14,14,0,1,0-18.07,16.12C452.5,434.44,466,439.76,466,439.76l-7.34-35.62A14,14,0,0,0,463.91,390.15Z\" style=\"fill: #86a4b7\"/>"
"      <path id=\"handle-grip\" d=\"M419,392s-17.61,46-9.75,84c0,0-23.3,1.33-29.26-19,0,0-2.17-43.33,19.51-67Z\" style=\"fill: #86a4b7\"/>"
"      <path id=\"handle\" d=\"M358.21,391.64c10.73,7.56,29.79,27.68,10.27,67.4a2,2,0,0,0,.08,1.93c2.42,3.88,13,18,38.07,16.32a2,2,0,0,0,.68-3.83c-13.23-5.83-33.71-24.49-2-82.49a2,2,0,0,0-1.75-3H359.35A2,2,0,0,0,358.21,391.64Z\" style=\"fill: #aa1f25\"/>"
"      <path id=\"fin-background\" d=\"M313.14,233.89l-31.65-47.7A4,4,0,0,1,282,180c14.71-10,61.33-24,139.76,8.16,3.87,1.59,2.4,7.55-1.79,7.57-23.91.07-68.2,5.2-101.26,38.11A4,4,0,0,1,313.14,233.89Z\" style=\"fill: #cab02d\"/>"
"      <g id=\"gun-tip\">"
"        <polygon id=\"barrel\" points=\"480 260 480 332 568 300 480 260\" style=\"fill: #fddb00\"/>"
"        <polygon id=\"barrel-shadow\" points=\"480 300 480 332 568 300 480 300\" style=\"fill: #cab02d\"/>"
"        <circle id=\"tip\" cx=\"568\" cy=\"300\" r=\"16\" style=\"fill: #c72f3b\"/>"
"        <circle id=\"tip--highlight\" cx=\"568\" cy=\"292\" r=\"4\" style=\"fill: #ef8c99\"/>"
"      </g>"
"      <g id=\"gun-background\">"
"        <path id=\"background-3\" data-name=\"background\" fill=\"url(#background-3-grad)\" d=\"M489,194.27C345,169.72,288,261,288,300c0,40.49,57,130.08,201,105.53,5.43-.93,10.42-1.8,15-2.64V196.7C499.42,195.86,494.43,195.19,489,194.27Z\"/>"
"        <rect id=\"background-shadow\" x=\"472\" y=\"340\" width=\"8\" height=\"48\" rx=\"4\" ry=\"4\" style=\"fill: #aa1f25\"/>"
"        <path d=\"M336.6 301.6C331.6 262.5 360 228.5 400 228.5S468 262.5 463.31 301.6C458.1 265.1 427 246.7 400 246.7S342 265.1 336.6 301.6Z\" style=\"fill: #aa1f25\"/>"
"        <path id=\"tailfin-shadow\" d=\"M302.55,342.75C315.47,332.66,337,331,337,331l-10-16-34.62,7.21A106.37,106.37,0,0,0,302.55,342.75Z\" style=\"fill: #aa1f25\"/>"
"        <path d=\"M326.1 250C362.6 210.9 413.5 198.9 463.4 203.5 470.7 204.7 467.4 213.4 462.9 212.5 410.4 203 356.3 219.6 326.1 250\" style=\"fill: #ef8c99\"/>"
"      </g>"
"      <g id=\"gun-metal\">"
"        <rect x=\"488\" y=\"188\" width=\"16\" height=\"224\" rx=\"4\" ry=\"4\" style=\"fill: #b0cce1\"/>"
"        <circle cx=\"496\" cy=\"196\" r=\"4\" style=\"fill: #fff\"/>"
"        <circle cx=\"496\" cy=\"403\" r=\"4\" style=\"fill: #86a4b7\"/>"
"        <rect x=\"492\" y=\"204\" width=\"8\" height=\"48\" rx=\"4\" ry=\"4\" style=\"fill: #fff\"/>"
"      </g>"
"      <g id=\"gun-detail\">"
"        <path style=\"fill: #fddb00\" d=\"M330.2 269.9C336.4 255 358 224 400 224S476 258 476 300 442 376 400 376 337.8 346.5 331 332C330.6 332 330.4 332 330 332L251 332C249 332.1 248.1 329.4 249.8 328.4L286.1 309.4C287.4 308.4 287 306.9 286.4 306.4L276.4 296.4C273.9 293.7 275.5 290.7 277.7 289.8L328.6 270.1C329.1 269.9 329.7 269.8 330.2 269.9ZM400 364C435 364 464 335 464 300S435 236 400 236 336 265 336 300 365 364 400 364Z\"/>"
"        <path d=\"M327.17,324H292a2,2,0,0,1-.85-3.81L324,306.12a2,2,0,0,1,2.72,1.11L329,321.3A2,2,0,0,1,327.17,324Z\" style=\"fill: #cab02d\"/>"
"        <g id=\"lightning-bolt\">"
"          <path d=\"M402,349.53a3.48,3.48,0,0,1-3.52-3.5V314a0.5,0.5,0,0,0-.5-0.5H375a3.5,3.5,0,0,1-3.24-4.83l23-55.89a3.43,3.43,0,0,1,3.21-2.17,3.48,3.48,0,0,1,3.52,3.5V286a0.5,0.5,0,0,0,.5.5h23a3.5,3.5,0,0,1,3.23,4.85l-23,56a3.43,3.43,0,0,1-3.21,2.16h0Z\" style=\"fill: #fff\"/>"
"          <path d=\"M398,252.11a2,2,0,0,1,2,2V286a2,2,0,0,0,2,2h23a2,2,0,0,1,1.85,2.77l-23,56A1.93,1.93,0,0,1,402,348a2,2,0,0,1-2-2V314a2,2,0,0,0-2-2H375a2,2,0,0,1-1.85-2.76l23-55.89a1.93,1.93,0,0,1,1.83-1.24m0-3a4.9,4.9,0,0,0-4.6,3.1l-23,55.89A5,5,0,0,0,375,315h22v31a5,5,0,0,0,5,5,4.91,4.91,0,0,0,4.59-3.07l23-56A5,5,0,0,0,425,285H403V254.11a5,5,0,0,0-5-5h0Z\" style=\"fill: #c72f3b\"/>"
"        </g>"
"      </g>"
"      <g id=\"fin-foreground\">"
"        <path d=\"M310.55,249.89l-34.59-33a2.91,2.91,0,0,1-.33-4.26c9.48-10.19,52.08-51.09,133.51-18.22a2.93,2.93,0,0,1-.6,5.63c-20.58,3.55-67.87,13.33-93.84,49.49A2.92,2.92,0,0,1,310.55,249.89Z\" style=\"fill: #fddb00\"/>"
"        <path d=\"M383,197s-6.06-1-15.21-1.44c-4.57-.24-9.92-0.34-15.62-0.13-2.85.09-5.79,0.3-8.77,0.56s-6,.65-9,1.15c-1.49.25-3,.5-4.45,0.82l-2.2.46-2.16.53c-2.87.7-5.64,1.58-8.28,2.5-1.31.47-2.58,1-3.83,1.49L310,204.59l-3.24,1.75-2.9,1.81L301.3,210c-0.81.57-1.48,1.23-2.16,1.79l-1,.83c-0.3.28-.57,0.57-0.84,0.84l-1.46,1.47c-0.85.92-1.44,1.7-1.88,2.21l-0.66.78-0.16.2a3,3,0,1,1-4.6-3.86l0.11-.13,0.81-.86c0.54-.55,1.27-1.4,2.3-2.39l1.76-1.57c0.32-.28.65-0.59,1-0.88l1.14-.86c0.8-.58,1.6-1.26,2.54-1.84s1.9-1.22,2.92-1.85l3.29-1.78,3.61-1.67,3.9-1.52c1.35-.43,2.72-0.92,4.14-1.33,2.83-.8,5.77-1.54,8.79-2.08l2.27-.41,2.29-.34c1.53-.24,3.07-0.4,4.61-0.56,3.08-.32,6.17-0.5,9.21-0.61s6-.11,8.91,0c5.77,0.14,11.13.57,15.7,1.09C377,195.66,383,197,383,197Z\" style=\"fill: #ccb32c\"/>"
"      </g>"
"    </g>"
"    <g id=\"shockwave\">"
"      <line class=\"line\" id=\"line-8\" x1=\"568\" y1=\"300\" x2=\"568\" y2=\"156\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-7\" x1=\"568\" y1=\"300\" x2=\"669.82\" y2=\"198.18\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-6\" x1=\"568\" y1=\"300\" x2=\"712\" y2=\"300\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-5\" x1=\"568\" y1=\"300\" x2=\"669.82\" y2=\"401.82\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-4\" x1=\"568\" y1=\"300\" x2=\"568\" y2=\"444\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-3\" x1=\"568\" y1=\"300\" x2=\"466.18\" y2=\"401.82\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-2\" x1=\"568\" y1=\"300\" x2=\"424\" y2=\"300\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"      <line class=\"line\" id=\"line-1\" x1=\"568\" y1=\"300\" x2=\"466.18\" y2=\"198.18\" style=\"fill: none;stroke: #ffffff;stroke-linecap: round;stroke-miterlimit: 10;stroke-width: 8px\"/>"
"    </g>"
"  </g>"
"</svg>";

#include "../../src/libs/svg/lv_svg_direct.h"
#include <string.h>

static void svg_cb(void)
{

    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* svg_obj = lv_svg_direct_create(scr);
    lv_svg_direct_set_src_data(svg_obj, svg, strlen(svg), (lv_point_t){0, 0}, true);

    // control will start animation!!!
}



#endif
