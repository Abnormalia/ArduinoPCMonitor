/*
 Name:		ARDMonitor.ino
 Created:	6/13/2019 2:52:37 AM
 Author:	Abnormalia
*/
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
// optional
#define LCD_RESET A4

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define M_INFO		0
#define M_CPUGRAPH  1
#define M_CRYPRO    2

#define MAX_CRYPTO 62

unsigned int displayMode = M_INFO;

void Init_TFT()
{
#ifdef USE_Elegoo_SHIELD_PINOUT
	Serial.println(F("Using Elegoo 2.4\" TFT Arduino Shield Pinout"));
#else
	Serial.println(F("Using Elegoo 2.4\" TFT Breakout Board Pinout"));
#endif

	Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

	tft.reset();

	uint16_t identifier = tft.readID();
	if (identifier == 0x9325) {
		Serial.println(F("Found ILI9325 LCD driver"));
	}
	else if (identifier == 0x9328) {
		Serial.println(F("Found ILI9328 LCD driver"));
	}
	else if (identifier == 0x4535) {
		Serial.println(F("Found LGDP4535 LCD driver"));
	}
	else if (identifier == 0x7575) {
		Serial.println(F("Found HX8347G LCD driver"));
	}
	else if (identifier == 0x9341) {
		Serial.println(F("Found ILI9341 LCD driver"));
	}
	else if (identifier == 0x8357) {
		Serial.println(F("Found HX8357D LCD driver"));
	}
	else if (identifier == 0x0101)
	{
		identifier = 0x9341;
		Serial.println(F("Found 0x9341 LCD driver"));
	}
	else if (identifier == 0x1111)
	{
		identifier = 0x9328;
		Serial.println(F("Found 0x9328 LCD driver"));
	}
	else {
		Serial.print(F("Unknown LCD driver chip: "));
		Serial.println(identifier, HEX);
		Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
		Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
		Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
		Serial.println(F("If using the breakout board, it should NOT be #defined!"));
		Serial.println(F("Also if using the breakout, double-check that all wiring"));
		Serial.println(F("matches the tutorial."));
		identifier = 0x9328;

	}
	tft.begin(identifier);
	tft.setRotation(3);
}

int BTCPrices[MAX_CRYPTO];

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(9600);
	Serial.println(F("TFT LCD test"));

	Init_TFT();

	pinMode(13, OUTPUT);

	DrawMainBG();

	for (size_t i = 0; i < MAX_CRYPTO; i++)
	{
		BTCPrices[i] = 0;
	}
}

void DrawMainBG()
{
	//Erase the screen
	tft.fillScreen(BLACK);
	//Draw frame
	tft.drawRect(0, 0, tft.width(), tft.height(), RED);
	tft.setCursor(40, 5);
	tft.setTextSize(2);
	tft.setTextColor(WHITE, RED);
	tft.print("ARDUINO MONITOR v0.1");
}

void DrawGraphBG()
{
	tft.drawRect(5, 30, 290, 100, WHITE);
	tft.drawRect(5, 135, 290, 100, WHITE);

	tft.setTextSize(1);
	tft.setTextColor(CYAN);

	for (size_t i = 0; i <= 100; i += 20)
	{
		if (i > 20)
		{
			for (size_t j = 0; j < 29; j++)
			{
				tft.drawFastHLine(6 + 10 * j, 8 + i, 5, CYAN);
				tft.drawFastHLine(6 + 10 * j, 112 + i, 5, CYAN);
			}
		}

		tft.setCursor(298, 120 - i * .9f);
		tft.print(i);

		tft.setCursor(298, 225 - i * .9f);
		tft.print(i);
	}
}

#define MINPRESSURE 10
#define MAXPRESSURE 1000

const byte	numChars = 100;
char		receivedChars[numChars];
boolean		newData = false;

void SerialReceiveMessage() 
{
	static boolean recvInProgress = false;
	static byte ndx = 0;
	char startMarker = '<';
	char endMarker = '>';
	char rc;

	while (Serial.available() > 0 && newData == false) 
	{
		rc = Serial.read();

		if (recvInProgress == true) 
		{
			if (rc != endMarker) 
			{
				receivedChars[ndx] = rc;
				ndx++;
				
				if (ndx >= numChars) 
				{
					ndx = numChars - 1;
				}
			}
			else 
			{
				receivedChars[ndx] = '\0'; // terminate the string
				recvInProgress = false;
				ndx = 0;
				newData = true;
			}
		}

		else 
		if (rc == startMarker) 
		{
			recvInProgress = true;
		}
	}
}

// Function to return a substring defined by a delimiter at an index
char* SubString(char* str, char delim, int index) 
{
	char *act, *sub, *ptr;
	char copy[numChars];
	int i;

	// Since strtok consumes the first arg, make a copy
	strcpy(copy, str);

	for (i = 1, act = copy; i <= index; i++, act = NULL) {
		//Serial.print(".");
		sub = strtok_r(act, &delim, &ptr);
		if (sub == NULL) break;
	}
	return sub;
}

String TrimSufx(String pSym, String pWord)
{
	String Suffix = pSym;

	for (int i = 0; i < 3 - pWord.length(); i++)
	{
		Suffix += " ";
	}

	return Suffix;
}

// STAT VARIABLES /////////////////////////////////////////////////
String CPULoad		= "";
String CPUCoreTemps = "";
String GPUTemps		= "";
String RAMUsed		= "";

String CPUName	= "";
String CPUMhz	= "";
String GPUName	= "";
String TotalRAM = "";

int    PriceDelta = 0;
String BTCPrice = "";

unsigned int GraphIndex = 0;

uint16_t ColorByValue(int value)
{
	if (value < 40) return GREEN;

	if (value > 40 && value < 80) return YELLOW;

	if (value > 80) return RED;
}

void PrintPCInfo()
{
	if (displayMode != M_INFO) return;

	tft.setTextSize(2);
	tft.setTextColor(CYAN, BLACK);

	tft.setCursor(10, 30);
	tft.print(CPUName);

	tft.setCursor(10, 50);
	tft.print(CPUMhz + " MHz");

	tft.setCursor(10, 70);
	tft.print(GPUName.substring(7, GPUName.length()));

	tft.setCursor(10, 90);
	tft.print("Memory : " + TotalRAM + " GB");
}

void OUT_PCInfo()
{
	tft.setCursor(30, 120);

	tft.setTextColor(ColorByValue(CPULoad.toInt()), BLACK);
	tft.print("CPU Load : " + CPULoad + TrimSufx("%", CPULoad));

	tft.setCursor(30, 150);
	tft.setTextColor(ColorByValue(CPUCoreTemps.toInt()), BLACK);
	tft.print("CPU Temp : " + CPUCoreTemps + TrimSufx("C", CPUCoreTemps));

	tft.setCursor(30, 180);
	tft.setTextColor(ColorByValue(RAMUsed.toInt()), BLACK);
	tft.print("RAM Used : " + RAMUsed + TrimSufx("%", RAMUsed));

	tft.setCursor(30, 210);
	tft.setTextColor(ColorByValue(GPUTemps.toInt()), BLACK);
	tft.print("GPU Temp : " + GPUTemps + TrimSufx("C", GPUTemps));
}

int CryptoIndex = 0;

void OUT_GraphInfo()
{
	tft.setTextSize(1);
	tft.setTextColor(WHITE, BLACK);

	int value = CPULoad.toInt();
	tft.drawFastVLine(5 + GraphIndex, 130 - value, value, ColorByValue(value));
	tft.fillCircle(5 + GraphIndex, 130 - value, 1, RED);

	tft.setCursor(120, 28);
	tft.print("  LOAD=" + CPULoad + TrimSufx("%", CPULoad));

	value = CPUCoreTemps.toInt();
	tft.drawFastVLine(5 + GraphIndex, 234 - value, value, ColorByValue(value));
	tft.fillCircle(5 + GraphIndex, 234 - value, 1, RED);

	tft.setCursor(120, 133);
	tft.print("  TEMP=" + CPUCoreTemps + TrimSufx("C", CPUCoreTemps));

	GraphIndex++;

	if (GraphIndex > 290)
	{
		//Cleanup
		tft.fillScreen(BLACK);
		DrawMainBG();
		DrawGraphBG();

		GraphIndex = 0;
	}
}

void DrawCryptoBG()
{
	tft.drawRect(5, 45, 310, 192, WHITE);

	for (size_t i = 0; i < CryptoIndex; i++)
	{
		int value = (BTCPrices[i] % 1000) / 5;

		PriceDelta = i == 0 ? 0 : BTCPrices[i] - BTCPrices[i - 1];

		tft.drawRect(5 + i * 5, 235 - value, 5, value, PriceDelta >= 0 ? GREEN : RED);
		tft.fillCircle(7 + i * 5, 235 - value, 3, RED);
	}
}

void OUT_CryptoInfo()
{
	tft.setTextSize(3);
	tft.setCursor(15, 35);	

	int value = (BTCPrices[CryptoIndex] % 1000) / 5;

	PriceDelta = CryptoIndex == 0 ? 0 : BTCPrices[CryptoIndex] - BTCPrices[CryptoIndex - 1];

	if (PriceDelta >= 0)
	{
		tft.setTextColor(GREEN, BLACK);
		tft.print("BTC : " + BTCPrice + "$(" + String(PriceDelta) + TrimSufx(")",String(PriceDelta)));
	}
	else
	{
		tft.setTextColor(RED, BLACK);
		tft.print("BTC : " + BTCPrice + "$(" + String(PriceDelta) + TrimSufx(")", String(PriceDelta)));
	}

	tft.drawRect(5 + CryptoIndex * 5, 235 - value, 5, value, PriceDelta >= 0 ? GREEN : RED);
	tft.fillCircle(7 + CryptoIndex * 5, 235 - value, 3, RED);
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	digitalWrite(13, HIGH);
	TSPoint p = ts.getPoint();
	digitalWrite(13, LOW);

	// if sharing pins, you'll need to fix the directions of the touchscreen pins
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);

	if (p.z > MINPRESSURE && p.z < MAXPRESSURE) 
	{
		//Serial.print("X = "); Serial.print(p.x);
		//Serial.print("\tY = "); Serial.print(p.y);
		//Serial.print("\tPressure = "); Serial.println(p.z);

		displayMode++;

		if (displayMode >= 3)
			displayMode = 0;

		switch (displayMode)
		{
			case M_INFO:
				DrawMainBG();
				PrintPCInfo();
				GraphIndex = 0;
			break;

			case M_CPUGRAPH:
				DrawMainBG();
				DrawGraphBG();
			break;

			case M_CRYPRO:
				DrawMainBG();
				DrawCryptoBG();
			break;
		}
	}

	SerialReceiveMessage();

	if (newData == true) 
	{
		String command = SubString(receivedChars, ',', 1);

		//Static one time data
		if (command == "I" )
		{
			CPUName	 = SubString(receivedChars, ',', 2);
			CPUMhz	 = SubString(receivedChars, ',', 3);
			GPUName	 = SubString(receivedChars, ',', 4);
			TotalRAM = SubString(receivedChars, ',', 5);

			PrintPCInfo();
		}

		//Update realtime data
		if (command == "U")
		{
			tft.setTextSize(3);

			CPULoad			= SubString(receivedChars, ',', 2);
			CPUCoreTemps	= SubString(receivedChars, ',', 3);
			GPUTemps		= SubString(receivedChars, ',', 4);
			RAMUsed			= SubString(receivedChars, ',', 5);

			if (displayMode == M_CPUGRAPH)
			{
				OUT_GraphInfo();
			}

			if (displayMode == M_INFO)
			{
				OUT_PCInfo();
			}
		}

		if (command == "B")
		{
			BTCPrice = SubString(receivedChars, ',', 2);

			int curPrice = BTCPrice.toInt();

			BTCPrices[CryptoIndex] = curPrice;
		
			if (displayMode == M_CRYPRO)
			{
				OUT_CryptoInfo();
			}

			CryptoIndex++;

			if (CryptoIndex >= MAX_CRYPTO)
			{
				CryptoIndex = 0;

				for (size_t i = 0; i < MAX_CRYPTO; i++)
				{
					BTCPrices[i] = 0;
				}

				if (displayMode == M_CRYPRO)
				{
					//Cleanup
					tft.fillScreen(BLACK);
					DrawMainBG();

					DrawCryptoBG();
				}
			}
		}

		newData = false;
	}
}
