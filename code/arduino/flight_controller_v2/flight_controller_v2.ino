#include <time.h> 
#include <math.h>
#include <Servo.h>

#define PWM_A 3
#define PWM_B 9
#define PWM_C 10
#define PWM_D 11


enum UsbCommand {
  CMD_BROADCAST,
  CMD_UPDATE_OUTPUT,
  CMD_GET_SENSOR_INFO,
  CMD_GET_CMD_RES,
  CMD_ADC_START,
  CMD_ADC_READ,
  CMD_ADC_STOP,
  CMD_RUN_TEST,
  CMD_TEST_LTC,
  CMD_TEST_TP,
  CMD_STR_MSG
};


const int buffLen = 64;
unsigned char inputBuff[buffLen];
unsigned char outputBuff[buffLen];

const String inCode = "in-code-9372";
const char LenInCode = 12;
const String outCode = "out-code-6334";
const char LenOutCode = 13;
bool test_done = false;

Servo servo_a, servo_b, servo_c, servo_d;
bool pwm_mode = false;

unsigned char state_a = 0, state_b = 0, state_c = 0, state_d = 0;
unsigned char max_cnt = 255;
// unsigned char ctrl_state = 0x00;

bool led_state = false;


void my_serial_read(int& len, unsigned char& code, unsigned char* buffer, int max_buff) {

  unsigned char cmd[2];
  Serial.readBytes(cmd, 2);
  len = cmd[0];
  code = cmd[1];
  if (code == UsbCommand::CMD_TEST_TP && buffLen > 64) {
    len = len * 64 - 2;
  }
  Serial.readBytes(buffer, len);
}

void my_serial_write(int len, unsigned char code, unsigned char* buffer, int max_buff) {

  if (len > max_buff) {
    return;
  }
  Serial.write(len);
  Serial.write(code);
  for (int i = 0; i < len; i++) {
    Serial.write(buffer[i]);
  }
}

bool check_test(int len, unsigned char* buffer) {

  if (len != LenInCode) {
    return false;
  }
  for (int i = 0; i < len; i++) {
    if (buffer[i] != inCode[i]) {
      return false;
    }
  }
  test_done = true;
  return true;
}

void send_test_response() {
  my_serial_write(LenOutCode, 0, outCode.c_str(), LenOutCode);
}

void toggle_led(void) {

  if (!led_state) {
    digitalWrite(LED_BUILTIN, HIGH);
    led_state = true;
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
    led_state = false;
  }
}

void test_analog() {
  int sensorValue = analogRead(A0);
  int outputValue = map(sensorValue, 0, 1023, 0, 255);
  analogWrite(PWM_A, outputValue);
  analogWrite(PWM_B, outputValue);
  analogWrite(PWM_C, outputValue);
  analogWrite(PWM_D, outputValue);
}

void update_analog() {

  // unsigned char value = (unsigned char) (((float) cnt_a / (float) max_cnt) * 255.0);
  analogWrite(PWM_A, map(state_a, 0, max_cnt, 0, 255));
  analogWrite(PWM_B, map(state_b, 0, max_cnt, 0, 255));
  analogWrite(PWM_C, map(state_c, 0, max_cnt, 0, 255));
  analogWrite(PWM_D, map(state_d, 0, max_cnt, 0, 255));
}

void update_pwm() {

  servo_a.write(map(state_a, 0, max_cnt, 0, 180));
  servo_b.write(map(state_b, 0, max_cnt, 0, 180));
  servo_c.write(map(state_c, 0, max_cnt, 0, 180));
  servo_d.write(map(state_d, 0, max_cnt, 0, 180));
}

void update_states(unsigned char* buffer, unsigned char len) {

    if (len < 4) {
      return;
    }
    state_a = buffer[0];
    state_b = buffer[1];
    state_c = buffer[2];
    state_d = buffer[3];
}

void update_output_with_states(void) {
  outputBuff[0] = state_a;
  outputBuff[1] = state_b;
  outputBuff[2] = state_c;
  outputBuff[3] = state_d;
}

void setup_pwm(void) {
  servo_a.attach(PWM_A);
  servo_b.attach(PWM_B);
  servo_c.attach(PWM_C);
  servo_d.attach(PWM_D);
}


void setup() {

  // Serial.begin(9600);
  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(LED_BUILTIN, OUTPUT);

  if (pwm_mode) {
    setup_pwm();
  }
}

void loop() {

  // Syncronization
  // while(!Serial.available()) {}
  
  if (Serial.available() > 0) {
    
    int len = 0;
    unsigned char code = 0;
    my_serial_read(len, code, inputBuff, buffLen);

    if (check_test(len, inputBuff)) {
      send_test_response();
    }
    else if (code == UsbCommand::CMD_UPDATE_OUTPUT) {
      // Directional commands
      // update current states
      update_states(inputBuff, len);
      // send back last state -> maybe this is redundant??
      update_output_with_states();
      my_serial_write(len, UsbCommand::CMD_GET_CMD_RES, outputBuff, buffLen);
    }
    else {
      my_serial_write(len, code, inputBuff, buffLen);
    }
  }

  if (pwm_mode) {
    update_pwm();
  }
  else {
    update_analog();
    // test_analog();
  }
  // delay(5);
}
