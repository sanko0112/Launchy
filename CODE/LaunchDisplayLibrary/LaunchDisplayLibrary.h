#ifndef LaunchDisplayLibrary_H
#define LaunchDisplayLibrary_H

#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Declare u8g2 as external (only declare, not define)
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern char countdown_menu_var[5];
extern char preheat_duration_menu_var[5];
extern char preheat_start_menu_var[5];
extern char ignition_delta_menu_var[5];

extern char armed_ps[5];
extern char armed_pd[5];
extern char armed_t_[5];
extern char armed_id[5];

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes

void draw_welcome(void);
void draw_loading(void);
void draw_auth_a(void);
void draw_auth_b(void);
void draw_auth_c(void);
void draw_auth_d(void);
void draw_auth_e(void);
void draw_auth_f(void);
void draw_auth_g(void);
void draw_auth_success(void);
void draw_armed(void);
void draw_launch(void);
void draw_launch_preheat(void);
void draw_ignition(void);
void loading(void);
void countdown(uint8_t T_sec, uint8_t P_STime, uint8_t P_DTime);
void draw_countdown_menu(void);
void draw_preheat_start_menu(void);
void draw_preheat_duration_menu(void);
void draw_ignition_delta_menu(void);
void draw_continuity_check_start(void);
void draw_continuity_check_success(void);
void draw_continuity_check_failed(void);
void draw_batt_s(void);
void draw_batt_ss(void);

#ifdef __cplusplus
}
#endif

#endif
