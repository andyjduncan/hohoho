#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
//#include <TinyWireM.h> // Enable this line if using Adafruit Trinket, Gemma, etc.

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

//#define DEBUG

// Interrupts

const int pinOne = 2;
const int pinTwo = 3;

volatile long lastDebounceOne = 0;
volatile long lastDebounceTwo = 0;

volatile int countOne = 0;
volatile int countTwo = 0;

const long debounceDelay = 100;    // the debounce time

void readOne() {
  if ((millis() - lastDebounceOne) > debounceDelay) {
    if (digitalRead(pinOne) == HIGH) {
      countOne++;
    }
    lastDebounceOne = millis();
  }
}

void readTwo() {
  if ((millis() - lastDebounceTwo) > debounceDelay) {
    if (digitalRead(pinTwo) == HIGH) {
      countTwo++;
    }
    lastDebounceTwo = millis();
  }
}

// CHANGE THESE THINGS
const int delayTime = 1; // Millis between loops, determines failure proc rate

#ifdef DEBUG
const int failureChance = 2000; // One in this many milliseconds has a chance of failure
#else
const int failureChance = 30000; // One in this many milliseconds has a chance of failure
#endif
const int maxSecsToFail = 2000; // Maximum length of failures in milliseconds
// END OF THINGS TO CHANGE

const int failureRand = failureChance / delayTime;
const int maxCyclesToFail = maxSecsToFail / delayTime;

void (*failures[]) (void) = {
  flickerFailure,
  flickerFailure,
  flickerFailure,
  flickerFailure,
  flickerFailure,
  flickerFailure,
  flickerFailure,
  invertFailure,
  invertFailure,
  invertFailure,
  invertFailure,
  invertFailure,
  flickerOut,
  flickerOut,
  flickerOut,
  flickerOut,
  flickerOut,
  flickerSegmentOut,
  flickerSegmentOut,
  flickerSegmentOut,
  flickerSegmentOut,
  flickerSegmentOut,
  upsideDown,
  leftRight,
  upsideDown,
  leftRight,
  upsideDown,
  leftRight,
  chase,
  chase,
  skyscraper,
  deadBeef
};
const int totalFailures = sizeof(failures) / sizeof(failures[0]);
void (*failureFunction)(void);

int failureCycles = 0;

int failureDigit = 2;

int digitMasks[] = {
  0,
  0,
  0,
  0,
  0
};

static const uint8_t numbertable[] = {
  0x3F, /* 0 */
  0x06, /* 1 */
  0x5B, /* 2 */
  0x4F, /* 3 */
  0x66, /* 4 */
  0x6D, /* 5 */
  0x7D, /* 6 */
  0x07, /* 7 */
  0x7F, /* 8 */
  0x6F, /* 9 */
  0x77, /* a */
  0x7C, /* b */
  0x39, /* C */
  0x5E, /* d */
  0x79, /* E */
  0x71, /* F */
};

static const uint8_t chasetable[] = {
  0x06,
  0x0C,
  0x18,
  0x30,
  0x21,
  0x03
};

const int chaseSteps = sizeof(chasetable) / sizeof(chasetable[0]);

static const uint8_t skytable[] = {
  0x08,
  0x1C,
  0x5C,
  0x7E,
  0x7F,
  0x7F,
  0x7E,
  0x5C,
  0x1C,
  0x08
};

const int skySteps = sizeof(skytable) / sizeof(skytable[0]);

int flickerSegment = 0;

Adafruit_7segment matrix = Adafruit_7segment();

void setup() {
#ifndef __AVR_ATtiny85__
  Serial.begin(115200);
  Serial.println("NO STEELING");
#endif
  matrix.begin(0x70);

#ifdef DEBUG
  matrix.setBrightness(4);
#endif

  pinMode(13, OUTPUT);

  randomSeed(analogRead(0));

  pinMode(pinOne, INPUT);
  pinMode(pinTwo, INPUT);
  attachInterrupt(0, readOne, CHANGE);
  attachInterrupt(1, readTwo, CHANGE);
}

void loop() {
  digitMasks[0] = numbertable[(countOne / 10)];
  digitMasks[1] = numbertable[(countOne % 10)];
  digitMasks[3] = numbertable[(countTwo / 10)];
  digitMasks[4] = numbertable[(countTwo % 10)];

  if (failureCycles == 0 && random(failureRand) == 1) {
    startFailure();
  }
  if (failureCycles > 0) {
    failureCycles++;
    (*failureFunction)();
    if (failureCycles > maxCyclesToFail) {
      endFailure();
    }
  }

  for (int i = 0; i < 5; i++) {
    matrix.writeDigitRaw(i, digitMasks[i]);
  }
  matrix.writeDisplay();
  delay(delayTime);
}

void startFailure() {
#ifdef DEBUG
  digitMasks[2] = 2;
#endif
  digitalWrite(13, HIGH);

  randomDigit(&failureDigit);
  failureFunction = failures[random(0, totalFailures)];
  failureCycles = 1;
}

void randomDigit(int *digit) {
  while (*digit == 2) {
    *digit = random(4);
  }
}

void endFailure() {
#ifdef DEBUG
  digitMasks[2] = 0;
#endif
  digitalWrite(13, LOW);

  failureCycles = 0;
  failureDigit = 2;
  flickerSegment = 0;
}

void flickerFailure() {
  if (random(3) == 2) {
    digitMasks[failureDigit] = 0;
  }
}

void invertFailure() {
  digitMasks[failureDigit] = ~digitMasks[failureDigit];
}

void flickerOut() {
  if (shouldFlickerOut()) {
    digitMasks[failureDigit] = 0;
  }
}

void flickerSegmentOut() {
  while (flickerSegment == 0) {
    flickerSegment = 1 << random(7);
    if (!(digitMasks[failureDigit] & flickerSegment)) {
      flickerSegment = 0;
    }
  }
  if (shouldFlickerOut()) {
    digitMasks[failureDigit] = digitMasks[failureDigit] ^ flickerSegment;
  }
}

boolean shouldFlickerOut() {
  int rand = random(failureCycles, maxCyclesToFail);
  int progress = (maxCyclesToFail - (0.25 * failureCycles));
  return rand > progress;
}


void chase() {
  int chaseStep = currentStep(chaseSteps, 100);
  digitMasks[failureDigit] = chasetable[chaseStep];
}

void skyscraper() {
  int skyStep = currentStep(skySteps, 250);
  digitMasks[failureDigit] = skytable[skyStep];
}

int currentStep(int totalSteps, int totalTime) {
  int cyclesPerStep = max(1, (totalTime / delayTime) / totalSteps);
  return (failureCycles / cyclesPerStep) % totalSteps;
}


void upsideDown() {
  int mask = digitMasks[failureDigit];
  int newMask = (mask & 0x40);
  newMask = newMask | (mask & 0x01) << 3;
  newMask = newMask | (mask & 0x08) >> 3;
  newMask = newMask | (mask & 0x12) << 1;
  newMask = newMask | (mask & 0x24) >> 1;
  digitMasks[failureDigit] = newMask;
}

void leftRight() {
  int mask = digitMasks[failureDigit];
  int newMask = (mask & 0x49);
  newMask = newMask | (mask & 0x02) << 4;
  newMask = newMask | (mask & 0x04) << 2;
  newMask = newMask | (mask & 0x20) >> 4;
  newMask = newMask | (mask & 0x10) >> 2;
  digitMasks[failureDigit] = newMask;
}

void deadBeef() {
  if ((double)failureCycles / maxCyclesToFail < 0.5) {
    digitMasks[0] = numbertable[0x0D];
    digitMasks[1] = numbertable[0x0E];
    digitMasks[3] = numbertable[0X0A];
    digitMasks[4] = numbertable[0x0D];
  } else {
    digitMasks[0] = numbertable[0x0B];
    digitMasks[1] = numbertable[0x0E];
    digitMasks[3] = numbertable[0X0E];
    digitMasks[4] = numbertable[0x0F];
  }
}






