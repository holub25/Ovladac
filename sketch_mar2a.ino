#include <IRremote.hpp>
#include <EEPROM.h>

#define IR_RECEIVE_PIN 12
#define IR_SEND_PIN 10

#define BTN_SEND1_PIN 11
#define BTN_SEND2_PIN 2
#define BTN_MODE_PIN 7

#define LED_R 6
#define LED_G 9
#define LED_B 5

struct StoredCode {
  decode_type_t protocol = UNKNOWN;
  uint16_t address = 0;
  uint16_t command = 0;
  uint8_t bits = 0;
  bool valid = false;
};

StoredCode codes[4][2];

int modeIndex = 0;

bool prev1 = HIGH;
bool prev2 = HIGH;
bool mode = HIGH;

void setColor(bool r, bool g, bool b){
  analogWrite(LED_R, r ? 15 : 0);
  analogWrite(LED_G, g ? 15 : 0);
  analogWrite(LED_B, b ? 15 : 0);
  /*digitalWrite(LED_R,r);
  digitalWrite(LED_G,g);
  digitalWrite(LED_B,b);*/
}
void showMode(){
  if(modeIndex == 0){
    setColor(1,0,0);
  }else if(modeIndex == 1){
    setColor(0,1,0);
  }else if(modeIndex == 2){
    setColor(0,0,1);
  }else if(modeIndex == 3){
    setColor(1,0,1);
  }
}

void storeCode(IRData data){

  if(data.flags & IRDATA_FLAGS_IS_REPEAT) return;
  if(data.protocol==UNKNOWN) return;

  codes[modeIndex][1]=codes[modeIndex][0];

  codes[modeIndex][0].protocol=data.protocol;
  codes[modeIndex][0].address=data.address;
  codes[modeIndex][0].command=data.command;
  codes[modeIndex][0].bits=data.numberOfBits;
  codes[modeIndex][0].valid=true;

  saveCodes();

  setColor(0,0,0);
  delay(80);
  showMode();

  Serial.println("KOD ULOZEN");
  Serial.println(data.command);

}

void sendCode(StoredCode c){

  if(!c.valid) return;

  IrReceiver.end();

  switch(c.protocol){

    case NEC:
    IrSender.sendNEC(c.address,c.command,0);
    break;

    case SAMSUNG:
    IrSender.sendSamsung(c.address,c.command,0);
    break;

    case LG:
    IrSender.sendLG(c.address,c.command,0);
    break;

    case SONY:
    IrSender.sendSony(c.address,c.command,0,c.bits);
    break;

    case RC5:
    IrSender.sendRC5(c.address,c.command,0);
    break;

    case RC6:
    IrSender.sendRC6(c.address,c.command,0);
    break;

    case PANASONIC:
    IrSender.sendPanasonic(c.address, c.command, 0);
    break;
    default:
    Serial.println("Neznamy/nepodporovany protokol - neposlano");
    break;

  }
  Serial.println("POSLANY KOD");
  Serial.println(c.command);

  if(c.valid){
    setColor(0,0,0);
    delay(80);
    showMode();
  }

  delay(120);

  IrReceiver.begin(IR_RECEIVE_PIN,ENABLE_LED_FEEDBACK);

}
void saveMode(){
  EEPROM.update(0, modeIndex);
}

void saveCodes(){
  int addr = 1;

  for(int i = 0;i<4;i++){
    for(int b = 0;b<2;b++){
      EEPROM.put(addr, codes[i][b]);
      addr += sizeof(StoredCode);
    }
  }
}

void loadCodes(){
  int addr = 1;

  for(int i = 0;i<4;i++){
    for(int b = 0;b<2;b++){
      EEPROM.get(addr, codes[i][b]);
      addr += sizeof(StoredCode);
    }
  }
}

void setup() {
  
  //PAMET RESTART
  /*for(int i = 0;i< EEPROM.length(); i++){
    EEPROM.write(i,0);
  }*/

  Serial.begin(115200);
  Serial.println("IR ucici ovladac (spravne - protocol/address/command)");

  pinMode(BTN_SEND1_PIN, INPUT_PULLUP);
  pinMode(BTN_SEND2_PIN, INPUT_PULLUP);
  pinMode(BTN_MODE_PIN,INPUT_PULLUP);

  pinMode(LED_R,OUTPUT);
  pinMode(LED_G,OUTPUT);
  pinMode(LED_B,OUTPUT);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);

  loadCodes();
  modeIndex = EEPROM.read(0);
  if(modeIndex > 3){
    modeIndex = 0;
  }
  showMode();
}

void loop() {
  // přijmi a ulož
  if (IrReceiver.decode()) {
    storeCode(IrReceiver.decodedIRData);
    IrReceiver.resume();
  }

  bool m = digitalRead(BTN_MODE_PIN);
  if(mode == HIGH && m == LOW){
    modeIndex++;
    if(modeIndex > 3){
      modeIndex = 0;
    }
    saveMode();
    showMode();
    delay(180);
  }
  mode = m;

  // tlačítka - jen jednou na stisk (hrana)
  bool b1 = digitalRead(BTN_SEND1_PIN);
  if (prev1 == HIGH && b1 == LOW){
    sendCode(codes[modeIndex][0]);
  }
  prev1 = b1;

  bool b2 = digitalRead(BTN_SEND2_PIN);
  if (prev2 == HIGH && b2 == LOW){
    sendCode(codes[modeIndex][1]);
  };
  prev2 = b2;
}
