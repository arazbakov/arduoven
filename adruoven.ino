#include <SPI.h>
#include <ILI9341_due_config.h>
#include <ILI9341_due.h>
#include "SmallFont.h"
#include "BigFont.h"
#include "Dingbats1_XL.h"

#include <URTouch.h>
#include <ILI9341_due_Buttons.h>

// For the Adafruit shield, these are the default.
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

#define APP_TITLE "ARDUOVEN"
#define APP_VERSION "v1706.1"

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_due tft = ILI9341_due(TFT_CS, TFT_DC, TFT_RST);

// Set up UTouch...
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino 2009/Uno/Leonardo shield   : 15,10,14,9,8
// Standard Arduino Mega/Due shield            : 6,5,4,3,2
// CTE TFT LCD/SD Shield for Arduino Due       : 6,5,4,3,2
// Standard chipKit Uno32/uC32                 : 20,21,22,23,24
// Standard chipKit Max32                      : 62,63,64,65,66
// AquaLEDSource All in One Super Screw Shield : 62,63,64,65,66
// CC3200 LaunchPad (pins used in the examples): 31,13,19,28,17
//
#define T_CLK 3
#define T_CS 4
#define T_DIN 5
#define T_DO 6
#define T_IRQ 7

URTouch  myTouch(T_CLK, T_CS, T_DIN, T_DO, T_IRQ);

#define TS_MINX 0
#define TS_MINY 8
#define TS_MAXX 302
#define TS_MAXY 227

#define MEASUREMENTS_LENGTH 320;


#define SCREEN_MAIN_MENU 0
#define SCREEN_THERMOSTAT_MENU 1
#define SCREEN_THERMOSTAT_SETTINGS 2
#define SCREEN_THERMOSTAT_HEATING 3

#define COLOR_BACKGROUND 0x0000
#define COLOR_TEXT_FOREGROUND 0xDEDB
#define COLOR_BUTTON_FOREGROUND 0xFFFF
#define COLOR_BUTTON_BACKGROUND 0xDA24
#define COLOR_BUTTON_DISABLED 0xB596

int currentScreen = -1;


double readingsSumm = 0.0;
int readingsNumber = 0;

unsigned long lastUpdateTime = 0;
unsigned long temperatureMeasurementTime = 500;

char stringBuffer[100];

// Finally we set up ILI9341_due_Buttons :)
ILI9341_due_Buttons  myButtons(&tft, &myTouch);

void setScreen(int screenIdentifier);

int but1, but2, but3, but4, butX, butY, pressed_button;

int mainMenuButtonLead, mainMenuButtonLeadFree, mainMenuButtonThermostat;

void screenMainMenuSetup() {
  tft.fillScreen(COLOR_BACKGROUND);
  // top bar - title
  tft.setTextColor(COLOR_TEXT_FOREGROUND, COLOR_BACKGROUND);
  tft.setTextLetterSpacing(1);
  tft.setFont(BigFont);
  tft.printAt(APP_TITLE, 4, 16);
  tft.setFont(SmallFont);
  tft.printAtPivoted(APP_VERSION, tft.width() - 4, 18, gTextPivotTopRight);

  int pos = 0;
  int horizontalOffset = 4;
  int topOffset = 48;
  int buttonHeight = 56;
  int buttonBottomMargin = 8;
  int buttonWidth = tft.width() - horizontalOffset * 2;
  myButtons.setButtonColors(COLOR_BUTTON_FOREGROUND, COLOR_BUTTON_DISABLED, COLOR_BUTTON_BACKGROUND, COLOR_BUTTON_FOREGROUND, COLOR_BUTTON_BACKGROUND);
  mainMenuButtonLead = myButtons.addButton( horizontalOffset,  topOffset + (buttonHeight + buttonBottomMargin) * pos++, buttonWidth,  buttonHeight, "Lead 210 C");
  mainMenuButtonLeadFree = myButtons.addButton( horizontalOffset,  topOffset + (buttonHeight + buttonBottomMargin) * pos++, buttonWidth,  buttonHeight, "Lead Free 230 C");
  mainMenuButtonThermostat = myButtons.addButton( horizontalOffset,  topOffset + (buttonHeight + buttonBottomMargin) * pos++, buttonWidth,  buttonHeight, "Thermostat");
  myButtons.drawButtons();
}

void screenMainMenuLoop() {
  int pressed_button = myButtons.checkButtons();
  if (pressed_button == mainMenuButtonThermostat) {
      setScreen(SCREEN_THERMOSTAT_MENU);
      return;
  }
}

void screenMainMenuDispose() {
  myButtons.deleteAllButtons();
}

int thermostatMenuButtonBack;
void screenThermostatMenuSetup() {
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setTextLetterSpacing(0);
  thermostatMenuButtonBack = myButtons.addButton( 4, 8, 32, 32, "<");
  myButtons.drawButtons();
  
  tft.setTextColor(COLOR_TEXT_FOREGROUND, COLOR_BACKGROUND);
  tft.setFont(BigFont);
  tft.setTextLetterSpacing(0);
  tft.printAt("THERMOSTAT", 32 + 4 + 8, 16);
}

void screenThermostatMenuLoop() {
  int pressed_button = myButtons.checkButtons();
  if (pressed_button == thermostatMenuButtonBack) {
      setScreen(SCREEN_MAIN_MENU);
      return;
  }
}

void screenThermostatMenuDispose() {
  myButtons.deleteAllButtons();
}


void setScreen(int screenIdentifier) {
  if (currentScreen >= 0) {
    switch (currentScreen) {
      case SCREEN_MAIN_MENU:
        screenMainMenuDispose();
        break;
      case SCREEN_THERMOSTAT_MENU:
        screenThermostatMenuDispose();
        break;
    }
  }

  currentScreen = screenIdentifier;
  switch (currentScreen) {
    case SCREEN_MAIN_MENU:
      screenMainMenuSetup();
      break;
    case SCREEN_THERMOSTAT_MENU:
      screenThermostatMenuSetup();
      break;  
  }
}


void setup()
{
  lastUpdateTime = millis();

  Serial.begin(115200);
  Serial1.begin(9600);
  // Initial setup
  tft.begin();
  tft.setRotation(iliRotation270);  // landscape
  tft.fillScreen(ILI9341_BLACK);

  tft.drawRect(0, 0, 320, 180, 0x69BC);

  tft.setFont(SmallFont);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

  myButtons.setTextFont(BigFont);
  myButtons.setSymbolFont(Dingbats1_XL);

  setScreen(SCREEN_MAIN_MENU);
}

void drawTemperature(double value)
{
  sprintf(stringBuffer, "%.2f C", value);
  
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);

  Serial.println("START");
  
  tft.printAt(stringBuffer, 110, 220);

  Serial.println("END");
  Serial.println(stringBuffer);
}

int graphCurrentPosition = 0;
int graphStartPoint = 1;
int graphWidth = 318;
int graphHeight = 178;
int graphPositionsNumber = graphWidth;
int previousPointPosition = 0;
int graphOffsetY = 1;

void drawGraph(double value)
{
  int resultPointPosition = map(round(value), 10, 250, 0, graphHeight);
  if (resultPointPosition < 0) {
    resultPointPosition = 0;
  }
  if (resultPointPosition > graphHeight - 1) {
    resultPointPosition = graphHeight - 1;
  }

  resultPointPosition = graphOffsetY + graphHeight - resultPointPosition;

  tft.drawFastVLine(graphStartPoint + graphCurrentPosition, 1, graphHeight, ILI9341_BLACK);
  if (graphCurrentPosition > 0) {
    int diff = max(abs(resultPointPosition - previousPointPosition), 1);
    int startPoint = min(resultPointPosition, previousPointPosition);
    tft.drawFastVLine(graphStartPoint + graphCurrentPosition, startPoint, diff, 0x27DE);
  }

  graphCurrentPosition = (graphCurrentPosition + 1) % graphWidth;

  if (graphCurrentPosition > 0) {
    tft.drawFastVLine(graphStartPoint + graphCurrentPosition, 1, graphHeight, 0xEE64);
  }
  
  previousPointPosition = resultPointPosition;
}

void loop()
{
  if (currentScreen >= 0) {
    switch (currentScreen) {
      case SCREEN_MAIN_MENU:
        screenMainMenuLoop();
        break;
      case SCREEN_THERMOSTAT_MENU: 
        screenThermostatMenuLoop();
        break;
    }
  }

  return;

  if (Serial1.available())
  {
    float value = Serial1.parseFloat();
    readingsSumm += value;
    ++readingsNumber;
  }

  if (millis() - lastUpdateTime > temperatureMeasurementTime) {
    lastUpdateTime = millis();
    double result = readingsNumber > 0 ? readingsSumm / (double)readingsNumber : 0;
    

    drawTemperature(result);

    drawGraph(result);
    
    readingsSumm = 0;
    readingsNumber = 0;
  }

  /*boolean default_colors = true;

    but1 = myButtons.addButton( 10,  20, 300,  30, "Button 1");
    but2 = myButtons.addButton( 10,  60, 300,  30, "Button 2");
    but3 = myButtons.addButton( 10, 100, 300,  30, "Button 3");
    but4 = myButtons.addButton( 10, 140, 300,  30, "Button 4", BUTTON_DISABLED);
    butX = myButtons.addButton(279, 199,  40,  40, "a", BUTTON_SYMBOL);
    butY = myButtons.addButton(  0, 199, 100,  40, "I", BUTTON_SYMBOL | BUTTON_SYMBOL_REP_3X);
    myButtons.drawButtons();

    tft.printAt("You pressed:", 110, 205);
    tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
    tft.printAt("None    ", 110, 220);

    while (1)
    {
    if (myTouch.dataAvailable() == true)
    {
      pressed_button = myButtons.checkButtons();

      if (pressed_button == butX)
      {
        if (myButtons.buttonEnabled(but4))
          myButtons.disableButton(but4, true);
        else
          myButtons.enableButton(but4, true);
      }
      else if (pressed_button == butY)
      {
        if (default_colors)
        {
          myButtons.setButtonColors(ILI9341_YELLOW, ILI9341_RED, ILI9341_YELLOW, ILI9341_BLUE, ILI9341_GRAY);
          myButtons.relabelButton(butY, "_");
          myButtons.drawButtons();
          default_colors = false;
        }
        else
        {
          myButtons.setButtonColors(ILI9341_WHITE, ILI9341_GRAY, ILI9341_WHITE, ILI9341_RED, ILI9341_BLUE);
          myButtons.relabelButton(butY, "I");
          myButtons.drawButtons();
          default_colors = true;
        }
      }
      if (pressed_button == but1)
      {
        tft.printAt("Button 1", 110, 220);
        //Serial.println("Button 1");
      }

      if (pressed_button == but2)
      {
        tft.printAt("Button 2", 110, 220);
        //Serial.println("Button 2");
      }
      if (pressed_button == but3)
      {
        tft.printAt("Button 3", 110, 220);
        //Serial.println("Button 3");
      }
      if (pressed_button == but4)
      {
        tft.printAt("Button 4", 110, 220);
        //Serial.println("Button 4");
      }
      if (pressed_button == -1)
      {
        tft.printAt("None    ", 110, 220);
      }
    }
    }*/
}

