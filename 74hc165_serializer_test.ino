/**
 * This sketch is used to test the 8-Bit parallel in/serial out shift register IC 74HC165.
 *   Copyright (c) 2021, maehw; for details check LICENSE
 * 
 * The specific use case and pinout has been intended for for testing the 
 * "splitflap classic controller" shield in combination with the magnetic switches (hall-effect sensors);
 * see also: https://github.com/scottbez1/splitflap
 * 
 * Holding magnets near the sensors should toggle the output from high to low.
 */

/* I/O configuration */
#if defined(__AVR_ATmega32U4__) /* working with an Arduino Leonardo here */
  /* SCK and MISO pins are usually used for SPI communication, 
   * but here "bitbanged" as GPIOs.
   */
  #define CLK_PIN              (15) /* "SPI_CLOCK"/SCK */
  #define QH_PIN               (14) /* "SENSOR_DATA_OUT"/MISO */
  #define nLD_SH_PIN           ( 5) /* "SENSOR_LATCH" */
#endif

/** 
 * The serial input pin is usually pulled low (resistor to GND),
 * but could also be daisy chained between other ICs and then pulled low, 
 * or be controlled by a uC and therefore be defined as an output pin here.
 */
//#define SER_IN_PIN         ()   /* "SENSOR_DATA_IN"; usually pull-up resistor to GND but could also be daisy chained between other ICs and then pulled low, or be controlled here */

#define NUM_INPUTS           (8)  /* Number of inputs to load and shift out serially; one IC has 8 input pins (labelled A..H);
                                   * pins are shifted out in the order H..A, i.e. in order to get A's value you must read at least 8 inputs */
#define CLK_PERIOD_HALF_US   (50) /* Approximate (minimum) half clock period (low/high duration) */
#define READ_DELAY_MS        (200) /* Delay between consecutive reads ("pause") */
#define SERIAL_BAUDRATE      (115200)
#define SERIAL_PRINT_WELCOME (1)
#define SERIAL_PRINT_HEADER  (1)
#define SERIAL_SYMBOL_HIGH   ("H") /* could be any string, e.g. "high", "1", "-", ... */
#define SERIAL_SYMBOL_LOW    ("L") /* could be any string, e.g. "low", "0", "_", ... */

const char sHeader[] = "HGFEDCBA"; /* Name of input pins */
uint32_t g_nValue = 0; /* The binary value which has been serialized (newest bit is stored at the LSB; value is continuously shifted to the left) */

void setup()
{
  /* I/O configuration */
  pinMode(nLD_SH_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
#ifdef SER_IN_PIN
  pinMode(SER_IN_PIN, OUTPUT);
#endif

  digitalWrite(nLD_SH_PIN, HIGH);
  digitalWrite(CLK_PIN, HIGH);
#ifdef SER_IN_PIN
  pinMode(SER_IN_PIN, LOW);
#endif

  pinMode(QH_PIN, INPUT);

  /* pause before final start */
  delay(2000);

  Serial.begin(SERIAL_BAUDRATE);
#if SERIAL_PRINT_WELCOME
  Serial.println("74HC165 Serializer Test");
  Serial.println("-----------------------");
#endif
}

void clock_cylce()
{
  digitalWrite(CLK_PIN, LOW);
  delayMicroseconds(CLK_PERIOD_HALF_US);
  digitalWrite(CLK_PIN, HIGH);
  delayMicroseconds(CLK_PERIOD_HALF_US);
}

void reset_value()
{
  g_nValue = 0;
}

void read_serial_in()
{
  g_nValue = g_nValue << 1;
  int sensorVal = digitalRead(QH_PIN);
  if( HIGH == sensorVal )
  {
     g_nValue |= 1;
  }
}

void output_value()
{
  /* header (with device numbers and pins names) */
#if SERIAL_PRINT_HEADER
  for( int k = 0; k < NUM_INPUTS; k++ )
  {
    if( k % 8 == 0 )
    {
      Serial.print(" ");
      Serial.print(k/8, HEX);
    }
    Serial.print( sHeader[k % 8] );
  }
  Serial.println();
#endif

  /* values: H=1, L=0 */
  for( int k = NUM_INPUTS-1, m = 0; k >= 0; k--, m++ )
  {
    if( m % 8 == 0 )
    {
      Serial.print("  ");
    }

    if(g_nValue & (1 << k))
    {
       Serial.print(SERIAL_SYMBOL_HIGH);
    }
    else
    {
       Serial.print(SERIAL_SYMBOL_LOW);
    }
  }
  Serial.println();
}

void loop()
{
  reset_value();

  /* cycle new data in */
  digitalWrite(nLD_SH_PIN, LOW); /* parallel load */
  clock_cylce();
  digitalWrite(nLD_SH_PIN, HIGH); /* shift mode */
  read_serial_in(); /* stable at transition high -> low */
  /* shift in H..A for every device */
  for( int k = 0; k < NUM_INPUTS-1; k++ )
  {
    clock_cylce();
    read_serial_in(); /* stable at transition high -> low */
  }
  clock_cylce(); /* one additional clock cycle */

  /* dump new value, wait and repeat */
  output_value();
  delay(READ_DELAY_MS); /* no clock cycling during the given period */
}
