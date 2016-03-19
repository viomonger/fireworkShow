
// PIN DEFINITIONS

// digital output to energize firing channels (triggers)
int trigger[8] = {D0,D1,D2,D3,D4,D5,D6,TX};

// analog input to sample resistance across firing channel (e-matches)
int emRes[8] = {A0,A1,A2,A3,A4,A5,DAC,WKP};

// digital input logic-level indication of HW-Arm/trigger circuits are powered
int hwArm = RX;

// digital output to energize the resistance sampling circuitry
int emResSenseSwitch = D7;


// GLOBAL VARIABLES

// variable to keep track of SW Arm state
bool armed = false;

// variable to store latest sampled HW Arm state
int hwArmVal;

// variable to store latest sampled resistance values
int emResVal[8];

// char array to hold telemetry strings for publishing
char telemetry[256];


// the function that collects telemetry, hooked up to a software timer
void collectTelemetry() {
    
    // COLLECT TELEMETRY (READ RESISTANCE and HW ARM)
    
    // Power on sense circuitry
    digitalWrite(emResSenseSwitch, HIGH);
    
    // TODO: delay for charging of sense circuitry?
    // delayMicroseconds(10);
    
    // read e-match resistance values
    for (int i=0; i<8; i++) {
        emResVal[i] = analogRead(emRes[i]);
    }
    
    // Power off sense circuitry
    digitalWrite(emResSenseSwitch, LOW);
    
    // read hardware arm value
    hwArmVal = digitalRead(hwArm);
    
    // format json telemetry string
    // note: coreID is passed with publish
    sprintf(telemetry, "{\"swArm\": %u, \"hwArm\": %u, \"R0\": %u, \"R1\": %u, \"R2\": %u, \"R3\": %u, \"R4\": %u, \"R5\": %u, \"R6\": %u, \"R7\": %u}", armed, hwArmVal, emResVal[0], emResVal[1], emResVal[2], emResVal[3], emResVal[4], emResVal[5], emResVal[6], emResVal[7]);
    
    // publish private telemetry string
    Particle.publish("superawesomefireworktelemetry", telemetry, 1, PRIVATE);
}


// set up software timer for telemetry publish
// docs say publish can be about 1 Hz? timer period can be modified if high-rate/low-rate modes desired
Timer tlmTimer(1500, collectTelemetry);


// setup function is run once on init / restart
void setup() {
    
    // SET PIN MODES
    for (int i=0; i<8; i++) {
        
        // trigger pins are output
        pinMode(trigger[i], OUTPUT);
        
        // resistance pins are input
        pinMode(emRes[i], INPUT);
    }
    
    // HW Arm pin is input
    pinMode(hwArm, INPUT);
    
    // resistance power switch is output
    pinMode(emResSenseSwitch, OUTPUT);
    
    
    // ENFORCE OUTPUTS LOW (for truth)
    
    // set triggers LOW
    for (int i=0; i<8; i++) {
        digitalWrite(trigger[i], LOW);
    }
    
    // set resistance power switch LOW
    digitalWrite(emResSenseSwitch, LOW);
    
    
    // REGISTER CLOUD FUNCTIONS
    
    // arm function to set the SW Arm state TRUE
    Particle.function("arm", swArm);
    
    // disarm function to set the SW Arm state FALSE
    Particle.function("disarm", swDisarm);
    
    // fire function to energize 1 or more triggers
    Particle.function("fire", energizeTrigger);
    
    
    // START SOFTWARE TIMERS
    
    // start telemetry timer
    tlmTimer.start();
}


// Software Arm cloud function callback
// Returns 0 if armed, -1 if state not changed
int swArm(String command) {
    
    // only accept the arming function if the correct command is paired with it
    if (command == "supersecretarmingcode") {
        armed = true;
        return 0;
    }
    else {
        return -1;
    }
}

// Software Disarm cloud function callback
// disarms and returns 0
int swDisarm(String command) {
    armed = false;
    return 0;
}

// Energize Trigger cloud function callback
// No signal cases: Returns -1 if not SW armed, -2 if empty command, -3 if bad command
// Signal case: Returns number of channels triggered
// Returns 0 if a trigger is energized, -1 if bad command is received, -2 if not SW armed
int energizeTrigger(String command) {
    
    // check arming (sw only)
    if (!armed) {
        return -1;
    }
    
    // TOOD: wtfbbqhax
    if (command != "") {
        char inputStr[64];
        command.toCharArray(inputStr, 64);
        
        // variable to count commanded triggers
        int numTriggered = 0;
        
        // parse command to energize triggers
        char *p = strtok(inputStr, ",");
        int commanded;
        bool triggered[8] = {false, false, false, false, false, false, false, false};
        for (int i=0; i<8; i++) {
            commanded = atoi(p);
            if (commanded == i) {
                numTriggered++;
                triggered[i] = true;
                p = strtok(NULL, ",");
            }
        }
        
        // return -3 and signal no triggers if not a good command
        if (numTriggered == 0) {
            return -3;
        }
        
        // energize chosen triggers
        for (int i=0; i<8; i++) {
            if (triggered[i]) {
                digitalWrite(trigger[i], HIGH);
            }
        }
        
        // TODO: how long to hold trigger? this delay is synchronous! ... ?
        // TODO: determine if this delay can be interrupted, and possibly disable interrupts here...
        delay(15);
        
        // de-energize chosen triggers
        for (int i=0; i<8; i++) {
            if (triggered[i]) {
                digitalWrite(trigger[i], LOW);
            }
        }
        
        return numTriggered;
    }
    else {
        return -2;
    }
}

// the thing that doesn't stop til the fat lady sings, or kyle resets it
void loop() {
}

