/**
 * RDA5807M FM Radio with RDS, Ky-040 Rotary Encoder, ST7735 TFT Display and DS3231 RTC
 *
 * ST7735 TFT Display pinout
 * LED: 3.3V
 * SCLK: PICO SPI0 SCK (Pin 2)
 * SDA: PICO SPI0 MOSI (Pin 3)
 * A0: PICO GPIO4 (Pin 4)
 * RESET: PICO GPIO5 (Pin 5)
 * CS: PICO GPIO6 (Pin 6)
 * GND: GND
 * VCC: 3.3V
 *
 * RDA5807M pinout:
 * GND: GND
 * 3.3V: 3.3V
 * SDA: PICO I2C1 SDA (Pin 0) - 3.3V-5V szintillesztővel csatlakoztatva!
 * SCL: PICO I2C1 SCL (Pin 1)- 3.3V-5V szintillesztővel csatlakoztatva!
 *
 *
 * Ky-040 Rotary Encoder pinout:
 * CLK: PICO GPIO22 (Pin 22)
 * DT: PICO GPIO21 (Pin 21)
 * SW: PICO GPIO20 (Pin 20)
 * +: 3.3V  (VCC)
 * -: GND   (GND)
 *
 *
 */

//------------------- 1.44" TFT ST7735
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#define TFT_DC 4
#define TFT_CS 6
#define TFT_RST 5
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, 3, 2, TFT_RST);

// Saját ST7735 színek definiálása
#define ST77XX_LIGHTGRAY 0xC618
#define ST77XX_DARKGRAY 0x7BEF
#define ST77XX_DARKCYAN 0x03EF
#define ST77XX_PURPLE 0xF81F

// Text méretek:
// 1: 6x8 (default)
// 2: 12x16
// 3: 18x24
// etc
#define TEXT_SIZE_1_WIDTH 6
#define TEXT_SIZE_1_HEIGHT 8
#define TEXT_SIZE_2_WIDTH 12
#define TEXT_SIZE_2_HEIGHT 16
#define TEXT_SIZE_3_WIDTH 18
#define TEXT_SIZE_3_HEIGHT 24

//------------------- DS3231 RTC
#include <DS3231.h>
DS3231 rtc;
bool century = false;
bool hourFormat = false;

//------------------- RDA5807 Rádió
#include <RDA5807.h>
#include <Wire.h> //Az SPI display miatt átirányítjuk a defualt pint-eket (4-I2CO SDA, 5-I2C0 SCL) az 0-I2C1 SDA, 1-I2C1 SCL pinekrere
RDA5807 radio;
uint16_t frequency = 9390; // Petőfi Rádió
#define POLLING_TIME 3000
#define POLLING_RDS 20
#define PUSH_MIN_DELAY 300 // Minimum waiting time after an action
const char *RDS_PROGRAM_TYPE_ARRAY[] = {"Not defined", "News", "Current affairs", "Info", "Sport", "Education", "Drama", "Culture", "Science", "Varied", "POP", "Rock", "Easy Listening", "Light Classical", "Serious Classical", "Other Music",
                                        "Weather", "Finance", "Children's Programmes", "Social Affairs", "Religion", "Phone-in", "Travel", "Leisure", "Jazz", "Country Music", "National Music", "Oldies Music", "Folk Music", "Documentary", "Alarm Test", "Alarm"};
// Tömb méretének meghatározása
#define NUMBER_OF_CHAR_ARRAY_ELEMENT(x) (sizeof(x) / sizeof(x[0]))

//------------------- Rotary Encoder
#define PIN_ENCODER_CLK 22
#define PIN_ENCODER_DT 21
#define PIN_ENCODER_SW 20
#include "RotaryEncoder.h"
RotaryEncoder *pRotaryEncoder;

// Pico Hardware timer a Rotary encoder olvasására
#include <RPi_Pico_TimerInterrupt.h>
RPI_PICO_Timer ITimer1(1);
#define TIMER1_INTERVAL_USEC 1000 * 5 // 5ms

/*********************************************************
   RDS
 *********************************************************/
// Kell az RDS?
bool needRds = true;

// RDS adatok tárolása karaktertömbökben
uint8_t rdsProgramType;
char *pProgramInfo = NULL;
char *pStationName = NULL;
char *pRdsTime = NULL;

// Előzőleg megjelenített értékek tárolása
int prevRssi = -1;
bool prevStereo = false;
float prevFrequency = -1.0;
char prevStationName[9] = "";
char prevProgramInfo[65] = "";
char prevRdsTime[6] = "";
int prevRdsProgramType = -1;

/**
 * Clear RDS information
 */
void clearRds() {
    rdsProgramType = -1;
    pProgramInfo = NULL;
    pStationName = NULL;
    pRdsTime = NULL;

    // ELőző RSD értékek törlése
    memset(prevStationName, 0, sizeof(prevStationName));
    prevStationName[0] = 255; // beállítjuk a tömb első elemét 255-re, hogy ezt mi töröltük
    memset(prevProgramInfo, 0, sizeof(prevProgramInfo));
    prevProgramInfo[0] = 255; // beállítjuk a tömb első elemét 255-re, hogy ezt mi töröltük
    memset(prevRdsTime, 0, sizeof(prevRdsTime));
    prevRdsTime[0] = 255; // beállítjuk a tömb első elemét 255-re, hogy ezt mi töröltük

    prevRdsProgramType = -1;
}

/**
 * Jobb oldali szóközök levágása a szövegből
 */
void trimRight(char *s) {
    int i = strlen(s) - 1;
    while (i >= 0 && isspace(s[i])) {
        s[i] = '\0';
        i--;
    }
}

/**
 * Check RDS information
 */
void checkRDS() {

    if (radio.getRdsReady()) {
        if (radio.hasRdsInfo()) {
            rdsProgramType = radio.getRdsProgramType();
            pProgramInfo = radio.getRdsProgramInformation();
            pStationName = radio.getRdsStationName();
            pRdsTime = radio.getRdsLocalTime();
        }
    }
}

/*********************************************************
   Seek
 *********************************************************/
/**
 *  Process seek command.
 */
void radioSeek() {
    // displayFrequency() will be called by the seek function during the process.
    radio.seek(RDA_SEEK_WRAP, RDA_SEEK_UP, displayFrequency);
    clearRds();
    delay(PUSH_MIN_DELAY);
}

/*********************************************************
   Display
 *********************************************************/

/**
 * Szöveg szélességének lekérdezése pixelben
 */
uint16_t getTextWidth(const char *text) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

/**
 * Frekvencia megjelenítése
 * A seek alatti kijelzés miatt külön függvény
 */
void displayFrequency() {
#define TXT_FREQUENCY_COL 10
#define TXT_FREQUENCY_ROW 20
    // tft.fillRect(TXT_FREQUENCY_COL, TXT_FREQUENCY_ROW, 100, TEXT_SIZE_3_HEIGHT, ST77XX_BLACK); // korábbi adatok törlése
    tft.setTextSize(3);
    tft.setCursor(TXT_FREQUENCY_COL, TXT_FREQUENCY_ROW);
    tft.setTextColor(ST77XX_PURPLE, ST77XX_BLACK);
    tft.print(radio.getFrequency() / 100.0, 1);
}

/**
 * Statikus adatok megjelenítése
 */
void displayInit() {
#define TXT_STEREO_COL 60
#define TXT_RDS_COL 100

    tft.fillScreen(ST77XX_BLACK);

    tft.setTextColor(ST77XX_LIGHTGRAY, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.print(F("rssi"));
    tft.drawPixel(23, 2, ST77XX_LIGHTGRAY); // kettőspont felső pixel
    tft.drawPixel(23, 5, ST77XX_LIGHTGRAY); // kettőspont alsó pixel

    tft.setTextColor(ST77XX_DARKGRAY, ST77XX_BLACK);
    tft.setCursor(TXT_STEREO_COL, 0);
    tft.print(F("ST"));

    tft.setCursor(TXT_RDS_COL, 0);
    tft.print(F("RDS"));

    tft.setTextColor(ST77XX_LIGHTGRAY, ST77XX_BLACK);
    tft.setCursor(110, TXT_FREQUENCY_ROW + 10);
    tft.print(F("MHz"));

    tft.drawFastHLine(0, 10, tft.width(), ST77XX_DARKCYAN);
}

/**
 * Változó adatok megjelenítése
 * Csak akkor frissítjük a kijelzőt, ha az új értékek eltérnek a korábbiaktól, így elkerülve a villogást.
 */
void displayData() {

    tft.setTextSize(1);

// RSSI érték kiírása
#define RSSI_X 30
    int currentRssi = radio.getRssi();
    if (currentRssi != prevRssi) {
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(RSSI_X, 0);
        tft.print(currentRssi);
        prevRssi = currentRssi;
    }

    // Sztereo kiírása
    bool currentStereo = radio.isStereo();
    if (currentStereo != prevStereo) {
        tft.setCursor(TXT_STEREO_COL, 0);
        tft.setTextColor(currentStereo ? ST77XX_GREEN : ST77XX_DARKGRAY, ST77XX_BLACK);
        tft.print(F("ST"));
        prevStereo = currentStereo;
    }

    // A frekvencia kiírása
    float currentFrequency = radio.getFrequency() / 100.0;
    if (currentFrequency != prevFrequency) {
        displayFrequency();
        prevFrequency = currentFrequency;
    }

    // RDS infók kiírása, ha be van kapcsolva az RDS
    if (needRds) {
        // RDS szinkronizáció megjelenítése a fejlécben
        tft.setTextColor(radio.getRdsSync() ? ST77XX_GREEN : ST77XX_DARKGRAY, ST77XX_BLACK);
        tft.setTextSize(1);
        tft.setCursor(TXT_RDS_COL, 0);
        tft.print(F("RDS"));

        // Text színe fehérre állítása
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

// Station Name kiírása
#define TXT_STATION_NAME_ROW 60
        if (prevStationName[0] == 255) {
            tft.fillRect(0, TXT_STATION_NAME_ROW, tft.width(), TEXT_SIZE_2_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
            prevStationName[0] = 0;
        } else if (pStationName != NULL && strcmp(pStationName, prevStationName) != 0) {
            tft.fillRect(0, TXT_STATION_NAME_ROW, tft.width(), TEXT_SIZE_2_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
            tft.setTextSize(2);
            tft.setCursor(0, TXT_STATION_NAME_ROW);
            tft.print(pStationName);
            strncpy(prevStationName, pStationName, sizeof(prevStationName) - 1);
        }

// ProgramInfo kiírása
#define TXT_PROGRAM_INFO_ROW 80
#define SCROLL_PIXEL 3
        static int scrollPosition;
        static uint16_t prevProgramInfoPixelWidth = 0;

        tft.setTextSize(1);
        tft.setCursor(0, TXT_PROGRAM_INFO_ROW);

        if (prevProgramInfo[0] == 255) {                                                          // kézzel töröltük a program infót?
            tft.fillRect(0, TXT_PROGRAM_INFO_ROW, tft.width(), TEXT_SIZE_1_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
            prevProgramInfo[0] = 0;

        } else if (pProgramInfo != NULL && strcmp(pProgramInfo, prevProgramInfo) != 0) { // új adat jött?

            tft.fillRect(0, TXT_PROGRAM_INFO_ROW, tft.width(), TEXT_SIZE_1_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
            strncpy(prevProgramInfo, pProgramInfo, sizeof(prevProgramInfo) - 1);                  // Lemásoljuk az új adatot a régi adatba
            trimRight(prevProgramInfo);                                                           // Jobb oldali szóközök levágása

            // Új szöveg kiírása után a görgetési pozíció beállítása a képernyő végére
            prevProgramInfoPixelWidth = getTextWidth(prevProgramInfo); // Szöveg méreteinek lekérdezése

            // Ha a szöveg szélessége nagyobb, mint a kijelző szélessége, akkor elkezdjük a görgetést
            if (prevProgramInfoPixelWidth > tft.width()) {
                scrollPosition = SCROLL_PIXEL;
            } else {
                // Csak kiírjuk a szöveget, ha az nem nagyobb, mint a kijelző szélessége
                tft.setCursor(0, TXT_PROGRAM_INFO_ROW);
                tft.print(prevProgramInfo);
            }
        }

        // Ha a szöveg szélessége nagyobb, mint a kijelző szélessége, akkor görgetünk
        if (prevProgramInfo[0] != 0 && prevProgramInfoPixelWidth > tft.width()) {

            // Görgetés balra
            scrollPosition -= SCROLL_PIXEL;

            // Ha a szöveg végére értünk, akkor jobbról kezdjük újra a görgetést
            if (scrollPosition + prevProgramInfoPixelWidth < 0) {
                tft.fillRect(0, TXT_PROGRAM_INFO_ROW, tft.width(), TEXT_SIZE_1_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
                scrollPosition = tft.width();                                                         // Újra kezdjük a görgetést jobb oldalról
            }

            tft.setCursor(scrollPosition, TXT_PROGRAM_INFO_ROW);
            tft.print(prevProgramInfo);
        }

// Program Type kiírása
#define TXT_PROGRAM_TYPE_ROW 90
        if (rdsProgramType != prevRdsProgramType) {
            tft.fillRect(0, TXT_PROGRAM_TYPE_ROW, tft.width(), TEXT_SIZE_1_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
            tft.setTextSize(1);
            tft.setCursor(0, TXT_PROGRAM_TYPE_ROW);
            if (rdsProgramType != -1 && rdsProgramType <= NUMBER_OF_CHAR_ARRAY_ELEMENT(RDS_PROGRAM_TYPE_ARRAY)) {
                tft.print(RDS_PROGRAM_TYPE_ARRAY[rdsProgramType]);
            }
            prevRdsProgramType = rdsProgramType;
        }
    }
}

/**
 * Rotary encoder controller
 */
void rotaryController(RotaryEncoder::Button buttonState, RotaryEncoder::Direction direction) {

    switch (buttonState) {
    case RotaryEncoder::Released:
        Serial.println("Button Released from held");
        break;

    case RotaryEncoder::Clicked:
        radioSeek();
        break;
    case RotaryEncoder::DoubleClicked:
        Serial.println("Button  DoubleClicked");
        break;
    }

    if (buttonState == RotaryEncoder::Open) {
        switch (direction) {
        case RotaryEncoder::Direction::UP:
            radio.setFrequencyUp();
            clearRds();
            displayFrequency();
            break;
        case RotaryEncoder::Direction::DOWN:
            radio.setFrequencyDown();
            clearRds();
            displayFrequency();
            break;
        }
    }
}

/**
 * Hardware timer interrupt service routine
 */
bool HardwareTimerHandler1(struct repeating_timer *t) {
    pRotaryEncoder->service();
    return true;
}

/**
 * Setup
 */
void setup() {
    Serial.begin(115200);

    // TFT display inicializálása
    tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
    tft.setTextWrap(false);       // Az RDS szöveg görgetéséhez kell, hogy ne törje a sorokat
    // tft.setRotation(3);
    displayInit();

    // Az SPI display miatt átirányítjuk a defualt pint-eket (4-I2CO SDA, 5-I2C0SCL) az 0-I2C1 SDA, 1-I2C1 SCL pinekrere
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    // Rotary Encoder felhúzása
    pRotaryEncoder = new RotaryEncoder(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);
    pRotaryEncoder->setDoubleClickEnabled(true);
    pRotaryEncoder->setAccelerationEnabled(false);

    // Pico HW Timer1 beállítása
    ITimer1.attachInterruptInterval(TIMER1_INTERVAL_USEC, HardwareTimerHandler1);

    // Radio beállítása
    radio.setup();
    radio.powerUp();            // Power up the radio
    radio.setVolume(3);         // Hangerő beállítása
    radio.setMono(false);       // Sztereó beállítása
    radio.setMute(false);       // Némítás kikapcsolása
    radio.setBass(true);        // Basszus bekapcsolása
    radio.setRDS(true);         // RDS bekapcsolása
    radio.setRBDS(true);        // RBDS bekapcsolása
    radio.setRdsFifo(true);     // RDS FIFO
    radio.setSeekThreshold(40); // Sets RSSI Seek Threshold (0 to 127)
    radio.setLnaPortSel(3);     // Sets the LNA Port Selection (0 to 3)
    radio.setFrequency(frequency);
}

/**
 * Loop
 */
void loop() {

    static unsigned long prevDisplay = millis(); // Az aktuális idő lekérése
    if (millis() - prevDisplay >= 50) {
        displayData();

        //     Serial.printf("Date: %04d-%02d-%02d, Time: %02d:%02d:%02d\n\r",
        //                   rtc.getYear(), rtc.getMonth(century), rtc.getDate(),
        //                   rtc.getHour(hourFormat, hourFormat), rtc.getMinute(), rtc.getSecond());
        //     Serial.printf("Temp: %02.2f\n\r", rtc.getTemperature());
        prevDisplay = millis();
    }

    static unsigned long polling_rds = millis();
    if ((millis() - polling_rds) > POLLING_RDS) {
        if (needRds) {
            checkRDS();
        }
        polling_rds = millis();
    }

    // Rotary Encoder olvasása
    RotaryEncoder::RotaryEncoderResult rotaryEncoderResult = pRotaryEncoder->readRotaryEncoder();

    // Ha klikkeltek VAGY van tekerés, akkor bizony piszkáltuk
    if (rotaryEncoderResult.buttonState != RotaryEncoder::Open || rotaryEncoderResult.direction != RotaryEncoder::Direction::NONE) {
        rotaryController(rotaryEncoderResult.buttonState, rotaryEncoderResult.direction);
    }
}
