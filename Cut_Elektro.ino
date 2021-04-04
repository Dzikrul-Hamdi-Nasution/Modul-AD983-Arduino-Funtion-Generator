#include <Wire.h> // Library komunikasi I2C 
#include <LiquidCrystal_I2C.h> // Library modul I2C LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <EEPROM.h>
#include <AD9833.h>     // Include the library 
#define FNC_PIN 10       // Can be any digital IO pin 
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency

#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;

const int tombol_down = 2;
const int tombol_up = 3;
const int relay = 4;
const int saklar = 5;
int frekuensi, memori, gelombang;

const unsigned int TRIG_KANAN = 8;
const unsigned int ECHO_KANAN = 9;
const unsigned int TRIG_KIRI = A1;
const unsigned int ECHO_KIRI = A0;
const unsigned int TRIG_DEPAN = 6;
const unsigned int ECHO_DEPAN = 7;

float voltage_V = 0, shuntVoltage_mV, busVoltage_V;
float current_mA = 0;
float power_mW = 0;
float energy_Wh = 0;
long time_s = 0;


void setup()
{
  Serial.begin(9600);
  pinMode(TRIG_KANAN, OUTPUT);
  pinMode(ECHO_KANAN, INPUT);
  pinMode(TRIG_KIRI, OUTPUT);
  pinMode(ECHO_KIRI, INPUT);
  pinMode(TRIG_DEPAN, OUTPUT);
  pinMode(ECHO_DEPAN, INPUT);

  lcd.backlight();
  lcd.init();
  lcd.setCursor(4, 0);
  lcd.print("Function");
  lcd.setCursor(3, 1);
  lcd.print("Generator");

  gen.Begin();
  gen.ApplySignal(SQUARE_WAVE, REG0, 350000);
  gen.EnableOutput(true);

  pinMode(relay, OUTPUT);
  pinMode(tombol_up, INPUT);
  pinMode(tombol_down, INPUT);
  pinMode(saklar, INPUT);
  pinMode (tombol_down, INPUT_PULLUP);
  pinMode (tombol_up, INPUT_PULLUP);
  pinMode (saklar, INPUT_PULLUP);
  digitalWrite(relay, HIGH);

  delay(3000);
  lcd.clear();
  frekuensi = 22000;

  uint32_t currentFrequency;
  ina219.begin();

}
int kondisi_jarak_kiri, kondisi_jarak_kanan, kondisi_jarak_depan;
int obstacle;

void loop() {

  cek_jarak_kiri();
  cek_jarak_kanan();
  cek_jarak_depan();
  prog_utama();

  if (kondisi_jarak_kiri == 1 or kondisi_jarak_kanan == 1 or kondisi_jarak_depan == 1) {
    lcd.backlight();
    obstacle = 1;
  }
  else {
    lcd.noBacklight();
    obstacle = 0;
    gen.ApplySignal(SINE_WAVE, REG0, 0);
  }


}


void cek_jarak_kiri() {
  digitalWrite(TRIG_KIRI, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_KIRI, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_KIRI, LOW);
  const unsigned long duration = pulseIn(ECHO_KIRI, HIGH);
  int distance = duration / 29 / 2;
  if (distance > 5 and distance < 30) {
    Serial.print("Sensor kiri aktif  ");
    Serial.println(distance);
    lcd.backlight();
    kondisi_jarak_kiri = 1;
  }
  else {
    kondisi_jarak_kiri = 0;
  }
}

void cek_jarak_kanan() {
  digitalWrite(TRIG_KANAN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_KANAN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_KANAN, LOW);
  const unsigned long duration = pulseIn(ECHO_KANAN, HIGH);
  int distance = duration / 29 / 2;
  if (distance > 5 and distance < 30) {
    Serial.print("Sensor kanan aktif  ");
    Serial.println(distance);
    lcd.backlight();
    kondisi_jarak_kanan = 1;
  }
  else {
    kondisi_jarak_kanan = 0;
  }
}

void cek_jarak_depan() {
  digitalWrite(TRIG_DEPAN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_DEPAN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_DEPAN, LOW);
  const unsigned long duration = pulseIn(ECHO_DEPAN, HIGH);
  int distance = duration / 29 / 2;
  if (distance > 5 and distance < 30) {
    Serial.print("Sensor depan aktif  ");
    Serial.println(distance);
    lcd.backlight();
    kondisi_jarak_depan = 1;
  }
  else {
    kondisi_jarak_depan = 0;
  }

}





void prog_utama() {
  lcd.setCursor(0, 0);
  lcd.print("F : ");
  lcd.setCursor(4, 0);
  lcd.print(frekuensi);

  if (digitalRead(tombol_up) == LOW) {
    frekuensi = frekuensi + 100;
    if (digitalRead(tombol_down) == LOW) {
      gelombang = 1;
    }
    delay(60);
    memori = frekuensi / 1000;
    EEPROM.write(0, memori);
  }
  if (digitalRead(tombol_down) == LOW) {
    frekuensi = frekuensi - 100;
    if (digitalRead(tombol_up) == LOW) {
      gelombang = 2;
    }
    delay(60);
    memori = frekuensi / 1000;
    EEPROM.write(0, memori);
  }

  memori = EEPROM.read(0);
  //frekuensi = memori * 1000;

  if (digitalRead(saklar) == LOW) {
    if (gelombang == 1) {
      getData();
      if (obstacle == 1) {
        gen.ApplySignal(SQUARE_WAVE, REG0, frekuensi);
      }
      lcd.setCursor(0, 1);
      lcd.print("SPIKE");
      digitalWrite(relay, HIGH);
    }
    if (gelombang == 2) {
      getData();
      if (obstacle == 1) {
        gen.ApplySignal(SINE_WAVE, REG0, frekuensi);
      }
      lcd.setCursor(0, 1);
      lcd.print("SINUS");
      digitalWrite(relay, LOW);
    }
  }
  if (digitalRead(saklar) == HIGH) {
    getData();
    if (obstacle == 1) {
      gen.ApplySignal(SQUARE_WAVE, REG0, frekuensi);
    }
    lcd.setCursor(0, 1);
    lcd.print("PULSA");
    digitalWrite(relay, LOW);
  }



}

void getData() {

  time_s = millis() / (1000); // convert time to sec
  busVoltage_V = ina219.getBusVoltage_V();
  shuntVoltage_mV = ina219.getShuntVoltage_mV();
  voltage_V = busVoltage_V + (shuntVoltage_mV / 1000);
  current_mA = ina219.getCurrent_mA();
  //power_mW = ina219.getPower_mW();
  power_mW = current_mA * voltage_V;
  energy_Wh = (power_mW * time_s) / 3600; //energy in watt hour



  lcd.setCursor(7, 1);
  lcd.print(power_mW);
  lcd.print(" mW ");

  Serial.print("Bus Voltage:   "); Serial.print(busVoltage_V); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntVoltage_mV); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(voltage_V); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  Serial.print("Energy:        "); Serial.print(energy_Wh); Serial.println(" mWh");
  Serial.println("----------------------------------------------------------------------------");
}
