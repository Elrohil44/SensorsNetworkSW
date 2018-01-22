// Enable debug prints to serial monitor
#define MY_DEBUG
// Set to AUTO so that controller assigns ID not in use
#define MY_NODE_ID AUTO
// Enable and select radio type attached
#define MY_RADIO_NRF24

#define TEMPERTATURE_ID 64

#include <SPI.h>
#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Bounce2.h>

#define COMPARE_TEMP 0 // Send temperature only if changed? 1 = Yes 0 = No

#define LED_PIN 6 // Pin where LED is connected
#define LED_ID 254
#define ONE_WIRE_BUS 2 // Pin where dallase sensor is connected

#define BUTTON_PIN 3
#define BUTTON_ID 4
#define MAX_ATTACHED_DS18B20 16


unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)

DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.

float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
bool receivedConfig = false;
bool metric = true;
// Initialize temperature message
MyMessage msg(TEMPERTATURE_ID, V_TEMP);
MyMessage msg_button(BUTTON_ID, V_TRIPPED);
MyMessage msg_led(LED_ID, V_STATUS);

Bounce debouncer = Bounce();

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}

void setup()  
{ 

  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);

  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
  pinMode(LED_PIN, OUTPUT);    
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Sensors_2", "1.2");
  
  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  Serial.begin(115200);
  Serial.print("Sensory: ");
  Serial.println(numSensors);

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(TEMPERTATURE_ID + i, S_TEMP);
  }

  present(BUTTON_ID, S_DOOR);
  present(LED_ID, S_BINARY);
}

void loop()     
{     
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();
  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  wait(conversionTime);

  bool button_state = digitalRead(BUTTON_PIN) == HIGH;
  send(msg_button.set(button_state?"1":"0"));  // Send button_state value to gw
  
  bool led_state = digitalRead(LED_PIN) == HIGH;
  send(msg_led.set(led_state?"1":"0");

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
    // Only send data if temperature has changed and no error
    #if COMPARE_TEMP == 1
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
    #else
    if (temperature != -127.00 && temperature != 85.00) {
    #endif
      // Send in the new temperature
      send(msg.setSensor(TEMPERTATURE_ID + i).set(temperature,1));
      // Save new temperatures for next compare
      lastTemperature[i]=temperature;
    }
  }
  wait(SLEEP_TIME, C_SET, V_STATUS)
}

void receive(const MyMessage &message){
  if (message.type == V_STATUS) {
    digitalWrite(LED_PIN, message.getBool() ? HIGH : LOW);
    saveState(message.sensor, message.getBool());
  }
}


