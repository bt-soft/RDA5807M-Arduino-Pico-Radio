# 1 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
//------------------- 1.44" TFT ST7735
# 3 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 2
# 4 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 2




Adafruit_ST7735 tft = Adafruit_ST7735(6, 4, 3, 2, 5);

// Saját ST7735 színek definiálása




// Text méretek:
// 1: 6x8 (default)
// 2: 12x16
// 3: 18x24
// etc







//------------------- DS3231 RTC
# 29 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 2
DS3231 rtc;
bool century = false;
bool hourFormat = false;

//------------------- RDA5807 Rádió
# 35 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 2

RDA5807 radio;
uint16_t frequency = 9390; // Petőfi Rádió



const char *RDS_PROGRAM_TYPE_ARRAY[] = {"Not defined", "News", "Current affairs", "Info", "Sport", "Education", "Drama", "Culture", "Science", "Varied", "POP", "Rock", "Easy Listening", "Light Classical", "Serious Classical", "Other Music",
                                        "Weather", "Finance", "Children's Programmes", "Social Affairs", "Religion", "Phone-in", "Travel", "Leisure", "Jazz", "Country Music", "National Music", "Oldies Music", "Folk Music", "Documentary", "Alarm Test", "Alarm"};
// Tömb méretének meghatározása


//------------------- Rotary Encoder



# 51 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 2
RotaryEncoder *pRotaryEncoder;

// Pico Hardware timer a Rotary encoder olvasására
# 55 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 2
RPI_PICO_Timer ITimer1(1);


/*********************************************************

   RDS

 *********************************************************/
# 61 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
// Kell az RDS?
bool needRds = true;

// RDS adatok tárolása karaktertömbökben
uint8_t rdsProgramType;
char *pProgramInfo = 
# 66 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                    __null
# 66 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                        ;
char *pStationName = 
# 67 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                    __null
# 67 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                        ;
char *pRdsTime = 
# 68 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                __null
# 68 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                    ;

// Előzőleg megjelenített értékek tárolása
int prevRssi = -1;
bool prevStereo = false;
float prevFrequency = -1.0;
char prevStationName[9] = "";
char prevProgramInfo[65] = "";
char prevRdsTime[6] = "";
int prevRdsProgramType = -1;

// Görgetési pozíció tárolása
int scrollPosition = 0;

/**

 * Clear RDS information

 */
# 85 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void clearRds() {
    rdsProgramType = -1;
    pProgramInfo = 
# 87 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                  __null
# 87 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                      ;
    pStationName = 
# 88 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                  __null
# 88 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                      ;
    pRdsTime = 
# 89 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
              __null
# 89 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                  ;

    // ELőző RSD értékek törlése
    prevStationName[0] = 255;
    prevProgramInfo[0] = 255;
    prevRdsTime[0] = 255;

    prevRdsProgramType = -1;
}

/**

 * Check RDS information

 */
# 102 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
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
# 117 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
/**

 *  Process seek command.

 */
# 120 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void radioSeek() {
    // displayFrequency() will be called by the seek function during the process.
    radio.seek(0 /*!< Wrap at the upper or lower band limit and continue seeking*/, 1 /*!< Seek UP*/, displayFrequency);
    clearRds();
    delay(300 /* Minimum waiting time after an action*/);
}

/*********************************************************

   Display

 *********************************************************/
# 131 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
/**

 * Frekvencia megjelenítése

 * A seek alatti kijelzés miatt külön függvény

 */
# 135 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void displayFrequency() {


    // tft.fillRect(TXT_FREQUENCY_COL, TXT_FREQUENCY_ROW, 100, TEXT_SIZE_3_HEIGHT, ST77XX_BLACK); // korábbi adatok törlése
    tft.setTextSize(3);
    tft.setCursor(10, 30);
    tft.setTextColor(0xFFE0, 0x0000);
    tft.print(radio.getFrequency() / 100.0, 1);
}

/**

 * Statikus adatok megjelenítése

 */
# 148 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void displayInit() {



    tft.fillScreen(0x0000);

    tft.setTextColor(0xC618, 0x0000);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.print((reinterpret_cast<const __FlashStringHelper *>(("rssi"))));
    tft.drawPixel(23, 2, 0xC618); // kettőspont felső pixel
    tft.drawPixel(23, 5, 0xC618); // kettőspont alsó pixel

    tft.setTextColor(0x7BEF, 0x0000);
    tft.setCursor(60, 0);
    tft.print((reinterpret_cast<const __FlashStringHelper *>(("ST"))));

    tft.setCursor(100, 0);
    tft.print((reinterpret_cast<const __FlashStringHelper *>(("RDS"))));

    tft.setTextColor(0xC618, 0x0000);
    tft.setCursor(110, 30 + 16);
    tft.print((reinterpret_cast<const __FlashStringHelper *>(("MHz"))));

    tft.drawFastHLine(0, 10, tft.width(), 0x03EF);
}

/**

 * Változó adatok megjelenítése

 * Csak akkor frissítjük a kijelzőt, ha az új értékek eltérnek a korábbiaktól, így elkerülve a villogást.

 */
# 179 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void displayData() {

    tft.setTextSize(1);

// RSSI érték kiírása

    int currentRssi = radio.getRssi();
    if (currentRssi != prevRssi) {
        tft.setTextSize(1);
        tft.setTextColor(0xFFFF, 0x0000);
        tft.setCursor(30, 0);
        tft.print(currentRssi);
        prevRssi = currentRssi;
    }

    // Sztereo kiírása
    bool currentStereo = radio.isStereo();
    if (currentStereo != prevStereo) {
        tft.setCursor(60, 0);
        tft.setTextColor(currentStereo ? 0x07E0 : 0x7BEF, 0x0000);
        tft.print((reinterpret_cast<const __FlashStringHelper *>(("ST"))));
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
        tft.setTextColor(radio.getRdsSync() ? 0x07E0 : 0x7BEF, 0x0000);
        tft.setTextSize(1);
        tft.setCursor(100, 0);
        tft.print((reinterpret_cast<const __FlashStringHelper *>(("RDS"))));

        // Text színe fehérre állítása
        tft.setTextColor(0xFFFF, 0x0000);

// Station Name kiírása

        if (prevStationName[0] == 255) {
            tft.fillRect(0, 60, tft.width(), 16, 0x0000); // Korábbi adatok törlése
            prevStationName[0] = 0;
        } else if (pStationName != 
# 226 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                                  __null 
# 226 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                                       && strcmp(pStationName, prevStationName) != 0) {
            tft.fillRect(0, 60, tft.width(), 16, 0x0000); // Korábbi adatok törlése
            tft.setTextSize(2);
            tft.setCursor(0, 60);
            tft.print(pStationName);
            strncpy(prevStationName, pStationName, sizeof(prevStationName) - 1);
        }

// ProgramInfo kiírása


        tft.setTextSize(1);
        if (prevProgramInfo[0] == 255) {
            tft.fillRect(0, 80, tft.width(), 8, 0x0000); // Korábbi adatok törlése
            prevProgramInfo[0] = 0;
        } else if (pProgramInfo != 
# 241 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino" 3 4
                                  __null 
# 241 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
                                       && strcmp(pProgramInfo, prevProgramInfo) != 0) {
            tft.fillRect(0, 80, tft.width(), 8, 0x0000); // Korábbi adatok törlése
            tft.setCursor(0, 80);
            tft.print(pProgramInfo);
            strncpy(prevProgramInfo, pProgramInfo, sizeof(prevProgramInfo) - 1);
            scrollPosition = 0; // Reset scroll position when new info is received
        } else if (strlen(prevProgramInfo) > (tft.width() / 6)) {
            // Görgetés megvalósítása
            tft.setCursor(0, 80);
            if (scrollPosition < strlen(prevProgramInfo) - (tft.width() / 6)) {
                tft.print(prevProgramInfo + scrollPosition);
                scrollPosition++;
            } else {
                // Szöveg úszása a jobb oldalról
                int offset = scrollPosition - (strlen(prevProgramInfo) - (tft.width() / 6));
                tft.print(prevProgramInfo + offset);
                tft.setCursor(tft.width() - (offset * 6), 80);
                tft.print(prevProgramInfo + offset);
                scrollPosition++;
                if (scrollPosition >= strlen(prevProgramInfo) + (tft.width() / 6)) {
                    scrollPosition = 0; // Reset scroll position when end is reached
                }
            }
        } else {
            // Ha a szöveg rövidebb, mint a kijelző szélessége, ne görgessen
            // tft.fillRect(0, TXT_PROGRAM_INFO_ROW, tft.width(), TEXT_SIZE_1_HEIGHT, ST77XX_BLACK); // Korábbi adatok törlése
            tft.setCursor(0, 80);
            tft.print(prevProgramInfo);
        }

// Program Type kiírása

        if (rdsProgramType != prevRdsProgramType) {
            tft.fillRect(0, 110, tft.width(), 8, 0x0000); // Korábbi adatok törlése
            tft.setTextSize(1);
            tft.setCursor(0, 110);
            if (rdsProgramType != -1 && rdsProgramType <= (sizeof(RDS_PROGRAM_TYPE_ARRAY) / sizeof(RDS_PROGRAM_TYPE_ARRAY[0]))) {
                tft.print(RDS_PROGRAM_TYPE_ARRAY[rdsProgramType]);
            }
            prevRdsProgramType = rdsProgramType;
        }
    }
}

/**

 * Rotary encoder controller

 */
# 288 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
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
# 322 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
bool HardwareTimerHandler1(struct repeating_timer *t) {
    pRotaryEncoder->service();
    return true;
}

/**

 * Setup

 */
# 330 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void setup() {
    Serial.begin(115200);

    // TFT display inicializálása
    tft.initR(0x01); // Init ST7735R chip, green tab
    tft.setTextWrap(false);
    // tft.setRotation(3);
    displayInit();

    // Az SPI display miatt átirányítjuk a defualt pint-eket (4-I2CO SDA, 5-I2C0SCL) az 0-I2C1 SDA, 1-I2C1 SCL pinekrere
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    // Rotary Encoder felhúzása
    pRotaryEncoder = new RotaryEncoder(22, 21, 20);
    pRotaryEncoder->setDoubleClickEnabled(true);
    pRotaryEncoder->setAccelerationEnabled(false);

    // Pico HW Timer1 beállítása
    ITimer1.attachInterruptInterval(1000 * 5 /* 5ms*/, HardwareTimerHandler1);

    // Radio beállítása
    radio.setup();
    radio.powerUp(); // Power up the radio
    radio.setVolume(3); // Hangerő beállítása
    radio.setMono(false); // Sztereó beállítása
    radio.setMute(false); // Némítás kikapcsolása
    radio.setBass(true); // Basszus bekapcsolása
    radio.setRDS(true); // RDS bekapcsolása
    radio.setRBDS(true); // RBDS bekapcsolása
    radio.setRdsFifo(true); // RDS FIFO
    radio.setSeekThreshold(40); // Sets RSSI Seek Threshold (0 to 127)
    radio.setLnaPortSel(3); // Sets the LNA Port Selection (0 to 3)
    radio.setFrequency(frequency);
}

/**

 * Loop

 */
# 370 "F:\\Elektro\\!Pico\\TFT-SPI\\RDA5807M-Arduino-Pico-Radio\\RDA5807M-Arduino-Pico-Radio.ino"
void loop() {

    static unsigned long prevDisplay = millis(); // Az aktuális idő lekérése
    if (millis() - prevDisplay >= 100) {
        displayData();

        //     Serial.printf("Date: %04d-%02d-%02d, Time: %02d:%02d:%02d\n\r",
        //                   rtc.getYear(), rtc.getMonth(century), rtc.getDate(),
        //                   rtc.getHour(hourFormat, hourFormat), rtc.getMinute(), rtc.getSecond());
        //     Serial.printf("Temp: %02.2f\n\r", rtc.getTemperature());
        prevDisplay = millis();
    }

    static unsigned long polling_rds = millis();
    if ((millis() - polling_rds) > 20) {
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
