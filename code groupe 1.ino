#include <i2cmaster.h>
#include <EEPROM.h>//use of the EEPROM to stock the number of hornet killed

int Led1 = 4;//working
int Led2 = 5;//hornet detected
int Led3 = 6;//hornet handled

int Laser1 = 8;
int Laser2 = 3;

int BoutonPoussoir = 2;

int Moteur = 13;

float CurrentTemp;
bool isCurrentTemp;

int NbrFrelons = 0;

void setup() {
  //Classical setup : initialization of the i2c and the Serial
  pinMode(Led1, OUTPUT);
  pinMode(Led2, OUTPUT);
  pinMode(Led3, OUTPUT);
  pinMode(Moteur, OUTPUT);
  pinMode(Laser1, OUTPUT);
  pinMode(Laser2, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(BoutonPoussoir), reset, RISING);
  Serial.begin(9600);
  Serial.println("Bonjour,");
  NbrFrelons = getNbrFrelons();//see below
  if(NbrFrelons == 0){
    Serial.println("Aucun frelon n'a ete detecte");
  }
  else if(NbrFrelons == 1){
    Serial.println("1 frelon a ete elimine.");
  }
  else{
    Serial.print(NbrFrelons);
    Serial.println(" frelons ont ete elimines.");
  }

  i2c_init(); //Initialise the i2c bus
  PORTC = (1 << PORTC5) | (1 << PORTC4);//enable pullups
}

void loop() {
  digitalWrite(Led1, HIGH);
  
  int dev = 0x5A<<1;
  int data_low = 0;
  int data_high = 0;
  int pec = 0;

  i2c_start_wait(dev+I2C_WRITE);
  i2c_write(0x07);
  
  // read
  i2c_rep_start(dev+I2C_READ);
  data_low = i2c_readAck(); //Read 1 byte and then send ack
  data_high = i2c_readAck(); //Read 1 byte and then send ack
  pec = i2c_readNak();
  i2c_stop();

  //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
  double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
  double tempData = 0x0000; // zero out the data
  int frac; // data past the decimal point

  // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * tempFactor)-0.01;

  float celcius = tempData - 273.15;

  if(!isCurrentTemp){//ambiant temperature not acquired yet
    CurrentTemp = tempData;
    isCurrentTemp = true;
    delay(100);
  }
  else if((CurrentTemp+0.7<tempData)||(tempData<CurrentTemp-0.7)){//the temperature changed a lot : a hornet was deteced
    Serial.println("FRELON DETECTE");
    NbrFrelons = NbrFrelons +1;
    setNbrFrelons(NbrFrelons);//see below
    
    digitalWrite(Led2, HIGH);
    digitalWrite(Laser1, HIGH);
    digitalWrite(Laser2, HIGH);
    digitalWrite(Moteur, HIGH);
    delay(3000);
    
    digitalWrite(Led3, HIGH);
    digitalWrite(Led2, LOW);
    digitalWrite(Laser1, LOW);
    digitalWrite(Laser2, LOW);
    digitalWrite(Moteur, LOW);
    delay(1000);
    digitalWrite(Led3, LOW);
    isCurrentTemp = false;//the ambiant temperature might have varied while the hornet was handled
    
    Serial.println("FRELON ELIMINE");
    Serial.print(NbrFrelons);
    if(NbrFrelons == 1){
      Serial.println(" frelon a ete elimine.");
    }
    else{
      Serial.println(" frelons ont ete elimines.");
    }
    delay(10);
  }
  else{//the temperature did not varied enough
    CurrentTemp=tempData;
    delay(100);
  }

  
}

void reset(){
  Serial.println("RESET");
  digitalWrite(Led3, HIGH);
  NbrFrelons = 0;
  setNbrFrelons(0);
  delay(1000);
  digitalWrite(Led3, LOW);
  Serial.println("Le compteur de frelon a ete reinitialise, aucun frelon n'a ete detecte");
}

int getNbrFrelons(){//to read the EEPROM memory easily
  return (256*EEPROM.read(0)+EEPROM.read(1));//use to bites -> NbrFrelons is between 0 and 65 535
}

void setNbrFrelons(int nbrFrelons){//to write on the EEPROM memory
  EEPROM.write(0,nbrFrelons/256);
  EEPROM.write(1,nbrFrelons%256);
}

