/* ----------------------------Скетч управления нагрузками в зависимости от времени и температуры.-----------------------------
//Обратить внмания на условия включения лампы, в некоторых временных вариантах надо менять знаки "и" и "или" местами. Например когда время выключения меньше времени включения. Необходимо унифицировать код !!!! Скоро займусь
   -------------------------------------------------Настройка:-----------------------------------------------------------------
                 1. В Arduino IDE заходим в меню ФАЙЛ-НАСТРОЙКИ и ставим галочку показывать нумерацию строк.
                 2. Проводим настройки в "Блоке настройки устройства" строки 20-40, а именно меняем значения константи на свои.
                    За значением каждой константы после значка " // " разъяснено за что она отвечает.
                 3. Переходим к строке 120 для первоначальной установки текущего времени
  ---------------------------------------------------------------------------------------------------------------------------*/   
  
  
//-------------------------------Подключаем все внешние библиотеки---------------------
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> //библиотека для LCD по 4 проводам
#include <OneWire.h>
#include <DallasTemperature.h> //для датчика температуры
#include <DS1307new.h> //для часов реального времени
#include <TimeHelpers.h> //для простого задания таймеров
//-------------------------------------------------------------------------------------

/*------------------------------------------------Блок настройки устройства--------------------------------------------------*/

//---------------------------------------что к какому пину arduino подключено--------------------
#define ONE_WIRE_BUS 2 // пин подключения датчика температуры
#define Relay1_PIN 3 //пин управления реле вентилятора
#define Relay2_PIN 4 //пин управления реле лампой
//----------------------------------------------------------------------------------------------------------------



//--------------------------константы------------------------------------------------
#define TEMP_INTERVAL _SEC_(5) // таймер опроса датчика температуры
#define TIME_INTERVAL _SEC_(1) // таймер опроса времени
#define TempFanOff 27 //температура выключения вентилятора
#define TempFanOn 29 //температура включения вентилятора
#define TimeHHon 9 // Час включения лампы
#define TimeHHoff 21 // Час выключения лампы
//------------------------------------------------------------------------------------


/*-----------------------------------------------Конец блока настройки пользователя------------------------------------------*/




//------------------------рисуем значок градуса----------------------------------------
uint8_t temp_cel[8] =
{
B00111,
B00101,
B00111,
B00000,
B00000,
B00000,
B00000
}; 
//-----------------------------------------------------------------------------------

//--------------------------------------для часов---------------------------------------------------------------------
uint16_t startAddr = 0x0000;            // Start address to store in the NV-RAM
uint16_t lastAddr;                      // new address for storing in NV-RAM
uint16_t TimeIsSet = 0xaa55;            // Helper that time must not set again
//-------------------------------------------------------------------------------------



//-------------------инициализация устройств-------------------------------------
OneWire oneWire(ONE_WIRE_BUS);//куда подключен датчик
DallasTemperature sensors(&oneWire);// говорим что это датчик температуры
DeviceAddress insideThermometer;//адрес датчика , если их много

LiquidCrystal_I2C lcd(0x27,16,2);  // подключаем монитор
//-------------------------------------------------------------------------------






//--------------------------начальная загрузка---------------------------------
void setup(void)
{
 //запускаем порт(для отладки)
  Serial.begin(115200);
  Serial.println("Hello");
  Serial.print("Running Sensors...");
 //запускаем датчик температуры--------------------------------------------
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" sensors");
  // смотрим как подключены
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  sensors.setResolution(insideThermometer, 9); 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
 //запускаем монитор
  lcd.init();  
  lcd.backlight();
  lcd.setCursor(5,0);
  lcd.print("Hello!");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("Running Sensors");
  delay(1000);
  lcd.clear();
  lcd.createChar(1, temp_cel); 
 //выключаем наши реле--------------------
  pinMode(Relay1_PIN, OUTPUT);
  pinMode(Relay2_PIN, OUTPUT);
 //------установка времени------------------
  RTC.setRAM(0, (uint8_t *)&startAddr, sizeof(uint16_t));// Store startAddr in NV-RAM address 0x08 

/*--------------------------------------Первоначальная настройка времени----------------------------------
   1. Раскомментируйте строки 128 и 129 для первоначальной установки времени
   2. В строках 141 и 142 впишите текущее время в формате год, месяц, число
                                                  часы, минуты, секунды
   3. Вгрузите скетч в arduino
   4. Закомментируйте строки 128 и 129
   5. Вгрузите скетч в arduino. Ваше устройство настроено.
----------------------------------------------------------------------------------------------------------*/
  //TimeIsSet = 0xffff;
  //RTC.setRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));  

/*
  Control the clock.
  Clock will only be set if NV-RAM Address does not contain 0xaa.
  DS1307 should have a battery backup.
*/
  RTC.getRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));
  if (TimeIsSet != 0xaa55)
  {
    RTC.stopClock();
        
    RTC.fillByYMD(2015,9,13);
    RTC.fillByHMS(14,3,0);
    
    RTC.setTime();
    TimeIsSet = 0xaa55;
    RTC.setRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));
    RTC.startClock();
  }
  else
  {
    RTC.getTime();
  }
}


//--------------------------основной цикл программы-----------------------------------------
void loop(void)
{ 
  //----------Опрос времени для вывода на экран и включения лампы---------------------------  
  DO_EVERY( TIME_INTERVAL , takeTime());
  //----------------------------------------------------------------------------------------

  //----------Опрос датчика температуры и отслеживание реле вентилятора---------------------
  DO_EVERY( TEMP_INTERVAL , {
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    printTemperature(insideThermometer); // Use a simple function to print out the data
    Serial.println("DONE");
    });
  //----------------------------------------------------------------------------------------
}





//--------------------------------------------------подпрограммы вызываемые многократно -----------------------------
  //----------------------function to print a device address-------------------------------
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
     }
  }
  //----------------------------------------------------------------------------------------
  
  //----------------------function to print the temperature for a device--------------------
void printTemperature(DeviceAddress deviceAddress)
{
  int tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  lcd.setCursor(10, 0);
  lcd.print("t:");
  lcd.print(tempC);
  char temp2[]={'\1', 67, '\0'}; //Отрисовка градусов Цельсия
    lcd.print(temp2);
    lcd.print("C");
  if ( tempC < TempFanOff ) digitalWrite(Relay1_PIN, LOW); //если температуры меньше заданной выключаем реле вентилятора
  else if ( tempC > TempFanOn ) digitalWrite(Relay1_PIN, HIGH);// если температура поднялась выше заданной включаем реле вентилятора
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}
  //-----------------------------------------------------------------------------------------

  //----------------------------функция отображения времени------------------------------------
int takeTime(){
  RTC.getTime();
  lcd.setCursor(0, 0);
  if (RTC.hour < 10) 
  {
    lcd.print("0");
    lcd.print(RTC.hour, DEC);
    Serial.print(RTC.hour);
  } 
  else
  {
    lcd.print(RTC.hour, DEC);
    Serial.print(RTC.hour);
  }
  lcd.print(":");
  if (RTC.minute < 10)                 
  {
    lcd.print("0");
    lcd.print(RTC.minute, DEC);
  }
  else
  {
    lcd.print(RTC.minute, DEC);
  }
    lcd.print(":");
  if (RTC.second < 10)                  
  {
    lcd.print("0");
    lcd.print(RTC.second, DEC);
  }
  else
  {
    lcd.print(RTC.second, DEC);
  }
    lcd.print(" ");
    lcd.setCursor(0, 1);
  if (RTC.day < 10)                    
  {
    lcd.print("0");
    lcd.print(RTC.day, DEC);
  }
  else
  {
    lcd.print(RTC.day, DEC);
  }
    lcd.print("-");
  if (RTC.month < 10)                  
  {
    lcd.print("0");
    lcd.print(RTC.month, DEC);
  }
  else
  {
    lcd.print(RTC.month, DEC);
  }
   lcd.print("-");
   lcd.print(RTC.year, DEC);
   lcd.print(" ");
  switch (RTC.dow)                      // Friendly printout the weekday
  {
    case 1:
      lcd.print("MON");
      break;
    case 2:
      lcd.print("TUE");
      break;
    case 3:
      lcd.print("WED");
      break;
    case 4:
      lcd.print("THU");
      break;
    case 5:
      lcd.print("FRI");
      break;
    case 6:
      lcd.print("SAT");
      break;
    case 7:
      lcd.print("SUN");
      break;
  }
  uint8_t MESZ = RTC.isMEZSummerTime();
  if ( RTC.hour >= TimeHHon && RTC.hour < TimeHHoff ) digitalWrite(Relay2_PIN, HIGH); //если время больше или равно времени включения или меньше или равно времени выключения , то светим
  else if ( RTC.hour < TimeHHon || RTC.hour >= TimeHHoff ) digitalWrite(Relay2_PIN, LOW);// если время меньше времени включения и больше времени выключения не светим 
}
  //------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

