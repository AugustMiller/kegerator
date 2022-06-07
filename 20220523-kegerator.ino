// Pin Assignments
// =============================================================================

const int TEMP_INTERIOR = A0;
const int TEMP_OVERLOAD = A1;
const int TEMP_COMPRESSOR = A2;

const int COMPRESSOR_RUN = 2;
const int COMPRESSOR_START = 3;

// Usage-Independent Constants
// =============================================================================

const int PIN_MAX = 1023;
const int PIN_MIN = 0;
const float V_REF = 5.0;
const int POLL_INTERVAL = 1000;

// Characteristics
// =============================================================================

const float NOMINAL_TEMP = 298.15;    // Degrees Kelvin when resistor is expected to be at the nominal resistance (~25C)
const int NOMINAL_RESISTANCE = 10000; // Resistance when at the nominal temperature.
const int BETA = 3950;                // Rate-of-change constant for the thermistor

// Preferences
// =============================================================================

float targetTemp = 37.0;                 // Temperature target
float offsetTrigger = 2.0;               // Threshold to start (and stop?) cooling
float hysteresis = 240 * POLL_INTERVAL;  // Minimum time between mode switches

// State
// =============================================================================

const int intervals = 30;
float history [intervals];
int pointer = 0;

int samplesAvailable = 0;
int currentModePeriod = 0;
bool isCooling = false;
bool isStarting = false;

// Loop
// =============================================================================

void setup() {
  pinMode(TEMP_INTERIOR, INPUT);

  Serial.begin(9600);
}

void loop() {
  float v = analogRead(TEMP_INTERIOR) * V_REF / PIN_MAX;
  float r = (v * NOMINAL_RESISTANCE) / (V_REF - v);

  float tK = 1 / ((1 / NOMINAL_TEMP) + ((log(r / NOMINAL_RESISTANCE)) / BETA));
  float tC = tK + (25.0 - NOMINAL_TEMP);
  float tF = tC * (9.0 / 5.0) + 32.0;

  logReading(tF);

  float a = getMovingAverage();

  Serial.print("Current (F): ");
  Serial.println(tF);

  Serial.print("Moving Average (F): ");
  Serial.println(a);

  // Has it been long enough since the last state change?
  if (currentModePeriod >= hysteresis) {
    // Should we start cooling?
    if (tF > targetTemp + offsetTrigger) {
      cool();
    }

    // Have we cooled enough and are ready to idle?
    if (tF < targetTemp - offsetTrigger) {
      idle();
    }

    // If we're within range, we don't need to change state.
  }

  // No change, just log the time spent in this state:
  currentModePeriod = currentModePeriod + POLL_INTERVAL;

  delay(POLL_INTERVAL);
}

// Math Helpers
// =============================================================================

/**
 * Adds a value to the temperature history log.
 * 
 * @param float t Temperature to log
 */
void logReading(float t) {
  history[pointer] = t;

  // Update the number of samples available, :
  samplesAvailable = min(intervals, samplesAvailable + 1);

  // Update pointer, starting over at the beginning, if necessary:
  pointer = (pointer + 1) % intervals;
}

/**
 * Calculates a moving average for the number of currently-available,
 * moment-in-time temperature values.
 * 
 * @return float
 */
float getMovingAverage() {
  float s = 0;

  // Only sum up to the number of samples taken—otherwise, we bias low due to empty indexes:
  for (int i = 0; i < samplesAvailable; i++) {
    s += history[i];
  }

  return s / samplesAvailable;
}

// State Helpers
// =============================================================================

/**
 * Starts a cooling cycle.
 */
void cool() {
  currentModePeriod = 0;
  isCooling = true;
  isStarting = true;

  digitalWrite(COMPRESSOR_START, HIGH);
  digitalWrite(COMPRESSOR_RUN, HIGH);
}

/**
 * Forces an idle state.
 */
void idle() {
  currentModePeriod = 0;
  isCooling = false;
  isStarting = false;

  digitalWrite(COMPRESSOR_START, LOW);
  digitalWrite(COMPRESSOR_RUN, LOW);
}