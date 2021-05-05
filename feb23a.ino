/*Serial.begin(9600);
Serial.print("Seri haberlesme: ");
Serial.println(sayac);*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>
#if (__STM32F1__) // bluepill
#define TFT_DC    PA1
#define TFT_RST   PA0
#include <Arduino_ST7789_Fast.h>
#else
#define TFT_DC    9
#define TFT_RST   8
#include <Arduino_ST7789_Fast.h>
#endif
#define SCR_WD 240
#define SCR_HT 240
Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST);
#include <TEA5767.h>
#include <Wire.h>
#include <Button.h>



// Color definitions
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define ORANGE          0xFD20
#define GREENYELLOW     0xAFE5
#define DARKGREEN       0x03E0
TEA5767 Radio;   //Pinout SLC and SDA - Arduino pins A5 and A4

double old_frequency;
double frequency;
int search_mode = 0;
int search_direction;
unsigned long last_pressed;

// Frekans Seçme
//                           1    2   3     4   5     6    7   8   9  10  11   12   13   14   15  16    17   18   19    20   21   22    23    24                                  
double    ai_Stations[24]={88.8,89.3,90.8,91.2,91.8,92.2,92.5,94,95.8,96,96.8,98.1,98.4,99.5,100,100.5,104.6,105,105.4,105.6,106,106.5,106.8,107.9};
int    i_sidx=0;        // Maksimum İstasyon Endeksi sıfırdan sayılan istasyon sayısı)
int    i_smax=23;       // Maks İstasyon Dizini - 0..22 = 23 :) (maksimum istasyon sayısı - sıfırdan sayılır)
int    i_smin=1;

Button btn_forward(A2, PULLUP); // HAFIZA RADYO BULMA YUKARI
Button btn_backward(A3, PULLUP); // HAFIZA RADYO BULMA AŞAĞI

#define incet 5  // volume - 
#define tare 4  // volume +
#define inainte 3    // search +
#define inapoi 2   // search -

byte f1, f2, f3, f4;  // her frekans bölgesi için numara
double frecventa;
int frecventa10;      // frekans x 10 

#include <EEPROM.h>

int PotWiperVoltage0 = 0;
int RawVoltage0 = 0;
float Voltage0 = 0;
int PotWiperVoltage1 = 1;
int RawVoltage1 = 0;
float Voltage1 = 0;
const int wiper0writeAddr = B00000000;
const int wiper1writeAddr = B00010000;
const int tconwriteAddr = B01000000;
const int tcon_0off_1on = B11110000;
const int tcon_0on_1off = B00001111;
const int tcon_0off_1off = B00000000;
const int tcon_0on_1on = B11111111;
const int slaveSelectPin = 10;   
const int shutdownPin = 7;

// transform lin to log
float a = 0.2;     // for x = 0 to 63% => y = a*x;
float x1 = 63;
float y1 = 12.6;   // 
float b1 = 2.33;     // for x = 63 to 100% => y = a1 + b1*x;
float a1 = -133.33;

int level, level1, level2, stepi;

int lung = 500;    // buton için duraklat
int scurt = 25;   // tekrar döngüsü için duraklat

byte sunet = 0;    
byte volmax = 15;

void setup () {
Serial.begin(9600);
   // slaveSelectPin'i bir çıktı olarak ayarlayın:
  pinMode (slaveSelectPin, OUTPUT);
  //shutdownPin'i bir çıktı olarak ayarlayın:
  pinMode (shutdownPin, OUTPUT);
  // tüm kapların kapanmasıyla başla
  digitalWrite(shutdownPin,LOW);
  // SPI'yı başlat:
  SPI.begin(); 
 
digitalPotWrite(wiper0writeAddr, 0); // Sileceği ayarla 0 
digitalPotWrite(wiper1writeAddr, 0); // Sileceği ayarla 0 

// son frekansın okuma değeri
f1 = EEPROM.read(101);
f2 = EEPROM.read(102);
f3 = EEPROM.read(103);
f4 = EEPROM.read(104);
stepi = EEPROM.read(105);

// numarayı kurtar
frecventa = 100*f1 + 10*f2 + f3 + 0.1*f4;

pinMode(inainte, INPUT);
pinMode(inapoi, INPUT);
pinMode(incet, INPUT);
pinMode(tare, INPUT);
digitalWrite(inainte, HIGH);
digitalWrite(inapoi, HIGH);
digitalWrite(incet, HIGH);
digitalWrite(tare, HIGH);

  Wire.begin();
  lcd.init(SCR_WD, SCR_HT);
  Radio.init();
  Radio.set_frequency(frecventa); 
  i_sidx=14; // İstasyonda indeks = 15 ile başlayın (Eski - istasyon 15'den başlıyoruz, ancak sıfırdan sayıyoruz)
  Radio.set_frequency(ai_Stations[i_sidx]); 


  lcd.begin();
  lcd.fillScreen();
  
  // LCD'ye bir logo mesajı yazdırın.
  lcd.setTextSize(3);
  lcd.setTextColor(WHITE,BLACK);
  lcd.setCursor(0,0);
  lcd.println("Aydin");
  lcd.setCursor(0, 30);
  lcd.print("DEMIR");
 // lcd.setTextSize(2);
  lcd.setCursor(1, 60);
  lcd.print("FM RADIO");  
  lcd.setCursor(0, 90);
  lcd.setTextSize(2);
  lcd.setTextColor(WHITE,BLACK);
  lcd.print("24/02/2021"); 
   delay (3000);
      lcd.fillScreen();
int procent1 = stepi * 6.66;
    if (procent1 < 63)  // for x = 0 to 63% => y = a*x;
    {
    level = a * procent1;
    }
    else               // for x = 63 to 100% => y = y1 + b*x;
    {
    level = a1 + b1 * procent1;
    }  
level1 = map(level, 0, 100, 0, 255);      // 256 adımda% 100 dönüştürün 
digitalPotWrite(wiper0writeAddr, level1); // Sileceği 0 ile 255 arasında ayarlayın
digitalPotWrite(wiper1writeAddr, level1); // Sileceği 0 ile 255 arasında ayarlayın
digitalWrite(shutdownPin,HIGH); //Turn off shutdown
}

void loop () {
  
  unsigned char buf[5];
  int stereo;
  int signal_level;
  double current_freq;
  unsigned long current_millis = millis();
  
  if (Radio.read_status(buf) == 1) {
    current_freq =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
    stereo = Radio.stereo(buf);
    signal_level = Radio.signal_level(buf);
  
   lcd.setTextSize(6);
   lcd.setTextColor(DARKGREEN,BLACK); 
   lcd.setCursor(0,0);
   if (current_freq < 100) lcd.print(" ");
   lcd.print(current_freq,1);
   lcd.setTextColor(RED); 
   lcd.setTextSize(2);
   lcd.print(" MHz");
  
   // FM sinyalinin görüntülenme seviyesi ..
  
   lcd.setCursor(120,100);
   lcd.setTextColor(DARKGREEN,BLACK); 
   lcd.setTextSize(3);
   if (signal_level<10) lcd.print(" ");
   lcd.print(signal_level);
   lcd.print("/15");
   
   printpost(current_freq);
  
  if (stepi <=0) stepi = 0;
  if (stepi >=volmax) stepi = volmax;
   lcd.setCursor(0,200);
   lcd.setTextSize(4);
   lcd.setTextColor(WHITE);
   if (stepi<10) lcd.print("");
   lcd.print("Ses ");
   lcd.setTextColor(RED,BLACK);
   lcd.print(stepi);
   delay (500);
   // sinyal değerini çiz
int sus = 8;
int sl = signal_level;
   for (int x = 0; x < sl; x++)
   { 
   lcd.drawLine(160+4*x, 90-sus, 160+4*x, 90-x-sus, BLUE);
   }
   if (stereo) {
   lcd.setCursor(0,60);
   lcd.setTextSize(4);
   lcd.setTextColor(BLUE,BLACK);
   lcd.print("STEREO");
   }
    else
   {
    lcd.setCursor(0,60);
    lcd.setTextSize(4);
    lcd.setTextColor(BLUE, BLACK);  
    lcd.print(" MONO ");
   }     
  }
   if (search_mode == 1) {
      if (Radio.process_search (buf, search_direction) == 1) {
          search_mode = 0;
      }
           if (Radio.read_status(buf) == 1) {  
      frecventa =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
      frecventa10 = 10*frecventa;
 f1 = frecventa10/1000;
 frecventa10 = frecventa10 - 1000*f1;
 f2 = frecventa10/100;
 frecventa10 = frecventa10 - 100*f2;
 f3 = frecventa10/10;
 f4 = frecventa10 - 10*f3;
EEPROM.write(101,f1);
EEPROM.write(102,f2);
EEPROM.write(103,f3);
EEPROM.write(104,f4);
frecventa = 100*f1 + 10*f2 + f3 + 0.1*f4;
Radio.set_frequency(frecventa);
      }
  }
if (sunet == 1) {
if (stepi <= 0) stepi = 0;
if (stepi >= volmax) stepi = volmax;
  EEPROM.write(105,stepi);
int procent1 = stepi * 6.66;
    if (procent1 < 63)  // for x = 0 to 63% => y = a*x;
    {
    level = a * procent1;
    }
    else               // for x = 63 to 100% => y = y1 + b*x;
    {
    level = a1 + b1 * procent1;
    }  
level1 = map(level, 0, 100, 0, 255);      // convert 100% in 256 steps  

if (level1 >255) level1 = 255;
digitalPotWrite(wiper0writeAddr, level1); // Set wiper 0 to 255
digitalPotWrite(wiper1writeAddr, level1); // Set wiper 0 to 255
sunet = 0;
}

//istasyon değişikliği yukarı
   if (btn_forward.isPressed()) {
   i_sidx++;
   if (i_sidx>i_smax){i_sidx=0; }
   Radio.set_frequency(ai_Stations[i_sidx]);
   delay(lung);  }
  //istasyon değişikliği aşağı
   if (btn_backward.isPressed()) {
   i_sidx--;
   if (i_sidx<i_smin){i_sidx=23;}
   Radio.set_frequency(ai_Stations[i_sidx]);
     delay(lung);  }
  
 if (digitalRead(inainte) == LOW) { 
    last_pressed = current_millis;
    search_mode = 1;
    search_direction = TEA5767_SEARCH_DIR_UP;
    Radio.search_up(buf);
    lcd.clearScreen();
    delay(lung);  }
   
 if (digitalRead(inapoi) == LOW) { 
    last_pressed = current_millis;
    search_mode = 1;
    search_direction = TEA5767_SEARCH_DIR_DOWN;
    Radio.search_down(buf);
    lcd.clearScreen();
    delay(lung); } 
  
 if (digitalRead(incet) == LOW) { 
    stepi = stepi -1;
    if (stepi <= 0) stepi == 0;
   sunet = 1;
    delay(lung/5);   } 
   
 if (digitalRead(tare) == LOW) { 
    stepi = stepi +1;
    if (stepi > volmax ) stepi == volmax;
    sunet = 1;
    delay(lung/5);  } 
    delay(scurt);}

 // Bu işlev, pota SPI verilerinin gönderilmesini sağlar.
  void digitalPotWrite(int address, int value) {
  // çipi seçmek için SS pinini düşük tutun:
  digitalWrite(slaveSelectPin,LOW);
 
  //  SPI aracılığıyla adresi ve değeri gönder:
  SPI.transfer(address);
  SPI.transfer(value);
  //çipin seçimini kaldırmak için SS pinini yükseğe alın:
  digitalWrite(slaveSelectPin,HIGH); 
  
  }
// istasyon adlarını görüntüleme 

void printpost(double current_freq)
{
 // switch(current_freq)
  {    
   if (current_freq==88.8) { for (int i=0; i<17; i++); lcd.setCursor(0,140);
   lcd.setTextSize(2);
   lcd.setTextColor(YELLOW);lcd.print("Show Radyo  ");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("1");}
      
   if  (current_freq==89.3) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("ALEM FM      ");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("2");}
   
   if (current_freq==90.8) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("SUPER FM     ");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("3");}
   
   if (current_freq==91.2) { lcd.setCursor(0,140);
   lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("DENIZLI NET");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("4");}
       
    if (current_freq==91.8) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("Radyo 20");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("5");}
   
     if (current_freq==92.2) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("TRT FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("6");}
   
    if (current_freq==92.5) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("RADYO SES");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("7");}
   
    if (current_freq==94.0) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("TRT TURkU");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("8");}
   
    if (current_freq==95.8) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("TRT NaGme");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("9");}
   
    if (current_freq==96.0) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("METRO FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("10");}
   
    if (current_freq==96.8) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("AKRA FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("11");}

    if (current_freq==98.1) { lcd.setCursor(0,140);  
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("TATLISES");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("12");}

    if (current_freq==98.4) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("BEST FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("13");}
 
   if (current_freq==99.5) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("Radyo EGE");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("14");}

     if (current_freq==100) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("POWER FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("15");}

    if (current_freq==100.5) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("MUJDE FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("16");}

     if (current_freq==104.6) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("polis Rad");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("17");}

    if (current_freq==105.0) { lcd.setCursor(0,140);
   lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("RADYO HOROZ");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("18");}

    if (current_freq==105.4) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("GURBETCI");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("19");}

   else if (current_freq==105.6) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("HABERTURK");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("20");}

    if (current_freq==106.0) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("Dogus FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("21");}

    if (current_freq==106.5) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("JOYTURK FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("22");}

    if (current_freq==106.8) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("Dostlar FM");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("23");}

    if (current_freq==107.9) { lcd.setCursor(0,140);
   lcd.setTextSize(4);
   lcd.setTextColor(YELLOW);lcd.print("TRT Haber");
   lcd.setCursor(0,100);lcd.setTextSize(3);
   lcd.setTextColor(YELLOW);lcd.print("MEM ");
   lcd.print("24");}
        
 } 
}
