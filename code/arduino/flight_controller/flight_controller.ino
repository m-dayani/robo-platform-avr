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

// You need to install mylib folder for this to work
//#define ARDUINO 1
//#include <led.h>
//#include <controller.h>
//#include <mypwm.h>
//#include <vusb_wrapper.h>


Servo servo_a, servo_b, servo_c, servo_d;

void send_test(void) {

  Serial.write(DEFAULT_TEST_OUT_MSG_LEN);
  Serial.write(0x01);
  
  //outputBuffer = DEFAULT_TEST_OUT_MESSAGE;
  outputBuffer[0] = 'o';
  outputBuffer[1] = 'u';
  outputBuffer[2] = 't';
  outputBuffer[3] = '-';
  outputBuffer[4] = 'c';
  outputBuffer[5] = 'o';
  outputBuffer[6] = 'd';
  outputBuffer[7] = 'e';
  outputBuffer[8] = '-';
  outputBuffer[9] = '6';
  outputBuffer[10] = '3';
  outputBuffer[11] = '3';
  outputBuffer[12] = '4';
  for(int i = 0; i < DEFAULT_TEST_OUT_MSG_LEN; i++)
    Serial.write(outputBuffer[i]);
}

void update_pwm() {
  servo_a.write(map(cnt_a, 0, max_cnt, 0, 180));
  servo_b.write(map(cnt_b, 0, max_cnt, 0, 180));
  servo_c.write(map(cnt_c, 0, max_cnt, 0, 180));
  servo_d.write(map(cnt_d, 0, max_cnt, 0, 180));
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


void setup() {

  Serial.begin(9600);
  Serial.setTimeout(100);

  pinMode(LED_BUILTIN, OUTPUT);

  servo_a.attach(8);
  servo_b.attach(9);
  servo_c.attach(10);
  servo_d.attach(11);

  set_timer();
}

bool usb_test_passed = false;

int readSerial() {

  char c = Serial.read();
  int len = c;

  c = Serial.read();
  for (int i = 0; i < len; i++) {
    c = Serial.read();
    inputBuffer[i] = c;
  }

  return len;
}

String readString;
String inCode = "in-code-9372";
String outCode = "out-code-6334";

void loop() {

  // Syncronization
  while(!Serial.available()) {}
  
  if (Serial.available() > 0) {
    
    int len = Serial.readBytesUntil('\n', inputBuffer, 64);
    Serial.write(inputBuffer, len);
    Serial.write('\n');
  }

  //update_states();
  //update_pwm();

  //delay(5);
}
