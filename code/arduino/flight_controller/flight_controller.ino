/*
The way robo-platform's test sequence works:
  1. Server (phone) sends a specific code: 
    The first byte is always data length and the second shows cmd or data stat
    The code starts from the third byte: in-code-9372
  2. If the in-code is correct, send: out-code-6334 with the same format
  3. [optional] toggle a LED indicating test was successful

Tasks:
  1. Test communication interface
  2. Test flight control operation
*/

#include <time.h> 
#include <math.h>

#include <Servo.h>

typedef unsigned char uchar;

uchar led_state = 0;

const int LEN_INPUT = 64;
const int LEN_OUTPUT = 64;
char inputBuff[LEN_INPUT];
char outputBuff[LEN_OUTPUT];

const char inCode[] = "in-code-9372";
const char outCode[] = "out-code-6334";
const char LEN_IN_CODE = 12;
const char LEN_OUT_CODE = 13;


uchar ctrl_state = 0x00;

char state_a = 0, state_b = 0, state_c = 0, state_d = 0;
int cnt_a = 0, cnt_b = 0, cnt_c = 0, cnt_d = 0;
const int max_cnt = 10000; // 5 sec at 2kHz clock
int n_cycle = 0;

Servo servo_a, servo_b, servo_c, servo_d;


bool cmp_code(char* code1, char lenCode1, const char* code2, const char lenCode2) {
  if (lenCode1 != lenCode2) {
    return 0;
  }
  for (int i = 0; i < lenCode1; i++) {
    if (code1[i] != code2[i]) {
      return 0;
    }
  }
  return 1;
}

void send_test(void) {

  Serial.write(LEN_OUT_CODE);
  Serial.write(0x01);
  
  for(int i = 0; i < LEN_OUT_CODE; i++)
    Serial.write(outCode[i]);
}


char update_state(const uchar mask, const uchar n_bit) {
  bool pcond = (ctrl_state & mask) >> n_bit;
  bool ncond = (ctrl_state & (mask << 1)) >> (n_bit + 1);
  if (pcond) return 1;
  else if (ncond) return -1;
  else return 0;
}

void update_states(void) {
  state_a = update_state(0x01, 0);
  state_b = update_state(0x04, 2);
  state_c = update_state(0x10, 4);
  state_d = update_state(0x40, 6);
}

void update_cnt(int* cnt, const char state) {
  *cnt += state;
  if (*cnt > max_cnt) *cnt = max_cnt;
  if (*cnt < 0) *cnt = 0;
}

void update_pwm() {
  servo_a.write(map(cnt_a, 0, max_cnt, 0, 180));
  servo_b.write(map(cnt_b, 0, max_cnt, 0, 180));
  servo_c.write(map(cnt_c, 0, max_cnt, 0, 180));
  servo_d.write(map(cnt_d, 0, max_cnt, 0, 180));
}


void set_timer() {

  cli();//stop interrupts

  //set timer0 interrupt at 2kHz
  TCCR0A = 0;// set entire TCCR0A register to 0
  TCCR0B = 0;// same for TCCR0B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR0A = 124;// = (16*10^6) / (2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS01 and CS00 bits for 64 prescaler
  TCCR0B |= (1 << CS01) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);

  sei();//allow interrupts
}

ISR(TIMER0_COMPA_vect){//timer0 interrupt
  // update timing variables
  update_cnt(&cnt_a, state_a);
  update_cnt(&cnt_b, state_b);
  update_cnt(&cnt_c, state_c);
  update_cnt(&cnt_d, state_d);

  n_cycle++;

  if (n_cycle > max_cnt) {
    n_cycle = 0;
  }
}

void toggle_led() {
  if (led_state == 0) {
    led_state = 1;
  }
  else {
    led_state = 0;
  }
  digitalWrite(LED_BUILTIN, led_state);
}


void setup() {

  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  servo_a.attach(8);
  servo_b.attach(9);
  servo_c.attach(10);
  servo_d.attach(11);

  set_timer();
}

void loop() {

  // Syncronization
  while(!Serial.available()) {}
  
  if (Serial.available() > 0) {
    
    // can also use Serial.read() in a loop to receive chars
    int len = Serial.readBytes(inputBuff, LEN_INPUT);
    
    if (len > 0) {
      toggle_led();
      
      if (cmp_code(&inputBuff[2], inputBuff[0], inCode, LEN_IN_CODE)) {
        // test command
        send_test();
      }
      if (inputBuff[0] == 3 && len == 3) {
        // directional commands
        ctrl_state = inputBuff[2];
        //toggle_led();
      }
    }
  }

  update_states();
  update_pwm();

  delay(5);
}