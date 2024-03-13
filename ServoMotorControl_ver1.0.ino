#include <Servo.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

Servo myservo;
int pos = 0;
int targetPos;
unsigned long startTime;
unsigned long elapsedTime;
int pauseDuration = 0;
int sleepDuration = 0;
int sleepLoop = 0; 
const int totalMovementDuration = 8000;
const int movementDuration = totalMovementDuration / 2;
const int buttonPin = 2;
volatile char sleepCnt = 0; // Counter for sleep duration

void setup() {
    Serial.begin(9600);
    myservo.attach(9);
    randomSeed(analogRead(0));
    pinMode(buttonPin, INPUT_PULLUP);

}

void loop() {
    int randomIndex = random(3);

    if (randomIndex == 0)
        targetPos = 22;
    else if (randomIndex == 1)
        targetPos = 44;
    else
        targetPos = 66;

    Serial.print("Target Position: ");
    Serial.println(targetPos);

    startTime = millis();
    moveServo(targetPos);

    if (digitalRead(buttonPin) == LOW) {
        pauseDuration = 10000;
        sleepDuration = 10000;
    } else {
        pauseDuration = 20000;
        sleepDuration = 20000;
    }

    unsigned long pauseStartTime = millis();
    while (millis() - pauseStartTime < pauseDuration) {
        elapsedTime = millis() - pauseStartTime;
        Serial.print("Pause Elapsed Time: ");
        Serial.println(elapsedTime);
        delay(1);
    }

     // Set sleep counter based on sleep duration
     sleepLoop = sleepDuration / 8000; // Convert milliseconds to seconds

    // Disable the ADC (Analog to digital converter, pins A0 [14] to A5 [19])
    static byte prevADCSRA = ADCSRA;
    ADCSRA = 0;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    while (sleepCnt < sleepLoop) {

      // Turn of Brown Out Detection (low voltage). This is automatically re-enabled upon timer interrupt
      sleep_bod_disable();
  
      // Ensure we can wake up again by first disabling interrupts (temporarily) so
      // the wakeISR does not run before we are asleep and then prevent interrupts,
      // and then defining the ISR (Interrupt Service Routine) to run when poked awake by the timer
      noInterrupts();
  
      // clear various "reset" flags
      MCUSR = 0;  // allow changes, disable reset
      WDTCSR = bit (WDCE) | bit(WDE); // set interrupt mode and an interval
      WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);   //| bit(WDP0);    // set WDIE, and 8 second delay
      wdt_reset();
  
      // Send a message just to show we are about to sleep
      Serial.println("Good night!");
      Serial.flush();
  
      // Allow interrupts now
      interrupts();
  
      // And enter sleep mode as set above
      sleep_cpu();
    }

  // --------------------------------------------------------
  // µController is now asleep until woken up by an interrupt
  // --------------------------------------------------------

  // Prevent sleep mode, so we don't enter it again, except deliberately, by code
  sleep_disable();

  // Wakes up at this point when timer wakes up µC
  Serial.println("I'm awake!");

  // Reset sleep counter
  sleepCnt = 0;

  // Re-enable ADC if it was previously running
  ADCSRA = prevADCSRA;

  
}

ISR (WDT_vect) {

    // Turn off watchdog, we don't want it to do anything (like resetting this sketch)
    wdt_disable();
  
    // Increment the WDT interrupt count
    sleepCnt++;
  
    // Now we continue running the main Loop() just after we went to sleep
}

void moveServo(int target) {
    int increment;
    int totalSteps;
    int currentPos = pos;

    if (target > pos) {
        totalSteps = target - pos;
        increment = 1;
    } else {
        totalSteps = pos - target;
        increment = -1;
    }

    unsigned long stepDuration = movementDuration / totalSteps;

    while (currentPos != target) {
        elapsedTime = millis() - startTime;
        int stepsElapsed = elapsedTime / stepDuration;
        currentPos = pos + (stepsElapsed * increment);
        myservo.write(currentPos);

        Serial.print("Current Position: ");
        Serial.print(currentPos);
        Serial.print(", Elapsed Time: ");
        Serial.println(elapsedTime);

        delay(stepDuration);
    }

    pos = target;

    delay(50);

    Serial.println("Returning to 0");

    startTime = millis();

    while (currentPos != 0) {
        elapsedTime = millis() - startTime;
        int stepsElapsed = elapsedTime / stepDuration;
        currentPos = target - (stepsElapsed * increment);
        myservo.write(currentPos);

        Serial.print("Current Position: ");
        Serial.print(currentPos);
        Serial.print(", Elapsed Time: ");
        Serial.println(elapsedTime);

        delay(stepDuration);
    }

    pos = 0;
}
