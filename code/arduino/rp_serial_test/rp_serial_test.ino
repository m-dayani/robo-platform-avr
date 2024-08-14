/*
The way robo-platform's test sequence works:
  1. Server (phone) sends a specific code: 
    The first byte is always data length and the second shows cmd or data stat
    The code starts from the third byte: in-code-9372
  2. If the in-code is correct, send: out-code-6334 with the same format
  3. [optional] toggle a LED indicating test was successful
*/

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

void setup() {

  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

  while(!Serial.available()) {}
  
  if (Serial.available() > 0) {
    int len = 0;
    unsigned char code = 0;
    my_serial_read(len, code, inputBuff, buffLen);

    if (check_test(len, inputBuff)) {
      send_test_response();
    }
    else if (len == 1) {
      if (code == UsbCommand::CMD_UPDATE_OUTPUT) {
        // Directional commands
        if (inputBuff[0] == 0x01) { //W
          digitalWrite(LED_BUILTIN, HIGH);
        }
        if (inputBuff[0] == 0x02) { //S
          digitalWrite(LED_BUILTIN, LOW);
        }
      }
      else if (code == UsbCommand::CMD_TEST_LTC) {
        // Latency test, echo the timestamp ID back
        if (len == 1) {
          len += 1;
          inputBuff[1] = 0;
        }
        my_serial_write(len, code, inputBuff, buffLen);
      }
    }
    else if (code == UsbCommand::CMD_TEST_TP) {
      // Throughput test, count successful bytes and send the number back
      int bytes_cnt = 0;
      for (int i = 0; i < len; i++) {
        if (i % 2 == 0) {
          if (inputBuff[i] == 0x55) {
            bytes_cnt++;
          }
        }
        else {
          if (inputBuff[i] == 0xAA) {
            bytes_cnt++;
          }
        }
      }
      outputBuff[0] = bytes_cnt % 64;
      outputBuff[1] = (int) (bytes_cnt / 64);
      my_serial_write(2, code, outputBuff, buffLen);
    }
    else {
      my_serial_write(len, code, inputBuff, buffLen);
    }
  }
}
