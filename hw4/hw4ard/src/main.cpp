#include <Arduino.h>
#include <string.h>
#define BTN_PIN 5
#define BTN_GND_PIN 7 //button is connected between PTN_PIN and BTN_GND_PIN due physical dimensions
#define BUF_LENGTH 64
#define BTN_DEBOUNCE_PERIOD 10

#define COMMANDS_LIST_LENGTH 5
#define HANDSHAKE_MESS "HELLO"
#define HANDSHAKE_I 0
#define AYS_MESS "AYS"          // At Your Service
#define AYS_I 1
#define LED_ON_MESS "LED_ON"
#define LED_ON_I 2
#define LED_OFF_MESS "LED_OFF"
#define LED_OFF_I 3
#define LED_TG_MESS "LED_TG"
#define LED_TG_I 4

#define COMMAND_MAX_LENGTH 7 // pure chars count, no '\0' considerd
#define COMMANDS_LIST_LENGTH 5
#define TRANSMISSION_OVER_DELAY 10

#define LED_ON digitalWrite(LED_BUILTIN, LOW);
#define LED_OFF digitalWrite(LED_BUILTIN, HIGH);
#define LED_TOGGLE digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

int checkCommand(const char* comms_list[], uint8_t comm_list_length);
void confirmationBlink(void);

char buf[BUF_LENGTH] = { '\0' }; 
int8_t comm_idx = -1;
bool ays_received = false;
bool handshake_received = false;

const char* commands_list[COMMANDS_LIST_LENGTH] = {
  HANDSHAKE_MESS,
  AYS_MESS, 
  LED_ON_MESS,
  LED_OFF_MESS
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {} 
  pinMode(LED_BUILTIN,OUTPUT);
  LED_ON;   
  // handshake
  while(1) { 
    Serial.print("\r\n");
    Serial.print(HANDSHAKE_MESS);
    delay(500);
    comm_idx = checkCommand(commands_list, COMMANDS_LIST_LENGTH);
    if (comm_idx == 0) { 
      handshake_received = true;
      break; 
    }
  } 
  LED_OFF; 
  pinMode(BTN_PIN,INPUT_PULLUP);
  pinMode(BTN_GND_PIN, OUTPUT);
  digitalWrite(BTN_GND_PIN, LOW);
  Serial.print("\r\n");
  Serial.print(AYS_MESS);
}

void loop() {
  comm_idx = checkCommand(commands_list, COMMANDS_LIST_LENGTH);
  switch (comm_idx) {
    case HANDSHAKE_I:
      blink(6,millis());
      ays_received = false;
      if(handshake_received) {
        Serial.print(HANDSHAKE_MESS);
        Serial.print(AYS_MESS);
      } else {
        handshake_received = true;
      }
      break;
    case LED_ON_I:
      blink(1, millis());
      if(handshake_received) { LED_ON; }
      break;
    case LED_OFF_I:
      blink(2, millis());
      if(handshake_received) { LED_OFF; }
      break;
    case LED_TG_I:
      blink(3, millis());
      if(handshake_received) { LED_TOGGLE; }  
    case AYS_I:
      blink(4, millis());
      if(ays_received) {
        handshake_received = false;
        ays_received = false;
        Serial.print(commands_list[HANDSHAKE_I]);
      } else {
        if(handshake_received) {  
          ays_received = true;
        }
      }
      break;
    default:
      blink(1, millis());
  }
  if(ays_received && buttonHandler(millis())) {
    Serial.print(commands_list[LED_TG_I]);
  }
}

int checkCommand(const char* comms_list[], uint8_t comm_list_length) {
  static char buf[COMMAND_MAX_LENGTH + 1] = { 0 };
  static uint8_t char_pos = 0;
  static bool overflow = false;
  int c = 0;

  while((c = Serial.read()) >= 0) {
    if (c == '\r' || c == '\n' || c == '\0' || c == ' ') {
      overflow = false;
      if(buf[0]) {
        buf[char_pos] = '\0'; 
        for (int i = 0; i < comm_list_length; i++) {
          if (strcmp((const char*)buf, comms_list[i]) == 0) {
            char_pos = 0;
            memset(buf, 0, sizeof(buf));  
            return i;
          }
        }  
        char_pos = 0;
        memset(buf, 0, sizeof(buf));
      }
      continue;
    }
    if(overflow) continue;
    if(char_pos > COMMAND_MAX_LENGTH - 1) {
      char_pos = 0;
      memset(buf, 0, sizeof(buf));
      overflow = true;     
    } else {
      buf[char_pos] = c;
      char_pos++;
    }
  }
  return -1;
}

void blink(uint8_t count, uint32_t tick) {
  static uint32_t prev_tick = 0;
  static uint8_t blinks_left = 0;
  if (count > 0) {
    blinks_left = blinks_left + count * 2;
    prev_tick = 0; //first immediate toggle
  }
  if (blinks_left && (tick - prev_tick) > 50) {
    LED_TOGGLE;
    prev_tick = tick;
    blinks_left--;
  } 
}
// throws 1 on the first call after button push (state rise detection)
uint8_t buttonHandler(uint32_t tick) { 
  static bool prev_state = 0;
  static uint32_t prev_tick = 0;
  bool state = 0; 

  // BTN is tied up to HIGH by default
  bool curr_state = (digitalRead(BTN_PIN));
  
  if(tick - prev_tick > BTN_DEBOUNCE_PERIOD) {
    prev_tick = tick;
    if(curr_state && !prev_state) {
      state = 1; 
    }
    prev_state = curr_state;
  }
  return state;
}
