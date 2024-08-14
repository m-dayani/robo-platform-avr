#include <time.h> 
#include <math.h>

#include <Servo.h>


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

unsigned short max_cnt = 2000;
unsigned long cnt_a = 0, cnt_b = 0, cnt_c = 0, cnt_d = 0;
char state_a = 0, state_b = 0, state_c = 0, state_d = 0;
int n_cycle = 0;

unsigned char ctrl_state = 0x00;


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

void update_pwm() {
  servo_a.write(map(cnt_a, 0, max_cnt, 0, 180));
  servo_b.write(map(cnt_b, 0, max_cnt, 0, 180));
  servo_c.write(map(cnt_c, 0, max_cnt, 0, 180));
  servo_d.write(map(cnt_d, 0, max_cnt, 0, 180));
}

void update_cnt(unsigned long *cnt, const char state) {
    *cnt += state;
    if (*cnt > max_cnt)
        *cnt = max_cnt;
    if (*cnt < 0)
        *cnt = 0;
}

char update_state(const unsigned char mask, const unsigned char n_bit) {
    unsigned char pcond = (ctrl_state & mask) >> n_bit;
    unsigned char ncond = (ctrl_state & (mask << 1)) >> (n_bit + 1);
    if (pcond)
        return 1;
    else if (ncond)
        return -1;
    else
        return 0;
}

void update_states(void) {
    state_a = update_state(0x01, 0);
    state_b = update_state(0x04, 2);
    state_c = update_state(0x10, 4);
    state_d = update_state(0x40, 6);
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

void set_timer(void) {
    // cli();//stop interrupts
    noInterrupts();

    // set timer0 interrupt at 2kHz
    TCCR0A = 0; // set entire TCCR0A register to 0
    TCCR0B = 0; // same for TCCR0B
    TCNT0 = 0;  // initialize counter value to 0
    // set compare match register for 2khz increments
    OCR0A = 124; // = (16*10^6) / (2000*64) - 1 (must be <256)
    // turn on CTC mode
    TCCR0A |= (1 << WGM01);
    // Set CS01 and CS00 bits for 64 prescaler
    TCCR0B |= (1 << CS01) | (1 << CS00);
    // enable timer compare interrupt
    TIMSK0 |= (1 << OCIE0A);

    // sei();//allow interrupts
    interrupts();
}


void setup() {

  Serial.begin(9600);
  // Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(LED_BUILTIN, OUTPUT);

  servo_a.attach(6);
  servo_b.attach(9);
  servo_c.attach(10);
  servo_d.attach(11);

  set_timer();
}

void loop() {

  // Syncronization
  while(!Serial.available()) {}
  
  if (Serial.available() > 0) {
    
    int len = 0;
    unsigned char code = 0;
    my_serial_read(len, code, inputBuff, buffLen);

    if (check_test(len, inputBuff)) {
      send_test_response();
    }
    else if (code == UsbCommand::CMD_UPDATE_OUTPUT) {
      // Directional commands
      ctrl_state = inputBuff[0];
    }
    else {
      my_serial_write(len, code, inputBuff, buffLen);
    }
  }

  update_states();
  update_pwm();

  delay(5);
}
