#include "mbed.h"
#include <cstdio>
#include <cstring>

using namespace std::chrono;

#define BOARD_ID 228
#define PERIOD 240

// Serial
#define TX PA_9
#define RX PA_10
#define BAUDRATE 115200

// LED
#define LED2PIN PB_4

// PHOTORESISTOR
#define PHOTOPIN PA_6

// Display
#define RS PB_11
#define E PB_10
#define D4 PB_12
#define D5 PB_15
#define D6 PB_13
#define D7 PB_14

static UnbufferedSerial pc(TX, RX, BAUDRATE);
static DigitalOut led2(LED2PIN, PullUp);
static AnalogIn sensor(PHOTOPIN);

static DigitalOut rs(RS, 0);
static DigitalOut e(E, 0);
static DigitalOut d4(D4, 0);
static DigitalOut d5(D5, 0);
static DigitalOut d6(D6, 0);
static DigitalOut d7(D7, 0);

FileHandle *mbed::mbed_override_console(int fd) { return &pc; }

void send(bool isCommand, uint8_t data) {
  rs.write(!isCommand);
  ThisThread::sleep_for(5ms);

  d7.write((data >> 7) & 1);
  d6.write((data >> 6) & 1);
  d5.write((data >> 5) & 1);
  d4.write((data >> 4) & 1);

  e.write(1);
  ThisThread::sleep_for(5ms);
  e.write(0);
  ThisThread::sleep_for(5ms);

  d7.write((data >> 3) & 1);
  d6.write((data >> 2) & 1);
  d5.write((data >> 1) & 1);
  d4.write((data >> 0) & 1);
  ThisThread::sleep_for(5ms);

  e.write(1);
  ThisThread::sleep_for(5ms);
  e.write(0);
  ThisThread::sleep_for(5ms);
}

void sendCommand(uint8_t cmd) { send(true, cmd); }

void sendChar(const char chr) { send(false, chr); }

void sendString(const char *str) {
  while (*str != '\0') {
    sendChar(*str);
    str++;
  }
}

void initDisplay() {
  sendCommand(0b00110000);
  sendCommand(0b00000010);
  sendCommand(0b00001111);
  sendCommand(0b00000001);
  sendCommand(0b10000000);
}

void updateDisplay() {
  sendCommand(0b00000001);
  sendCommand(0b10000000);
}

int main() {
  initDisplay();
  sendString("Boot completed");

  Timer timer;
  timer.start();
  unsigned long long elapsed = 0;
  float percent = 0.5;
  int test = PERIOD * percent;

  char symbol;

  while (true) {
    elapsed = duration_cast<milliseconds>(timer.elapsed_time()).count();
    int test2 = elapsed % PERIOD;
    if (test2 >= test) {
      led2 = 0;
    } else if (test2 < test) {
      led2 = 1;
    }

    if (pc.readable()) {
      pc.read(&symbol, 1);

      if (symbol == 'i') {
        printf("%d\r\n", BOARD_ID);
      }

      if (symbol == 'g') {
        int value = (int)sensor.read_u16();
        printf("%d\r\n", value);
        char buff[10] = {};
        sprintf(buff, "%d", value);
        updateDisplay();
        sendString(buff);
      }
    }
  }
}