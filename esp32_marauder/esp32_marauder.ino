/* FLASH SETTINGS
Board: LOLIN D32
Flash Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
https://www.online-utility.org/image/convert/to/XBM
*/

#include "configs.h"

#ifndef HAS_SCREEN
  #define MenuFunctions_h
  #define Display_h
#endif

#include <stdio.h>

#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif

#include "Assets.h"
#include "WiFiScan.h"
#ifdef HAS_SD
  #include "SDInterface.h"
#endif
#include "Buffer.h"

#ifdef HAS_FLIPPER_LED
  #include "flipperLED.h"
#elif defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
#elif defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
  #include "stickcLED.h"
#elif defined(HAS_NEOPIXEL_LED)
  #include "LedInterface.h"
#endif

#include "settings.h"
#include "CommandLine.h"
#include "lang_var.h"

#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif

#ifdef HAS_SCREEN
  #include "Display.h"
  #include "MenuFunctions.h"
#endif

#ifdef MARAUDER_M5STICKCP2_FLIPPER
  #include <TFT_eSPI.h>
#endif

#ifdef HAS_BUTTONS
  #include "Switches.h"
  
  #if (U_BTN >= 0)
    Switches u_btn = Switches(U_BTN, 1000, U_PULL);
  #endif
  #if (D_BTN >= 0)
    Switches d_btn = Switches(D_BTN, 1000, D_PULL);
  #endif
  #if (L_BTN >= 0)
    Switches l_btn = Switches(L_BTN, 1000, L_PULL);
  #endif
  #if (R_BTN >= 0)
    Switches r_btn = Switches(R_BTN, 1000, R_PULL);
  #endif
  #if (C_BTN >= 0)
    Switches c_btn = Switches(C_BTN, 1000, C_PULL);
  #endif

#endif

WiFiScan wifi_scan_obj;
EvilPortal evil_portal_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;

#ifdef HAS_GPS
  GpsInterface gps_obj;
#endif

#ifdef HAS_BATTERY
  BatteryInterface battery_obj;
#endif

#ifdef HAS_SCREEN
  Display display_obj;
  MenuFunctions menu_function_obj;
#endif

#if defined(HAS_SD) && !defined(HAS_C5_SD)
  SDInterface sd_obj;
#endif

#ifdef MARAUDER_M5STICKC
  AXP192 axp192_obj;
#endif

#ifdef HAS_FLIPPER_LED
  flipperLED flipper_led;
#elif defined(XIAO_ESP32_S3)
  xiaoLED xiao_led;
#elif defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
  stickcLED stickc_led;
#elif defined(HAS_NEOPIXEL_LED)
  LedInterface led_obj;
#endif

const String PROGMEM version_number = MARAUDER_VERSION;

#ifdef HAS_NEOPIXEL_LED
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
#endif

uint32_t currentTime  = 0;

#ifdef MARAUDER_M5STICKCP2_FLIPPER
  TFT_eSPI flipper_status_tft = TFT_eSPI();
  uint32_t flipper_status_last_draw = 0;
  uint32_t flipper_status_last_activity = 0;
  bool flipper_status_dimmed = false;
  bool flipper_status_off = false;
  uint8_t flipper_status_prev_mode = 255;
  uint8_t flipper_status_prev_channel = 255;
  bool flipper_status_prev_scanning = false;
  static const uint16_t FLIPPER_ORANGE = 0xFD20;
  static const uint16_t FLIPPER_BG = TFT_BLACK;
  static const int FLIPPER_FP = 1;
  static const int FLIPPER_LH = 8;
  static const uint32_t FLIPPER_DIM_TIMEOUT = 20000;
  static const uint32_t FLIPPER_OFF_TIMEOUT = 25000;
  static const uint8_t FLIPPER_MIN_BRIGHT = 190;

  int flipper_status_prev_battery = -1;

  int getFlipperBatteryPercent() {
    static bool battery_pin_initialized = false;
    if (!battery_pin_initialized) {
      pinMode(38, INPUT);
      battery_pin_initialized = true;
    }

    uint32_t adcReading = analogReadMilliVolts(38);
    float actualVoltage = (float)adcReading * 2.0f;
    const float MIN_VOLTAGE = 3300.0f;
    const float MAX_VOLTAGE = 4150.0f;
    float percent = ((actualVoltage - MIN_VOLTAGE) / (MAX_VOLTAGE - (MIN_VOLTAGE + 50.0f))) * 100.0f;

    if (percent < 1.0f) percent = 1.0f;
    if (percent > 100.0f) percent = 100.0f;
    return (int)percent;
  }

  void flipperSetBrightness(uint8_t brightval) {
    if (brightval == 0) {
      analogWrite(TFT_BL, 0);
    } else {
      int bl = FLIPPER_MIN_BRIGHT + round(((255 - FLIPPER_MIN_BRIGHT) * brightval / 100.0f));
      analogWrite(TFT_BL, bl);
    }
  }

  void flipperWakeScreen() {
    flipper_status_last_activity = millis();
    if (flipper_status_off || flipper_status_dimmed) {
      flipper_status_off = false;
      flipper_status_dimmed = false;
      flipperSetBrightness(100);
    }
  }

  void flipperPowerSaveTick() {
    uint32_t elapsed = millis() - flipper_status_last_activity;
    if (!flipper_status_dimmed && elapsed >= FLIPPER_DIM_TIMEOUT) {
      flipper_status_dimmed = true;
      flipperSetBrightness(5);
    }
    if (!flipper_status_off && elapsed >= FLIPPER_OFF_TIMEOUT) {
      flipper_status_off = true;
      flipperSetBrightness(0);
    }
  }

  bool flipperAnyWakeButtonPressed() {
    return (digitalRead(35) == LOW) || (digitalRead(37) == LOW) || (digitalRead(39) == LOW);
  }

  void drawFlipperBatteryStatus(int bat) {
    int screen_w = flipper_status_tft.width();
    flipper_status_tft.drawRoundRect(screen_w - 42, 7, 34, FLIPPER_FP * FLIPPER_LH + 9, 2, FLIPPER_ORANGE);
    flipper_status_tft.setTextSize(1);
    flipper_status_tft.setTextColor(FLIPPER_ORANGE, FLIPPER_BG);
    flipper_status_tft.drawRightString("  " + String(bat) + "%", screen_w - 45, 12, 1);
    flipper_status_tft.fillRoundRect(screen_w - 40, 9, 30, FLIPPER_FP * FLIPPER_LH + 5, 2, FLIPPER_BG);
    flipper_status_tft.fillRoundRect(screen_w - 40, 9, 30 * bat / 100, FLIPPER_FP * FLIPPER_LH + 5, 2, FLIPPER_ORANGE);
    flipper_status_tft.drawLine(screen_w - 30, 9, screen_w - 30, 9 + FLIPPER_FP * FLIPPER_LH + 6, FLIPPER_BG);
    flipper_status_tft.drawLine(screen_w - 20, 9, screen_w - 20, 9 + FLIPPER_FP * FLIPPER_LH + 6, FLIPPER_BG);
  }

  void drawFlipperStatusFrame() {
    int screen_w = flipper_status_tft.width();
    int screen_h = flipper_status_tft.height();
    flipper_status_tft.fillScreen(TFT_BLACK);
    flipper_status_tft.setTextDatum(0);
    flipper_status_tft.setTextSize(FLIPPER_FP);
    flipper_status_tft.drawRoundRect(5, 5, screen_w - 10, screen_h - 10, 5, FLIPPER_ORANGE);
    flipper_status_tft.setTextColor(FLIPPER_ORANGE, FLIPPER_BG);
    flipper_status_tft.drawString("Marauder", 12, 12, 1);
    flipper_status_tft.drawLine(5, (6 + 6 + FLIPPER_FP * FLIPPER_LH + 5), screen_w - 6, (6 + 6 + FLIPPER_FP * FLIPPER_LH + 5), FLIPPER_ORANGE);
  }

  void drawFlipperStatusContent(bool force = false) {
    int bat = getFlipperBatteryPercent();

    flipper_status_tft.setTextColor(TFT_WHITE, FLIPPER_BG);
    flipper_status_tft.fillRect(12, 34, 180, 12, FLIPPER_BG);
    flipper_status_tft.drawString("Flipper Mode", 12, 34, 1);

    flipper_status_tft.setTextColor(FLIPPER_ORANGE, FLIPPER_BG);
    flipper_status_tft.fillRect(12, 46, 180, 12, FLIPPER_BG);
    flipper_status_tft.drawString(flipperModeName(wifi_scan_obj.currentScanMode), 12, 46, 1);

    flipper_status_tft.setTextColor(TFT_LIGHTGREY, FLIPPER_BG);
    flipper_status_tft.fillRect(12, 58, 120, 12, FLIPPER_BG);
    flipper_status_tft.drawString("CH: " + String(wifi_scan_obj.set_channel), 12, 58, 1);

    flipper_status_tft.setTextColor(TFT_WHITE, FLIPPER_BG);
    flipper_status_tft.fillRect(12, 70, 180, 12, FLIPPER_BG);
    flipper_status_tft.drawString(wifi_scan_obj.scanning() ? "UART active" : "Waiting command", 12, 70, 1);

    if (force || bat != flipper_status_prev_battery) {
      drawFlipperBatteryStatus(bat);
      flipper_status_prev_battery = bat;
    }
  }

  const char* flipperModeName(uint8_t mode) {
    switch (mode) {
      case WIFI_SCAN_OFF: return "Idle";
      case WIFI_SCAN_AP: return "Scan AP";
      case WIFI_SCAN_ALL: return "Scan All";
      case WIFI_SCAN_DEAUTH: return "Sniff Deauth";
      case WIFI_SCAN_PROBE: return "Sniff Probe";
      case WIFI_SCAN_EAPOL: return "Sniff PMKID";
      case WIFI_SCAN_RAW_CAPTURE: return "Sniff Raw";
      case WIFI_SCAN_PACKET_RATE: return "Packet Count";
      case WIFI_SCAN_WAR_DRIVE: return "Wardrive";
      case WIFI_SCAN_GPS_DATA: return "GPS Data";
      case WIFI_SCAN_GPS_NMEA: return "NMEA Stream";
      case BT_SCAN_ALL: return "BT Scan";
      case BT_SCAN_FLIPPER: return "BT Flipper";
      case BT_SCAN_AIRTAG: return "BT Airtag";
      default: return "Running";
    }
  }

  void drawFlipperStatusScreen(bool force = false) {
    if (force) drawFlipperStatusFrame();
    drawFlipperStatusContent(force);
  }
#endif

// PWM Brightness Control
#ifdef HAS_SCREEN
  #include <Preferences.h>
  #define BL_CHANNEL 0
  #define BL_FREQ 5000
  #define BL_RESOLUTION 8
  const uint8_t BL_LEVELS[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255};
  const uint8_t BL_NUM_LEVELS = 10;
  uint8_t bl_level_idx = 9; // default full brightness
  Preferences bl_prefs;
#endif

// Helper macros for LEDC API compatibility (2.x vs 3.x board package)
#ifdef HAS_SCREEN
  #ifndef HAS_MINI_SCREEN
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
      #define BL_SETUP()       ledcAttach(TFT_BL, BL_FREQ, BL_RESOLUTION)
      #define BL_SET(duty)     ledcWrite(TFT_BL, (duty))
    #else
      #define BL_SETUP()       do { ledcSetup(BL_CHANNEL, BL_FREQ, BL_RESOLUTION); ledcAttachPin(TFT_BL, BL_CHANNEL); } while(0)
      #define BL_SET(duty)     ledcWrite(BL_CHANNEL, (duty))
    #endif
  #endif
#endif

#ifndef HAS_MINI_SCREEN
  void brightnessInit() {
    #ifdef HAS_SCREEN
      BL_SETUP();
      bl_prefs.begin("backlight", false);
      bl_level_idx = bl_prefs.getUChar("level", 9);
      if (bl_level_idx >= BL_NUM_LEVELS) bl_level_idx = 9;
      BL_SET(BL_LEVELS[bl_level_idx]);
    #endif
  }

  void brightnessCycle() {
    #ifdef HAS_SCREEN
      bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
      Serial.print(F("[Brightness] Level "));
      Serial.print(bl_level_idx + 1);
      Serial.print(F("/"));
      Serial.print(BL_NUM_LEVELS);
      Serial.print(F(" ("));
      Serial.print(BL_LEVELS[bl_level_idx] * 100 / 255);
      Serial.println(F("%)"));
    #endif
  }

  uint8_t getBrightnessLevel() {
    #ifdef HAS_SCREEN
      return bl_level_idx;
    #else
      return 0;
    #endif
  }

  void brightnessSave(uint8_t level) {
    #ifdef HAS_SCREEN
      if (level >= BL_NUM_LEVELS) level = BL_NUM_LEVELS - 1;
      bl_level_idx = level;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
    #endif
  }

  void backlightOn() {
    #ifdef HAS_SCREEN
      BL_SET(BL_LEVELS[bl_level_idx]);
    #endif
  }

  void backlightOff() {
    #ifdef HAS_SCREEN
      BL_SET(0);
    #endif
  }
#else
  void backlightOn() {
    #ifdef HAS_SCREEN
      #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3) && !defined(DUAL_MINI_C5)
        digitalWrite(TFT_BL, LOW);
      #endif

      #if defined(DUAL_MINI_C5)
        digitalWrite(TFT_BL, HIGH);
      #endif
    
      #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, HIGH);
      #endif
    #endif
  }

  void backlightOff() {
    #ifdef HAS_SCREEN
      #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3) && !defined(DUAL_MINI_C5)
        digitalWrite(TFT_BL, HIGH);
      #endif

      #if defined(DUAL_MINI_C5)
        digitalWrite(TFT_BL, LOW);
      #endif
    
      #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, LOW);
      #endif
    #endif
  }
#endif

#ifdef HAS_C5_SD
  SPIClass sharedSPI(SPI);
  SDInterface sd_obj = SDInterface(&sharedSPI, SD_CS);
#endif

void setup()
{
  randomSeed(esp_random());
  
  #ifndef DEVELOPER
    esp_log_level_set("*", ESP_LOG_NONE);
  #endif
  
  #ifndef HAS_IDF_3
    esp_spiram_init();
  #endif

  #if defined(MARAUDER_M5STICKCP2_FLIPPER)
    Serial.begin(115200, SERIAL_8N1, /*RX=*/36, /*TX=*/26);
  #else
    Serial.begin(115200);
  #endif

  while(!Serial)
    delay(10);

  #ifdef HAS_C5_SD
    sharedSPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    delay(100);
  #endif

  #ifdef defined(MARAUDER_M5STICKC) && !defined(MARAUDER_M5STICKCP2)
    axp192_obj.begin();
  #endif

  #if defined(MARAUDER_M5STICKCP2) // Prevent StickCP2 from turning off when disconnect USB cable
    pinMode(POWER_HOLD_PIN, OUTPUT);
    digitalWrite(POWER_HOLD_PIN, HIGH);
  #endif

  #ifdef MARAUDER_M5STICKCP2_FLIPPER
    pinMode(TFT_BL, OUTPUT);
    flipperSetBrightness(100);
    flipper_status_tft.init();
    flipper_status_tft.setRotation(3);
    delay(50);
    flipper_status_last_activity = millis();
    drawFlipperStatusScreen(true);
    flipper_status_prev_mode = wifi_scan_obj.currentScanMode;
    flipper_status_prev_channel = wifi_scan_obj.set_channel;
    flipper_status_prev_scanning = wifi_scan_obj.scanning();
  #endif

  #ifdef HAS_SCREEN
    pinMode(TFT_BL, OUTPUT);
  #endif
  
  backlightOff();
  #if BATTERY_ANALOG_ON == 1
    pinMode(BATTERY_PIN, OUTPUT);
    pinMode(CHARGING_PIN, INPUT);
  #endif
  
  // Preset SPI CS pins to avoid bus conflicts
  #ifdef HAS_SCREEN
    digitalWrite(TFT_CS, HIGH);
  #endif
  
  #if defined(HAS_SD) && !defined(HAS_C5_SD)
    pinMode(SD_CS, OUTPUT);

    delay(10);
  
    digitalWrite(SD_CS, HIGH);

    delay(10);
  #endif

  //Serial.begin(115200);

  //while(!Serial)
  //  delay(10);

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

  #ifdef HAS_PSRAM
    if (!psramInit()) {
      Serial.println(F("PSRAM not available"));
    }
  #endif

  #ifdef HAS_SIMPLEX_DISPLAY
    #if defined(HAS_SD)
      // Do some SD stuff
      if(!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));

    #endif
  #endif

  #ifdef HAS_SCREEN
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  // Init PWM brightness AFTER display init (so ledcAttach overrides TFT_eSPI's pinMode)
  #ifndef HAS_MINI_SCREEN
    brightnessInit();
    backlightOff();
  #endif

  #ifdef HAS_SCREEN
    #ifndef MARAUDER_CARDPUTER
      display_obj.tft.drawCentreString("ESP32 Marauder", TFT_WIDTH/2, TFT_HEIGHT * 0.33, 1);
      display_obj.tft.drawCentreString("JustCallMeKoko", TFT_WIDTH/2, TFT_HEIGHT * 0.5, 1);
      display_obj.tft.drawCentreString(display_obj.version_number, TFT_WIDTH/2, TFT_HEIGHT * 0.66, 1);
    #else
      display_obj.tft.drawCentreString("ESP32 Marauder", TFT_HEIGHT/2, TFT_WIDTH * 0.33, 1);
      display_obj.tft.drawCentreString("JustCallMeKoko", TFT_HEIGHT/2, TFT_WIDTH * 0.5, 1);
      display_obj.tft.drawCentreString(display_obj.version_number, TFT_HEIGHT/2, TFT_WIDTH * 0.66, 1);
    #endif
  #endif


  backlightOn(); // Need this

  #ifdef HAS_SCREEN
    #ifdef MARAUDER_M5STICKCP2_FLIPPER
      display_obj.headless_mode = true;
    #endif

    // Do some stealth mode stuff
    #ifdef HAS_BUTTONS
      if (c_btn.justPressed()) {
        display_obj.headless_mode = true;

        backlightOff();
      }
    #endif
  #endif

  settings_obj.begin();

  if (settings_obj.getSettingType("ChanHop") == "") {
    Serial.println(F("Current settings format not supported. Installing new default settings..."));
    settings_obj.createDefaultSettings(SPIFFS);
  }

  buffer_obj = Buffer();

  #ifndef HAS_SIMPLEX_DISPLAY
    #if defined(HAS_SD)
      // Do some SD stuff
      if(!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));

    #endif
  #endif

  wifi_scan_obj.RunSetup();

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawCentreString("Initializing...", TFT_WIDTH/2, TFT_HEIGHT * 0.82, 1);
  #endif

  evil_portal_obj.setup();

  #ifdef HAS_BATTERY
    battery_obj.RunSetup();
  #endif

  #ifdef HAS_BATTERY
    battery_obj.battery_level = battery_obj.getBatteryLevel();
  #endif

  // Do some LED stuff
  #ifdef HAS_FLIPPER_LED
    flipper_led.RunSetup();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.RunSetup();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.RunSetup();
  #elif defined(HAS_NEOPIXEL_LED)
    led_obj.RunSetup();
  #endif

  #ifdef HAS_GPS
    gps_obj.begin();
  #endif

  #ifdef HAS_SCREEN  
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  #ifdef HAS_SCREEN
    menu_function_obj.RunSetup();
  #endif

  /*char ssidBuf[64] = {0};  // or prefill with existing SSID
  if (keyboardInput(ssidBuf, sizeof(ssidBuf), "Enter SSID")) {
    // user pressed OK
    Serial.println(ssidBuf);
  } else {
    Serial.println(F("User exited keyboard"));
  }

  menu_function_obj.changeMenu(menu_function_obj.current_menu);*/

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  
  cli_obj.RunSetup();
}


void loop()
{
  currentTime = millis();
  bool mini = false;

  #ifdef SCREEN_BUFFER
    #ifndef HAS_ILI9341
      mini = true;
    #endif
  #endif

  #if (defined(HAS_ILI9341) && !defined(MARAUDER_CYD_2USB))
    #ifdef HAS_BUTTONS
      if (c_btn.isHeld()) {
        if (menu_function_obj.disable_touch)
          menu_function_obj.disable_touch = false;
        else
          menu_function_obj.disable_touch = true;

        menu_function_obj.updateStatusBar();

        while (!c_btn.justReleased())
          delay(1);
      }
    #endif
  #endif

  // Update all of our objects
  cli_obj.main(currentTime);
  wifi_scan_obj.main(currentTime);

  #ifdef HAS_GPS
    gps_obj.main();
  #endif

  // Save buffer to SD and/or serial
  buffer_obj.save();

  #ifdef HAS_BATTERY
    battery_obj.main(currentTime);
  #endif
  if ((wifi_scan_obj.currentScanMode != WIFI_PACKET_MONITOR) ||
      (mini)) {
    #ifdef HAS_SCREEN
      #ifndef MARAUDER_M5STICKCP2_FLIPPER
        menu_function_obj.main(currentTime);
      #endif
    #endif
  }
  #ifdef HAS_FLIPPER_LED
    flipper_led.main();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.main();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.main();
  #elif defined(HAS_NEOPIXEL_LED)
    led_obj.main(currentTime);
  #endif

  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif

  #ifdef MARAUDER_M5STICKCP2_FLIPPER
    if (flipperAnyWakeButtonPressed()) {
      bool was_sleeping = flipper_status_dimmed || flipper_status_off;
      flipperWakeScreen();
      if (was_sleeping) {
        drawFlipperStatusScreen(true);
        flipper_status_last_draw = currentTime;
      }
    }

    bool flipper_mode_changed = (flipper_status_prev_mode != wifi_scan_obj.currentScanMode);
    bool flipper_status_changed =
      flipper_mode_changed ||
      (flipper_status_prev_channel != wifi_scan_obj.set_channel) ||
      (flipper_status_prev_scanning != wifi_scan_obj.scanning());

    if (flipper_mode_changed) {
      flipperWakeScreen();
    }

    if (flipper_status_changed && !flipper_status_off) {
      drawFlipperStatusScreen();
      flipper_status_last_draw = currentTime;
      flipper_status_prev_mode = wifi_scan_obj.currentScanMode;
      flipper_status_prev_channel = wifi_scan_obj.set_channel;
      flipper_status_prev_scanning = wifi_scan_obj.scanning();
    } else if (!flipper_status_off && currentTime - flipper_status_last_draw >= 30000) {
      drawFlipperStatusScreen();
      flipper_status_last_draw = currentTime;
    }

    flipperPowerSaveTick();
  #endif
}
