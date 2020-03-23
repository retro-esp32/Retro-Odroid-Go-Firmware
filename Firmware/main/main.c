//{#pragma region Includes
  #include "includes/core.h"
  #include "includes/definitions.h"
  #include "includes/structures.h"
  #include "includes/declarations.h"
//}#pragma endregion Includes

//{#pragma region Odroid
  static odroid_gamepad_state gamepad;
  odroid_battery_state battery_state;
//}#pragma endregion Odroid

//{#pragma region Global
  bool SAVED = false;
  bool RESTART = false;
  bool LAUNCHER = false;
  bool FOLDER = false;
  bool SPLASH = true;
  bool SETTINGS = false;
  bool SD = false;

  int8_t STEP = 0;
  int OPTION = 0;
  int PREVIOUS = 0;
  int32_t VOLUME = 0;
  int32_t BRIGHTNESS = 0;
  int32_t BRIGHTNESS_COUNT = 10;
  int32_t BRIGHTNESS_LEVELS[10] = {10,20,30,40,50,60,70,80,90,100};
  int8_t USER;
  int8_t SETTING;
  int8_t COLOR;
  uint32_t currentDuty;

  char** FILES;
  char folder_path[256] = "";

  DIR *directory;
  struct dirent *file;

  const char* HEADER_V00_01 = "ODROIDGO_FIRMWARE_V00_01";

  char FirmwareDescription[FIRMWARE_DESCRIPTION_SIZE];
//}#pragma endregion Global

//{#pragma region Emulator and Directories
  char FEATURES[COUNT][30] = {
    "SETTINGS",
    "FIRMWARE",
  };

  char DIRECTORIES[COUNT][10] = {
    "",
    "firmware",      // 1
  };

  char EXTENSIONS[COUNT][10] = {
    "",
    "fw",      // 1
  };

  //int PROGRAMS[COUNT] = {1, 2, 2, 3, 3, 3, 4, 5, 6, 7, 8, 9};
  int LIMIT = 6;
//}#pragma endregion Emulator and Directories

//{#pragma region Buffer
  unsigned short buffer[40000];
//}#pragma endregion Buffer

/*
  APPLICATION
*/
//{#pragma region Main
  void app_main(void)
  {
    nvs_flash_init();
    odroid_system_init();

    odroid_audio_init(16000);

    VOLUME = odroid_settings_Volume_get();
    odroid_settings_Volume_set(VOLUME);

    //odroid_settings_Backlight_set(BRIGHTNESS);

    // Display
    ili9341_init();
    BRIGHTNESS = get_brightness();
    apply_brightness();

    // Joystick
    odroid_input_gamepad_init();

    // Battery
    odroid_input_battery_level_init();

    // SD
    //odroid_sdcard_open("/sd");
    esp_err_t r = odroid_sdcard_open("/sd");
    SD = r != ESP_OK ? false : true;

    // Theme
    get_theme();
    // get_restore_states();

    // Toggle
    get_toggle();

    GUI = THEMES[USER];

    ili9341_prepare();
    ili9341_clear(0);

    //printf("==============\n%s\n==============\n", "RETRO ESP32");
    switch(esp_reset_reason()) {
      case ESP_RST_POWERON:
        RESTART = false;
        STEP = 1;
        ROMS.offset = 0;
      break;
      case ESP_RST_SW:
        RESTART = true;
        STEP = 1;
        ROMS.offset = 0;
      break;
      default:
        RESTART = false;
      break;
    }
    //STEP = 0;
    RESTART ? restart() : SPLASH ? splash() : NULL;
    draw_background();
    restore_layout();
    xTaskCreate(launcher, "launcher", 4096, NULL, 5, NULL);
  }
//}#pragma endregion Main

/*
  METHODS
*/
//{#pragma region Debounce
  void debounce(int key) {
    draw_battery();
    draw_speaker();
    draw_contrast();
    while (gamepad.values[key]) odroid_input_gamepad_read(&gamepad);
  }
//}#pragma endregion Debounce

//{#pragma region States
  void get_step_state() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "STEP", &STEP);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        STEP = 0;
        break;
      default :
        STEP = 0;
    }
    nvs_close(handle);
    //printf("\nGet nvs_get_i8:%d\n", STEP);
  }

  void set_step_state() {
    //printf("\nGet nvs_set_i8:%d\n", STEP);
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "STEP", STEP);
    nvs_commit(handle);
    nvs_close(handle);
  }

  void get_list_state() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);
    err = nvs_get_i16(handle, "LAST", &ROMS.offset);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ROMS.offset = 0;
        break;
      default :
        ROMS.offset = 0;
    }
    nvs_close(handle);
    //printf("\nGet nvs_get_i16:%d\n", ROMS.offset);
  }

  void set_list_state() {
    //printf("\nSet nvs_set_i16:%d", ROMS.offset);
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i16(handle, "LAST", ROMS.offset);
    nvs_commit(handle);
    nvs_close(handle);
    get_list_state();
  }

  void set_restore_states() {
    set_step_state();
    set_list_state();
  }

  void get_restore_states() {
    get_step_state();
    get_list_state();
  }
//}#pragma endregion States

//{#pragma region Text
  int get_letter(char letter) {
    int dx = 0;
    char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!-'&?.,/()[] ";
    char lower[] = "abcdefghijklmnopqrstuvwxyz0123456789!-'&?.,/()[] ";
    for(int n = 0; n < strlen(upper); n++) {
      if(letter == upper[n] || letter == lower[n]) {
        return letter != ' ' ? n * 5 : 0;
        break;
      }
    }
    return dx;
  }

  void draw_text(short x, short y, char *string, bool ext, bool current) {
    int length = !ext ? strlen(string) : strlen(string)-(strlen(EXTENSIONS[STEP])+1);
    int rows = 7;
    int cols = 5;
    for(int n = 0; n < length; n++) {
      int dx = get_letter(string[n]);
      int i = 0;
      for(int r = 0; r < (rows); r++) {
        if(string[n] != ' ') {
          for(int c = dx; c < (dx+cols); c++) {
            //buffer[i] = FONT_5x5[r][c] == 0 ? GUI.bg : current ? WHITE : GUI.fg;
            buffer[i] = FONT_5x7[r][c] == 0 ? GUI.bg : current ? GUI.hl : GUI.fg;
            i++;
          }
        }
      }
      if(string[n] != ' ') {
        ili9341_write_frame_rectangleLE(x, y-1, cols, rows, buffer);
      }
      x+= string[n] != ' ' ? 7 : 3;
    }
  }
//}#pragma endregion Text

//{#pragma region Mask
  void draw_mask(int x, int y, int w, int h){
    for (int i = 0; i < w * h; i++) buffer[i] = GUI.bg;
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_background() {
    int w = 320;
    int h = 60;
    for (int i = 0; i < 4; i++) draw_mask(0, i*h, w, h);
    draw_battery();
    draw_speaker();
    draw_contrast();
  }
//}#pragma endregion Mask

//{#pragma region Settings
  void draw_settings() {
    int x = ORIGIN.x;
    int y = POS.y + 46;

    draw_mask(x,y-1,100,17);
    draw_text(x,y,"THEMES",false, SETTING == 0 ? true : false);

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"COLORED ICONS",false, SETTING == 1 ? true : false);
    draw_toggle();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"VOLUME",false, SETTING == 2 ? true : false);

    draw_volume();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"BRIGHTNESS",false, SETTING == 3 ? true : false);

    draw_brightness();

    /*
      BUILD
    */
    char message[100] = BUILD;
    int width = strlen(message)*5;
    int center = ceil((320)-(width))-48;
    y = 225;
    draw_text(center,y,message,false,false);
  }
//}#pragma endregion Settings

//{#pragma region Toggle
  void draw_toggle() {
    get_toggle();
    int x = SCREEN.w - 38;
    int y = POS.y + 66;
    int w, h;

    int i = 0;
    for(h = 0; h < 9; h++) {
      for(w = 0; w < 18; w++) {
        buffer[i] = toggle[h + (COLOR*9)][w] == 0 ? 
        GUI.bg : toggle[h + (COLOR*9)][w] == WHITE ? 
        SETTING == 1 ? GUI.hl : GUI.fg : toggle[h + (COLOR*9)][w];
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 18, 9, buffer);
  }

  void set_toggle() {
    COLOR = COLOR == 0 ? 1 : 0;
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "COLOR", COLOR);
    nvs_commit(handle);
    nvs_close(handle);
  }

  void get_toggle() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "COLOR", &COLOR);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        COLOR = false;
        break;
      default :
        COLOR = false;
    }
    nvs_close(handle);
  }
//}#pragma endregion Toggle

//{#pragma region Volume
  void draw_volume() {
    int32_t volume = get_volume();
    int x = SCREEN.w - 120;
    int y = POS.y + 86;
    //int w = 25 * volume;
    int w, h;

    int i = 0;
    for(h = 0; h < 7; h++) {
      for(w = 0; w < 100; w++) {
        buffer[i] = (w+h)%2 == 0 ? GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 100, 7, buffer);

    if(volume > 0) {
      i = 0;
      for(h = 0; h < 7; h++) {
        for(w = 0; w < (25 * volume); w++) {
          if(SETTING == 2) {
            buffer[i] = GUI.hl;
          } else {
            buffer[i] = GUI.fg;
          }
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, (25 * volume), 7, buffer);
    }

    draw_speaker();
  }
  int32_t get_volume() {
    return odroid_settings_Volume_get();
  }
  void set_volume() {
    odroid_settings_Volume_set(VOLUME);
    draw_volume();
  }
//}#pragma endregion Volume

//{#pragma region Brightness
  void draw_brightness() {
    int x = SCREEN.w - 120;
    int y = POS.y + 106;
    int w, h;

    int i = 0;
    for(h = 0; h < 7; h++) {
      for(w = 0; w < 100; w++) {
        buffer[i] = (w+h)%2 == 0 ? GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 100, 7, buffer);

    //if(BRIGHTNESS > 0) {
      i = 0;
      for(h = 0; h < 7; h++) {
        for(w = 0; w < (BRIGHTNESS_COUNT * BRIGHTNESS)+BRIGHTNESS+1; w++) {
          if(SETTING == 3) {
            buffer[i] = GUI.hl;
          } else {
            buffer[i] = GUI.fg;
          }
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, (BRIGHTNESS_COUNT * BRIGHTNESS)+BRIGHTNESS+1, 7, buffer);
    //}
    draw_contrast();
  }
  int32_t get_brightness() {
    return odroid_settings_Backlight_get();
  }
  void set_brightness() {
    odroid_settings_Backlight_set(BRIGHTNESS);
    draw_brightness();
    apply_brightness();
  }
  void apply_brightness() {
    const int DUTY_MAX = 0x1fff;
    //BRIGHTNESS = get_brightness();
    int duty = DUTY_MAX * (BRIGHTNESS_LEVELS[BRIGHTNESS] * 0.01f);

    if(is_backlight_initialized()) {
      currentDuty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
      if (currentDuty != duty) {
        //ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, currentDuty);
        //ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        //ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 1000);
        //ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE /*LEDC_FADE_NO_WAIT|LEDC_FADE_WAIT_DONE|LEDC_FADE_MAX*/);
        //ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        //ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_set_fade_time_and_start(
          LEDC_LOW_SPEED_MODE,
          LEDC_CHANNEL_0,
          duty,
            25,
          LEDC_FADE_WAIT_DONE
        );
      }
    }
  }
//}#pragma endregion Brightness

//{#pragma region Theme
  void draw_themes() {
    int x = ORIGIN.x;
    int y = POS.y + 46;
    int filled = 0;
    int count = 22;
    for(int n = USER; n < count; n++){
      if(filled < ROMS.limit) {
        draw_mask(x,y-1,100,17);
        draw_text(x,y,THEMES[n].name,false, n == USER ? true : false);
        y+=20;
        filled++;
      }
    }
    int slots = (ROMS.limit - filled);
    for(int n = 0; n < slots; n++) {
      draw_mask(x,y-1,100,17);
      draw_text(x,y,THEMES[n].name,false,false);
      y+=20;
    }
  }

  void get_theme() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "USER", &USER);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        USER = 19;
        set_theme(USER);
        break;
      default :
        USER = 19;
        set_theme(USER);
    }
    nvs_close(handle);
  }

  void set_theme(int8_t i) {
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "USER", i);
    nvs_commit(handle);
    nvs_close(handle);
    get_theme();
  }

  void update_theme() {
    GUI = THEMES[USER];
    set_theme(USER);
    draw_background();
    draw_mask(0,0,320,64);
    draw_systems();
    draw_text(16,16,FEATURES[STEP], false, true);
    draw_themes();
  }
//}#pragma endregion Theme

//{#pragma region GUI
  void draw_systems() {
    for(int e = 0; e < COUNT; e++) {
      int i = 0;
      int x = SYSTEMS[e].x;
      int y = POS.y;
      int w = 32;
      int h = 32;
      if(x > 0 && x < 288) {
        for(int r = 0; r < 32; r++) {
          for(int c = 0; c < 32; c++) {
            switch(COLOR) {
              case 0:
                buffer[i] = (SYSTEMS[e].system)[r][c] == WHITE ? GUI.hl : GUI.bg;
              break;
              case 1:
                //buffer[i] = (SYSTEMS[e].system)[r][c] == WHITE ? GUI.hl : GUI.bg;
                buffer[i] = (SYSTEMS[e].color)[r][c] == 0 ? GUI.bg : (SYSTEMS[e].color)[r][c];
              break;
            }
            i++;
          }
        }
        ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      }
    }
  }

  void draw_folder(int x, int y, bool current) {
    int i = 0;
    for(int h = 0; h < 16; h++) {
      for(int w = 0; w < 16; w++) {
        buffer[i] = folder[h][w] == WHITE ? current ? WHITE : GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);
  }

  void draw_media(int x, int y, bool current) {
    int offset = (STEP-1) * 16;
    int i = 0;
    for(int h = 0; h < 16; h++) {
      for(int w = offset; w < (offset+16); w++) {
        switch(COLOR) {
          case 0:
            buffer[i] = media[h][w] == WHITE ? current ? GUI.hl : GUI.fg : GUI.bg;
          break;
          case 1:
            buffer[i] = media_color[h][w] == 0 ? GUI.bg : media_color[h][w];
            if(current) {
              buffer[i] = media_color[h+16][w] == 0 ? GUI.bg : media_color[h+16][w];
            }
          break;
        }
        /*

        */
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);
  }

  void draw_battery() {
    #ifdef BATTERY
      odroid_input_battery_level_read(&battery_state);

      int i = 0;
      int x = SCREEN.w - 32;
      int y = 8;
      int h = 0;
      int w = 0;

      draw_mask(x,y,16,16);
      for(h = 0; h < 16; h++) {
        for(w = 0; w < 16; w++) {
          buffer[i] = battery[h][w] == WHITE ? GUI.hl : GUI.bg;
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);

      int percentage = battery_state.percentage/10;
      x += 2;
      y += 6;
      w = percentage > 0 ? percentage > 10 ? 10 : percentage : 10;
      h = 4;
      i = 0;

      //printf("\nbattery_state.percentage:%d\n(percentage):%d\n(millivolts)%d\n", battery_state.percentage, percentage, battery_state.millivolts);

      int color[11] = {24576,24576,64288,64288,65504,65504,65504,26592,26592,26592,26592};

      int fill = color[w];
      for(int c = 0; c < h; c++) {
        for(int n = 0; n <= w; n++) {
          buffer[i] = fill;
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);

      /*
      if(battery_state.millivolts > 4200) {
        i = 0;
        for(h = 0; h < 5; h++) {
          for(w = 0; w < 3; w++) {
            buffer[i] = charge[h][w] == WHITE ? WHITE : fill;
            i++;
          }
        }
        ili9341_write_frame_rectangleLE(x+4, y, 3, 5, buffer);
      }
      */
    #endif
  }

  void draw_speaker() {
    int32_t volume = get_volume();

    int i = 0;
    int x = SCREEN.w - 52;
    int y = 8;
    int h = 16;
    int w = 16;

    draw_mask(x,y,16,16);

    int dh = 64 - (volume*16);
    for(h = 0; h < 16; h++) {
      for(w = 0; w < 16; w++) {
        buffer[i] = speaker[dh+h][w] == WHITE ? GUI.hl : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_contrast() {
    int32_t dy = 0;
    switch(BRIGHTNESS) {
      case 10:
      case 9:
      case 8:
        dy = 0;
      break;
      case 7:
      case 6:
      case 5:
        dy = 16;
      break;
      case 4:
      case 3:
      case 2:
        dy = 32;
      break;
      case 1:
      case 0:
        dy = 48;
      break;
    }
    int i = 0;
    int x = SCREEN.w - 72;
    int y = 8;
    int h = 16;
    int w = 16;

    draw_mask(x,y,16,16);

    for(h = 0; h < 16; h++) {
      for(w = 0; w < 16; w++) {
        buffer[i] = brightness[dy+h][w] == WHITE ? GUI.hl : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_numbers() {
    int x = 296;
    int y = POS.y + 48;
    int h = 5;
    int w = 0;
    char count[10];
    sprintf(count, "(%d/%d)", (ROMS.offset+1), ROMS.total);
    w = strlen(count)*5;
    x -= w;
    draw_mask(x-5,y,w+5,h);
    draw_text(x,y,count,false,false);
  }

  void draw_launcher() {
    draw_background();
    draw_text(16,16,FEATURES[STEP], false, true);
    int i = 0;
    int x = GAP/3;
    int y = POS.y;
    int w = 32;
    int h = 32;
    for(int r = 0; r < 32; r++) {
      for(int c = 0; c < 32; c++) {
        switch(COLOR) {
          case 0:
            buffer[i] = SYSTEMS[STEP].system[r][c] == WHITE ? GUI.hl : GUI.bg;
          break;
          case 1:
            //buffer[i] = SYSTEMS[e].system[r][c] == WHITE ? GUI.hl : GUI.bg;
            buffer[i] = SYSTEMS[STEP].color[r][c] == 0 ? GUI.bg : SYSTEMS[STEP].color[r][c];
          break;
        }
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);

    y += 48;
    draw_media(x,y-6,true);
    draw_launcher_options();
  }

  void draw_launcher_options() {
    //has_save_file(ROM.name);

    int x = ORIGIN.x;
    int y = POS.y + 48;
    int w = 5;
    int h = 5;
    int i = 0;
    int offset = 0;
    if(SAVED) {
      // resume
      i = 0;
      offset = 5;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 0 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x+20, y, w, h, buffer);
      draw_text(x+32,y,"Resume",false,OPTION == 0?true:false);
      // restart
      i = 0;
      y+=20;
      offset = 10;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 1 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x+20, y, w, h, buffer);
      draw_text(x+32,y,"Restart",false,OPTION == 1?true:false);
      // restart
      i = 0;
      y+=20;
      offset = 20;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 2 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x+20, y, w, h, buffer);
      draw_text(x+32,y,"Delete Save",false,OPTION == 2?true:false);
    } else {
      // run
      i = 0;
      offset = 0;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? GUI.hl : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x+20, y, w, h, buffer);
      draw_text(x+32,y,"Install Firmware",false,true);

      uint16_t* tile = malloc(TILE_LENGTH);
      draw_tile_image(odroid_settings_RomFilePath_get());
    }
  }
//}#pragma endregion GUI

//{#pragma region Tile
  void draw_tile_image(const char* filename) {
    printf("%s: filename='%s'\n", __func__, filename);
    const uint8_t DEFAULT_DATA = 0xff;


    FILE* file = fopen(filename, "rb");
    if (!file) {return;}

    size_t count = fread(FirmwareDescription, 1, FIRMWARE_DESCRIPTION_SIZE, file);

    uint16_t* tile = malloc(TILE_LENGTH);
    count = fread(tile, 1, TILE_LENGTH, file);

    int x = ORIGIN.x;
    int i = 0;
    for(int h = 0; h < (TILE_HEIGHT+2); h++) {
      for(int w = 0; w < TILE_WIDTH+1; w++) {
        buffer[i] = GUI.fg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x+31, POS.y+63, TILE_WIDTH+2, TILE_HEIGHT+1, buffer);
    i = 0;
    for(int h = 0; h < (TILE_HEIGHT-1); h++) {
      for(int w = 0; w < TILE_WIDTH; w++) {
        buffer[i] = tile[h * TILE_WIDTH + w + 12];
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x+32, POS.y+64, TILE_WIDTH, TILE_HEIGHT, buffer);
    int y = POS.y+76+TILE_HEIGHT;
    draw_text(x+32,y,ROM.name,false,false);

    free(tile);
    fclose(file);
  }
//}#pragma endregion Tile

//{#pragma region Files
  //{#pragma region Sort
    inline static void swap(char** a, char** b) {
        char* t = *a;
        *a = *b;
        *b = t;
    }

    static int strcicmp(char const *a, char const *b) {
        for (;; a++, b++)
        {
            int d = tolower((int)*a) - tolower((int)*b);
            if (d != 0 || !*a) return d;
        }
    }

    static int partition (char** arr, int low, int high) {
        char* pivot = arr[high];
        int i = (low - 1);

        for (int j = low; j <= high- 1; j++)
        {
            if (strcicmp(arr[j], pivot) < 0)
            {
                i++;
                swap(&arr[i], &arr[j]);
            }
        }
        swap(&arr[i + 1], &arr[high]);
        return (i + 1);
    }

    void quick_sort(char** arr, int low, int high) {
        if (low < high)
        {
            int pi = partition(arr, low, high);

            quick_sort(arr, low, pi - 1);
            quick_sort(arr, pi + 1, high);
        }
    }

    void sort_files(char** files)
    {
        if (ROMS.total > 1)
        {
            quick_sort(files, 0, ROMS.total - 1);
        }
    }
  //}#pragma endregion Sort

  void get_files() {
    if(SD) {

      free(FILES);
      FILES = (char**)malloc(MAX_FILES * sizeof(void*));
      ROMS.total = 0;

      char path[256] = "/sd/odroid/";
      strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
      strcat(&path[strlen(path) - 1],folder_path);
      printf("\npath:%s\n", path);

      strcpy(ROM.path, path);

      DIR *directory = opendir(path);

      if(directory == NULL) {
        char message[100] = "no firmware available";
        int center = ceil((320/2)-((strlen(message)*5)/2));
        draw_text(center,134,message,false,false);
      } else {
        struct dirent *file;
        while ((file = readdir(directory)) != NULL) {
          int firmware_length = strlen(file->d_name);
          int ext_lext = strlen(EXTENSIONS[STEP]);
          bool extenstion = strcmp(&file->d_name[firmware_length - ext_lext], EXTENSIONS[STEP]) == 0 && file->d_name[0] != '.';
          if(extenstion || (file->d_type == 2)) {
            if(ROMS.total >= MAX_FILES) { break; }
            size_t len = strlen(file->d_name);
            FILES[ROMS.total] = (file->d_type == 2) ? (char*)malloc(len + 5) : (char*)malloc(len + 1);
            if((file->d_type == 2)) {
              char dir[256];
              strcpy(dir, file->d_name);
              char dd[8];
              sprintf(dd, "%s", ext_lext == 2 ? "dir" : ".dir");
              strcat(&dir[strlen(dir) - 1], dd);
              strcpy(FILES[ROMS.total], dir);
            } else {
              strcpy(FILES[ROMS.total], file->d_name);
            }
            ROMS.total++;
          }
        }
        ROMS.pages = ROMS.total/ROMS.limit;
        if(ROMS.offset > ROMS.total) { ROMS.offset = 0;}

        closedir(directory);
        if(ROMS.total < 500) sort_files(FILES);
        draw_files();

        //free(FILES);
      }              

    } else {
      char message[100] = "insert sd card";
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false);    

      sprintf(message, "[press a to restart]");
      center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,225,message,false,false);    
    }
    
  }

  void draw_files() {
    //printf("\n");
    int x = ORIGIN.x;
    int y = POS.y + 48;
    ROMS.page = ROMS.offset/ROMS.limit;

    /*
    printf("\nROMS.offset:%d", ROMS.offset);
    printf("\nROMS.limit:%d", ROMS.limit);
    printf("\nROMS.total:%d", ROMS.total);
    printf("\nROMS.page:%d", ROMS.page);
    printf("\nROMS.pages:%d", ROMS.pages);
    */

    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);

    int limit = (ROMS.offset + ROMS.limit) > ROMS.total ? ROMS.total : ROMS.offset + ROMS.limit;
    for(int n = ROMS.offset; n < limit; n++) {
      draw_text(x+24,y,FILES[n],true,n == ROMS.offset ? true : false);

      bool directory = strcmp(&FILES[n][strlen(FILES[n]) - 3], "dir") == 0;
      directory ? draw_folder(x,y-6,n == ROMS.offset ? true : false) : draw_media(x,y-6,n == ROMS.offset ? true : false);
      if(n == ROMS.offset) {
        strcpy(ROM.name, FILES[n]);
        ROM.ready = true;
      }
      y+=20;
    }


    draw_numbers();
  }

  void has_save_file(char *name) {
    SAVED = false;
    DIR *directory;
    struct dirent *file;
    char path[256] = "/sd/odroid/data/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    directory = opendir(path);
    gets(name);
    while ((file = readdir(directory)) != NULL) {
      char tmp[256] = "";
      strcat(tmp, file->d_name);
      tmp[strlen(tmp)-4] = '\0';
      gets(tmp);
      if(strcmp(name, tmp) == 0){
        SAVED = true;
      }
    }
    closedir(directory);
  }
//}#pragma endregion Files

//{#pragma region Animations
  void animate(int dir) {
    int y = POS.y + 46;
    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);
    int sx[4][13] = {
      {8,8,4,4,4,3,3,3,3,2,2,2,2}, // 48
      {38,34,24,24,22,21,19,18,15,14,10,9,8} // 208 30+26+20+20+18+18+16+16+12+12+8+8+4
    };
    for(int i = 0; i < 13; i++) {
      if(dir == -1) {
        // LEFT
        for(int e = 0; e < COUNT; e++) {
          SYSTEMS[e].x += STEP != COUNT - 1 ?
            STEP == (e-1) ? sx[1][i] : sx[0][i] :
            e == 0 ? sx[1][i] : sx[0][i] ;
        }
      } else {
        // RIGHT
        for(int e = 0; e < COUNT; e++) {
          SYSTEMS[e].x -= STEP == e ? sx[1][i] : sx[0][i];
        }
      }
      draw_mask(0,32,320,32);

      draw_systems();
      usleep(20000);
    }
    draw_mask(0,0,SCREEN.w - 56,32);
    draw_text(16,16,FEATURES[STEP], false, true);
    STEP == 0 ? draw_settings() : get_files();
    clean_up();
    draw_systems();
  }

  void restore_layout() {

    SYSTEMS[0].x = GAP/3;
    for(int n = 1; n < COUNT; n++) {
      if(n == 1) {
        SYSTEMS[n].x = GAP/3+NEXT;
      } else if(n == 2) {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP);
      } else {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP*(n-1));
      }
    };

    draw_background();

    for(int n = 0; n < COUNT; n++) {
      int delta = (n-STEP);
      if(delta < 0) {
        SYSTEMS[n].x = (GAP/3) + (GAP * delta);
      } else if(delta == 0) {
        SYSTEMS[n].x = (GAP/3);
      } else if(delta == 1) {
        SYSTEMS[n].x = GAP/3+NEXT;
      } else if(delta == 2) {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP);
      } else {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP*(delta-1));
      }
    }

    clean_up();
    draw_systems();
    draw_text(16,16,FEATURES[STEP],false,true);
    STEP == 0 ? draw_settings() : get_files();
  }

  void clean_up() {             
    int MAX = 304; //256
    printf("\n%s", __func__);
    for(int n = 0; n < COUNT; n++) {
      if(SYSTEMS[n].x >= 320) {
        SYSTEMS[n].x -= MAX;
        printf("<-- HERE");
      }
      if(SYSTEMS[n].x <= -32) {
        SYSTEMS[n].x += MAX;
      }
      printf("\nn:%d x:%d", n, SYSTEMS[n].x);
    }
    printf("\nend %s", __func__);
  }
//}#pragma endregion Animations

//{#pragma region Boot Screens
  void splash() {
    draw_background();
    int w = 128;
    int h = 18;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    int i = 0;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        buffer[i] = logo[r][c] == 0 ? GUI.bg : GUI.hl;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);

    /*
      BUILD
    */
    char message[100] = BUILD;
    int width = strlen(message)*5;
    int center = ceil((320)-(width))-48;
    y = 225;
    draw_text(center,y,message,false,false);

    sleep(2);
    draw_background();
  }

  void boot() {
    draw_background();
    char message[100] = "retro esp32";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false);

    y+=10;
    for(int n = 0; n < (width+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(center+n, y, 1, 5, buffer);
      usleep(10000);
    }
  }

  void restart() {
    draw_background();

    char message[100] = "restarting";
    int h = 5;
    int w = strlen(message)*h;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    draw_text(x,y,message,false,false);

    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    restore_layout();
  }
//}#pragma endregion Boot Screens

//{#pragma region Firmware Options
  void firmware_debug(char *string) {
    printf("\n**********\n%s\n**********\n", string);
  }

  void firmware_status(char *string) {
    int x = ORIGIN.x+32;
    int y = SCREEN.h-16;
    int w = SCREEN.w-32;
    int h = 16;
    draw_mask(x, y-2, w, h+4);
    draw_text(x,y,string,false,true);
  }

  void firmware_progress(int percentage) {
    int x = ORIGIN.x+32;
    int y = SCREEN.h-32;
    int w, h;

    int i = 0;

    if(percentage >= 99) {
      draw_mask(x,y,100,7);
      return;
    }

    for(h = 0; h < 7; h++) {
      for(w = 0; w < 100; w++) {
        buffer[i] = (w+h)%2 == 0 ? GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 100, 7, buffer);

    if(percentage > 0) {
      i = 0;
      for(h = 0; h < 7; h++) {
        for(w = 0; w < (percentage); w++) {
          buffer[i] = GUI.hl;
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, (percentage), 7, buffer);
    }
  }

  void firmware_run(bool resume) {

    size_t count;
    const char* filename = odroid_settings_RomFilePath_get();
    const char message[100];

    int x = ORIGIN.x+32;
    int y = SCREEN.h-16;
    int w = SCREEN.w-32;
    int h = 16;

    printf("%s: HEAP=%#010x\n", __func__, esp_get_free_heap_size());

    firmware_status("Initialization");

    //{#pragma region File
      FILE* file = fopen(filename, "rb");
      if (!file) {
        sprintf(message, "%s: FILE ERROR", __func__);
        firmware_debug(message);
        firmware_status("FILE ERROR");
        return;
      }
    //}#pragma endregion File

    //{#pragma region Header
      const size_t headerLength = strlen(HEADER_V00_01);
      char* header = malloc(headerLength + 1);
      if(!header) {
        sprintf(message, "%s: MEMORY ERROR", __func__);
        firmware_debug(message);
        firmware_status("MEMORY ERROR");
        return;
      }
      memset(header, 0, headerLength + 1);
      count = fread(header, 1, headerLength, file);

      if (count != headerLength) {
        sprintf(message, "%s: HEADER READ ERROR", __func__);
        firmware_debug(message);
        firmware_status("HEADER READ ERROR");
        return;
      }

      if (strncmp(HEADER_V00_01, header, headerLength) != 0) {
        sprintf(message, "%s: HEADER MATCH ERROR - %s", __func__, header);
        firmware_debug(message);
        firmware_status("HEADER MATCH ERROR");
        return;
      }
      firmware_status("Header OK");
      free(header);
    //}#pragma endregion Header

    //{#pragma region Description
      count = fread(FirmwareDescription, 1, FIRMWARE_DESCRIPTION_SIZE, file);
      if (count != FIRMWARE_DESCRIPTION_SIZE)
      {
        sprintf(message, "%s: DESCRIPTION READ ERROR", __func__);
        firmware_debug(message);
        firmware_status("DESCRIPTION READ ERROR");
        return;
      }
      FirmwareDescription[FIRMWARE_DESCRIPTION_SIZE - 1] = 0;
    //}#pragma endregion Description

    //{#pragma region Tile
      uint16_t* tileData = malloc(TILE_LENGTH);
      if (!tileData)
      {
        sprintf(message, "%s: TILE MEMORY ERROR", __func__);
        firmware_debug(message);
        firmware_status("TILE MEMORY ERROR");
        return;
      }
      count = fread(tileData, 1, TILE_LENGTH, file);
      if (count != TILE_LENGTH)
      {
        sprintf(message, "%s: TILE READ ERROR", __func__);
        firmware_debug(message);
        firmware_status("TILE READ ERROR");
        return;
      }
      free(tileData);
    //}#pragma endregion Tile

    //{#pragma region Erase
      const int ERASE_BLOCK_SIZE = 4096;
      void* data = malloc(ERASE_BLOCK_SIZE);
      if (!data) {
        sprintf(message, "%s: DATA ERROR", __func__);
        firmware_debug(message);
        firmware_status("DATA ERROR");
        return;
      }
    //}#pragma endregion Erase

    //{#pragma region Checksum
      size_t current_position = ftell(file);

      fseek(file, 0, SEEK_END);
      size_t file_size = ftell(file);

      uint32_t expected_checksum;
      fseek(file, file_size - sizeof(expected_checksum), SEEK_SET);
      count = fread(&expected_checksum, 1, sizeof(expected_checksum), file);

      if (count != sizeof(expected_checksum)) {
        sprintf(message, "%s: CHECKSUM READ ERROR", __func__);
        firmware_debug(message);
        firmware_status("CHECKSUM READ ERROR");
        return;
      }

      fseek(file, 0, SEEK_SET);

      uint32_t checksum = 0;
      size_t check_offset = 0;
    //}#pragma endregion Checksum

    //{#pragma region Read
      int i = 0;
      int dot = 0;
      double percentage = 0;
      while(true) {
        count = fread(data, 1, ERASE_BLOCK_SIZE, file);
        if (check_offset + count == file_size){count -= 4;}
        checksum = crc32_le(checksum, data, count);
        check_offset += count;
        percentage = ((double)check_offset/(double)file_size)*100;
        firmware_progress((int)percentage);
        i++;
        if(i%25 == 0) {
          dot++;
          if(dot == 4) {dot = 0;}
          draw_mask(x, y-2, w, h+4);
          switch(dot) {
            case 1:
              draw_text(x,y,"Verifying.",false,true);
            break;
            case 2:
              draw_text(x,y,"Verifying..",false,true);
            break;
            case 3:
              draw_text(x,y,"Verifying...",false,true);
            break;
            default:
              draw_text(x,y,"Verifying",false,true);
            break;
          }
        }

        if (count < ERASE_BLOCK_SIZE) break;
      }

      if (checksum != expected_checksum) {
        sprintf(message, "%s: CHECKSUM MISMATCH ERROR", __func__);
        firmware_debug(message);
        firmware_status("CHECKSUM MISMATCH ERROR");
        return;
      }

      fseek(file, current_position, SEEK_SET);
    //}#pragma endregion Read

    //{#pragma region Partition
      const esp_partition_t* factory_part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
      if (factory_part == NULL)
      {
        sprintf(message, "%s: FACTORY PARTITION ERROR", __func__);
        firmware_debug(message);
        firmware_status("FACTORY PARTITION ERROR");
        return;
      }
    //}#pragma endregion Partition

    const size_t FLASH_START_ADDRESS = factory_part->address + factory_part->size;

    printf("%s: expected_checksum=%#010x\n", __func__, expected_checksum);
    printf("%s: checksum=%#010x\n", __func__, checksum);
    printf("%s: FLASH_START_ADDRESS=%#010x\n", __func__, FLASH_START_ADDRESS);

    //{#pragma region Parts
      const size_t PARTS_MAX = 20;
      int parts_count = 0;
      odroid_partition_t* parts = malloc(sizeof(odroid_partition_t) * PARTS_MAX);

      if (!parts)
      {
        sprintf(message, "%s: PARTITION MEMORY ERROR", __func__);
        firmware_debug(message);
        firmware_status("PARTITION MEMORY ERROR");
        return;
      }
    //}#pragma endregion Parts

    //{#pragma region Copy
      size_t address = FLASH_START_ADDRESS;

      while(true) {
        if (ftell(file) >= (file_size - sizeof(checksum))) {
          break;
        }
        odroid_partition_t slot;
        count = fread(&slot, 1, sizeof(slot), file);

        printf("\n"
               "\n------------"
               "\nFirmware Details"
               "\nfile_size:%d"
               "\naddress:%d"
               "\nslot.length:%d"
               "\n(address+length) %d"
               "\nmax:%d"
               "\n------------\n",
          file_size,
          address,
          slot.length,
          (address+slot.length),
          (16 * 1024 * 1024)
        );

        //{#pragma region Errors
          if (count != sizeof(slot)) {
            sprintf(message, "%s: PARTITION READ ERROR", __func__);
            firmware_debug(message);
            firmware_status("PARTITION READ ERROR");
            return;
          }

          if (parts_count >= PARTS_MAX) {
            sprintf(message, "%s: PARTITION COUNT ERROR", __func__);
            firmware_debug(message);
            firmware_status("PARTITION COUNT ERROR");
            return;
          }

          if (slot.type == 0xff) {
            sprintf(message, "%s: PARTITION TYPE ERROR", __func__);
            firmware_debug(message);
            firmware_status("PARTITION TYPE ERROR");
            return;
          }

          if (address + slot.length > 16 * 1024 * 1024) {
            sprintf(message, "%s: PARTITION LENGTH ERROR", __func__);
            firmware_debug(message);
            firmware_status("PARTITION LENGTH ERROR");
            return;
          }

          if ((address & 0xffff0000) != address) {
            sprintf(message, "%s: PARTITION ALIGNMENT ERROR", __func__);
            firmware_debug(message);
            firmware_status("PARTITION ALIGNMENT ERROR");
            return;
          }
        //}#pragma endregion Errors

        //{#pragma region Write
          uint32_t length;
          count = fread(&length, 1, sizeof(length), file);

          if (count != sizeof(length)) {
            sprintf(message, "%s: LENGTH READ ERROR", __func__);
            firmware_debug(message);
            firmware_status("LENGTH READ ERROR");
            return;
          }

          if (length > slot.length) {
            sprintf(message, "%s: DATA LENGTH ERROR", __func__);
            firmware_debug(message);
            firmware_status("DATA LENGTH ERROR");
            return;
          }

          size_t nextEntry = ftell(file) + length;

          //{#pragma region TX/RX
            if (length > 0) {
              int eraseBlocks = length / ERASE_BLOCK_SIZE;
              if (eraseBlocks * ERASE_BLOCK_SIZE < length) ++eraseBlocks;

              sprintf(message, "Erasing Partition [%d]", parts_count);
              firmware_debug(message);
              firmware_status(message);
              firmware_progress(0);

              esp_err_t ret = spi_flash_erase_range(address, eraseBlocks * ERASE_BLOCK_SIZE);

              if (ret != ESP_OK) {
                sprintf(message, "%s: ERASE ERROR", __func__);
                firmware_debug(message);
                firmware_status("ERASE ERROR");
                return;
              }

              int totalCount = 0;

              sprintf(message, "Writing Partition [%d]", parts_count);
              firmware_debug(message);

              for (int offset = 0; offset < length; offset += ERASE_BLOCK_SIZE) {
                percentage = ((double)offset/(double)(length - ERASE_BLOCK_SIZE))*100;
                // sprintf(message, "Writing Part - %d %d", parts_count, (int)percentage);
                firmware_status(message);
                firmware_progress((int)percentage);

                count = fread(data, 1, ERASE_BLOCK_SIZE, file);
                if (count <= 0) {
                  sprintf(message, "%s: DATA READ ERROR", __func__);
                  firmware_debug(message);
                  firmware_status("DATA READ ERROR");
                  return;
                }

                if (offset + count >= length) {
                    count = length - offset;
                }

                ret = spi_flash_write(address + offset, data, count);
                if (ret != ESP_OK) {
                  sprintf(message, "%s: WRITE ERROR", __func__);
                  firmware_debug(message);
                  firmware_status("WRITE ERROR");
                }

                totalCount += count;
              }

              if (totalCount != length)
              {
                  sprintf(message, "%s: DATA SIZE ERROR", __func__);
                  firmware_debug(message);
                  firmware_status("DATA SIZE ERROR");
                  //printf("Size mismatch: lenght=%#08x, totalCount=%#08x\n", length, totalCount);
              }
            }
          //}#pragma endregion TX/RX

        parts[parts_count++] = slot;
        address += slot.length;


        // Seek to next entry
        if (fseek(file, nextEntry, SEEK_SET) != 0)
        {
          sprintf(message, "%s: SEEK ERROR", __func__);
          firmware_debug(message);
          firmware_status("SEEK ERROR");
        }
        //}#pragma endregion Write
      }
    //}#pragma endregion Copy


    close(file);

    //{#pragma region Utils
    //}#pragma enregion Utils

    //{#pragma region Partition
      firmware_partition(parts, parts_count);
      free(data);
      odroid_sdcard_close();
    //}#pragma enregion Partition

    //{#pragma region Boot
      firmware_boot();
    //}#pragma enregion Boot
  }


  #define ESP_PARTITION_TABLE_OFFSET CONFIG_PARTITION_TABLE_OFFSET /* Offset of partition table. Backwards-compatible name.*/
  #define ESP_PARTITION_TABLE_MAX_LEN 0xC00 /* Maximum length of partition table data */
  #define ESP_PARTITION_TABLE_MAX_ENTRIES (ESP_PARTITION_TABLE_MAX_LEN / sizeof(esp_partition_info_t)) /* Maximum length of partition table data, including terminating entry */

  #define PART_TYPE_APP 0x00
  #define PART_SUBTYPE_FACTORY 0x00

  void firmware_partition(odroid_partition_t* parts, size_t parts_count) {

    const char message[100];

    int x = ORIGIN.x+32;
    int y = SCREEN.h-16;
    int w = SCREEN.w-32;
    int h = 16;

    esp_err_t err;

    // Read table
    const esp_partition_info_t* partition_data = (const esp_partition_info_t*)malloc(ESP_PARTITION_TABLE_MAX_LEN);
    if (!partition_data)
    {
      sprintf(message, "%s: TABLE MEMORY  ERROR", __func__);
      firmware_debug(message);
      firmware_status("TABLE MEMORY  ERROR");
    }

    err = spi_flash_read(ESP_PARTITION_TABLE_OFFSET, (void*)partition_data, ESP_PARTITION_TABLE_MAX_LEN);
    if (err != ESP_OK)
    {
      sprintf(message, "%s: TABLE READ ERROR", __func__);
      firmware_debug(message);
      firmware_status("TABLE READ ERROR");
    }

    // Find end of first partitioned
    int startTableEntry = -1;
    size_t startFlashAddress = 0xffffffff;

    for (int i = 0; i < ESP_PARTITION_TABLE_MAX_ENTRIES; ++i)
    {
        const esp_partition_info_t *part = &partition_data[i];
        if (part->magic == 0xffff) break;

        if (part->magic == ESP_PARTITION_MAGIC)
        {
            if (part->type == PART_TYPE_APP &&
                part->subtype == PART_SUBTYPE_FACTORY)
            {
                startTableEntry = i + 1;
                startFlashAddress = part->pos.offset + part->pos.size;
                break;
            }
        }
    }

    if (startTableEntry < 0)
    {
      sprintf(message, "%s: NO FACTORY PARTITION ERROR", __func__);
      firmware_debug(message);
      firmware_status("NO FACTORY PARTITION ERROR");
    }

    //printf("%s: startTableEntry=%d, startFlashAddress=%#08x\n",__func__, startTableEntry, startFlashAddress);

    // blank partition table entries
    for (int i = startTableEntry; i < ESP_PARTITION_TABLE_MAX_ENTRIES; ++i)
    {
        memset(&partition_data[i], 0xff, sizeof(esp_partition_info_t));
    }

    // Add partitions
    size_t offset = 0;
    for (int i = 0; i < parts_count; ++i)
    {
        esp_partition_info_t* part = &partition_data[startTableEntry + i];
        part->magic = ESP_PARTITION_MAGIC;
        part->type = parts[i].type;
        part->subtype = parts[i].subtype;
        part->pos.offset = startFlashAddress + offset;
        part->pos.size = parts[i].length;
        for (int j = 0; j < 16; ++j)
        {
            part->label[j] = parts[i].label[j];
        }
        part->flags = parts[i].flags;

        offset += parts[i].length;
    }

    //abort();

    // Erase partition table
    if (ESP_PARTITION_TABLE_MAX_LEN > 4096)
    {
      sprintf(message, "%s: TABLE SIZE ERROR", __func__);
      firmware_debug(message);
      firmware_status("TABLE SIZE ERROR");
    }

    err = spi_flash_erase_range(ESP_PARTITION_TABLE_OFFSET, 4096);
    if (err != ESP_OK)
    {
      sprintf(message, "%s: TABLE ERASE ERROR", __func__);
      firmware_debug(message);
      firmware_status("TABLE ERASE ERROR");
    }

    // Write new table
    err = spi_flash_write(ESP_PARTITION_TABLE_OFFSET, (void*)partition_data, ESP_PARTITION_TABLE_MAX_LEN);
    if (err != ESP_OK)
    {
      sprintf(message, "%s: TABLE WRITE ERROR", __func__);
      firmware_debug(message);
      firmware_status("TABLE WRITE ERROR");
    }

    esp_partition_reload_table();

    sprintf(message, "%s: SUCCESS", __func__);
    firmware_debug(message);
  }

  void firmware_boot() {
    const char message[100];

    int x = ORIGIN.x+32;
    int y = SCREEN.h-16;
    int w = SCREEN.w-32;
    int h = 16;

    sprintf(message, "%s: BOOTING APPLICATION", __func__);
    firmware_debug(message);
    firmware_status("BOOTING APPLICATION");

    // Set firmware active
    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (partition == NULL)
    {
      sprintf(message, "%s: NO BOOT PART ERROR", __func__);
      firmware_debug(message);
      firmware_status("NO BOOT PART ERROR");
    }

    esp_err_t err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK)
    {
      sprintf(message, "%s: BOOT SET ERROR", __func__);
      firmware_debug(message);
      firmware_status("BOOT SET ERROR");
    }

    sprintf(message, "%s: SUCCESS", __func__);
    firmware_debug(message);

    // reboot
    esp_restart();
  }

  void firmware_delete() {
    draw_background();
    char message[100] = "deleting...";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false);

    y+=10;
    for(int n = 0; n < (width+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(center+n, y, 1, 5, buffer);
      usleep(15000);
    }
  }
//}#pragma endregion Firmware Options

//{#pragma region Launcher
  static void launcher() {
    draw_battery();
    draw_speaker();
    draw_contrast();
    while (true) {
      /*
        Get Gamepad State
      */
      odroid_input_gamepad_read(&gamepad);
      /*
        LEFT
      */
      if(gamepad.values[ODROID_INPUT_LEFT]) {
        if(!LAUNCHER && !FOLDER) {
          if(!SETTINGS && SETTING != 1 && SETTING != 2 && SETTING != 3) {
            STEP--;
            if( STEP < 0 ) {
              STEP = COUNT - 1;
            }

            ROMS.offset = 0;
            animate(-1);
          } else {
            if(SETTING == 1) {
              nvs_handle handle;
              nvs_open("storage", NVS_READWRITE, &handle);
              nvs_set_i8(handle, "COLOR", 0);
              nvs_commit(handle);
              nvs_close(handle);
              draw_toggle();
              draw_systems();
            }
            if(SETTING == 2) {
              if(VOLUME > 0) {
                VOLUME--;
                set_volume();
                usleep(200000);
              }
            }
            if(SETTING == 3) {
              if(BRIGHTNESS > 0) {
                BRIGHTNESS--;
                set_brightness();
                usleep(200000);
              }
            }
          }
        }
        usleep(100000);
        //debounce(ODROID_INPUT_LEFT);
      }
      /*
        RIGHT
      */
      if(gamepad.values[ODROID_INPUT_RIGHT]) {
        if(!LAUNCHER && !FOLDER) {
          if(!SETTINGS && SETTING != 1 && SETTING != 2 && SETTING != 3) {
            STEP++;
            if( STEP > COUNT-1 ) {
              STEP = 0;
            }
            ROMS.offset = 0;
            animate(1);
          } else {
            if(SETTING == 1) {
              nvs_handle handle;
              nvs_open("storage", NVS_READWRITE, &handle);
              nvs_set_i8(handle, "COLOR", 1);
              nvs_commit(handle);
              nvs_close(handle);
              draw_toggle();
              draw_systems();
            }
            if(SETTING == 2) {
              if(VOLUME < 4) {
                VOLUME++;
                set_volume();
                usleep(200000);
              }
            }
            if(SETTING == 3) {
              if(BRIGHTNESS < (BRIGHTNESS_COUNT-1)) {
                BRIGHTNESS++;
                set_brightness();
                usleep(200000);
              }
            }
          }
        }
        usleep(100000);
        //debounce(ODROID_INPUT_RIGHT);
      }
      /*
        UP
      */
      if (gamepad.values[ODROID_INPUT_UP]) {
        if(!LAUNCHER) {
          if(STEP == 0) {
            if(!SETTINGS) {
              SETTING--;
              if( SETTING < 0 ) { SETTING = 3; }
              draw_settings();
            } else {
              USER--;
              if( USER < 0 ) { USER = 21; }
              draw_themes();
            }
          }
          if(STEP != 0) {
            ROMS.offset--;
            if( ROMS.offset < 0 ) { ROMS.offset = ROMS.total - 1; }
            draw_files();
          }
        } else {
          if(SAVED) {
            OPTION--;
            if( OPTION < 0 ) { OPTION = 2; }
            draw_launcher_options();
          }
        }
        usleep(200000);
        //debounce(ODROID_INPUT_UP);
      }
      /*
        DOWN
      */
      if (gamepad.values[ODROID_INPUT_DOWN]) {
        if(!LAUNCHER) {
          if(STEP == 0) {
            if(!SETTINGS) {
              SETTING++;
              if( SETTING > 3 ) { SETTING = 0; }
              draw_settings();
            } else {
              USER++;
              if( USER > 21 ) { USER = 0; }
              draw_themes();
            }
          }
          if(STEP != 0) {
            ROMS.offset++;
            if( ROMS.offset > ROMS.total - 1 ) { ROMS.offset = 0; }
            draw_files();
          }
        } else {
          if(SAVED) {
            OPTION++;
            if( OPTION > 2 ) { OPTION = 0; }
            draw_launcher_options();
          }
        }

        usleep(200000);
        //debounce(ODROID_INPUT_DOWN);
      }

      /*
        START + SELECT
      */
      if (gamepad.values[ODROID_INPUT_START] || gamepad.values[ODROID_INPUT_SELECT]) {
        /*
          SELECT
        */
        if (gamepad.values[ODROID_INPUT_START] && !gamepad.values[ODROID_INPUT_SELECT]) {
          if(!LAUNCHER) {
            if(STEP != 0) {
              ROMS.page++;
              if( ROMS.page > ROMS.pages ) { ROMS.page = 0; }
              ROMS.offset =  ROMS.page * ROMS.limit;
              draw_files();
            }
          }
          //debounce(ODROID_INPUT_START);
        }

        /*
          SELECT
        */
        if (!gamepad.values[ODROID_INPUT_START] && gamepad.values[ODROID_INPUT_SELECT]) {
          if(!LAUNCHER) {
            if(STEP != 0) {
              ROMS.page--;
              if( ROMS.page < 0 ) { ROMS.page = ROMS.pages; };
              ROMS.offset =  ROMS.page * ROMS.limit;
              draw_files();
            }
          }
          //debounce(ODROID_INPUT_SELECT);
        }

        usleep(200000);
      }

      /*
        BUTTON A
      */
      if (gamepad.values[ODROID_INPUT_A]) {
        if(STEP == 0) {
          if(!SETTINGS && SETTING == 0) {
            SETTINGS = true;
            draw_background();
            draw_systems();
            switch(SETTING) {
              case 0:
                draw_text(16,16,"THEMES",false,true);
                draw_themes();
              break;
            }
          } else {
            printf("\nSETTING:%d COLOR:%d\n", SETTING, COLOR);
            switch(SETTING) {
              case 0:
                update_theme();
              break;
              case 1:
                set_toggle();
                draw_toggle();
                draw_systems();
              break;
            }
          }
        } else {
          if (ROM.ready && !LAUNCHER ) {
            OPTION = 0;
            char file_to_load[256] = "";
            sprintf(file_to_load, "%s/%s", ROM.path, ROM.name);
            bool directory = strcmp(&file_to_load[strlen(file_to_load) - 3], "dir") == 0;

            if(directory) {
              FOLDER = true;
              PREVIOUS = ROMS.offset;
              ROMS.offset = 0;
              ROMS.total = 0;

              sprintf(folder_path, "/%s", ROM.name);
              folder_path[strlen(folder_path)-(strlen(EXTENSIONS[STEP]) == 3 ? 4 : 3)] = 0;
              draw_background();
              draw_systems();
              draw_text(16,16,FEATURES[STEP],false,true);
              get_files();
            } else {
              LAUNCHER = true;
              odroid_settings_RomFilePath_set(file_to_load);
              draw_launcher();
            }
          } else {
            if(SD) {
              switch(OPTION) {
                case 0:
                case 1:
                  firmware_run(true);
                break;
                case 2:
                  firmware_delete();
                break;
              }            
            } else {
              esp_restart();                      
            }
          }
        }
        debounce(ODROID_INPUT_A);
      }
      /*
        BUTTON B
      */
      if (gamepad.values[ODROID_INPUT_B]) {
        if (ROM.ready && LAUNCHER) {
          LAUNCHER = false;
          draw_background();
          draw_systems();
          draw_text(16,16,FEATURES[STEP],false,true);
          if(FOLDER) {
            printf("\n------\nfolder_path:%s\n-----\n", folder_path);
            get_files();
          } else {
            STEP == 0 ? draw_settings() : draw_files();
          }
        } else {
          if(FOLDER) {
            FOLDER = false;
            ROMS.offset = PREVIOUS;
            ROMS.total = 0;
            PREVIOUS = 0;
            folder_path[0] = 0;
            get_files();
          }
          if(SETTINGS) {
            SETTINGS = false;
            draw_background();
            draw_systems();
            draw_text(16,16,FEATURES[STEP],false,true);
            draw_settings();
          }
        }
        debounce(ODROID_INPUT_B);
      }

      /*
        START + SELECT (MENU)
      */
      if (gamepad.values[ODROID_INPUT_MENU]) {
        usleep(10000);
        esp_restart();
        debounce(ODROID_INPUT_MENU);
      }
    }
  }
//}#pragma endregion Launcher
