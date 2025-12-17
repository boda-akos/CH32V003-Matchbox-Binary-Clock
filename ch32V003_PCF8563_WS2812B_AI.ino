// This setup runs on 24Mhz internal clock only - power save requirement
// PMOS AO3401 power switch to WS2812B + supply, 16 LED matrix , see arrangement 
//       8   0
//       9 6 1
//   13 10 5 2
//   12 11 4 3

// PCF8563 clock chip + 32768Hz crystal
// Li Ion battery, charger TP4056, 1k2 resistor -> 6k8 reduced charge current
// TTP223 touch sensor to pin PA2
// LED power to pin PC3 

#define led_num 16

typedef struct {
    byte r;  // Red component (0-255)
    byte g;  // Green component (0-255)
    byte b;  // Blue component (0-255)
} WS_Color;

// Pre-defined colors
#define WS_RED (WS_Color){255, 0, 0}
#define WS_GREEN (WS_Color){0, 255, 0}
#define WS_BLUE (WS_Color){0, 0, 255}
#define WS_WHITE (WS_Color){255, 255, 255}
#define WS_BLACK (WS_Color){0, 0, 0}
#define WS_PURPLE (WS_Color){128, 0, 128}
#define WS_YELLOW (WS_Color){255, 255, 0}
#define WS_CYAN (WS_Color){0, 255, 255}
#define WS_ORANGE (WS_Color){255, 165, 0}
#define WS_PINK (WS_Color){255, 20, 147}

void WS_SetBrightness(byte brightness);
void WS_SetPixel(byte index, WS_Color color);
void WS_Fill(WS_Color color);
void WS_Clear(void);
void WS_Show(void);

WS_Color WS_RGB(byte r, byte g, byte b);
static WS_Color led_buffer[16];

byte a = 8;  // intensity

// PCF8523:  SDA pin 5 -> to PC1 pin 11
//           SCL pin 6 -> to PC2 12
 // LED VCC PC3 via PMOS  Data PC4

#include "Wire.h"
#define PCF8563address 0x51

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

void setup() {
    //Serial.begin(115200); Serial.setTimeout(1);
    
    // change the following to set your initial time
    second = 00;
    minute = 0x32;
    hour = 0x14;
    dayOfWeek = 4;
    dayOfMonth = 14;
    month = 11;
    year = 25;

    //This GPIO configuration became part of GPIOConfig
    //pinMode(PC3, OUTPUT); pinMode(PC4, OUTPUT); // LED power and data
    //while(1) {GPIOC->OUTDR |=(1<<3); delay(1); GPIOC->OUTDR &=~(1<<3); delay(1);}

    // pinMode(PC1, INPUT_PULLUP); pinMode(PC2, INPUT_PULLUP);  // i2c to PCF8263
    // pinMode(PC5, INPUT_PULLUP); pinMode(PC6, INPUT_PULLUP);
    
    GPIOConfig();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate(); 

    EXTI2_INT_INIT();
  
    Wire.begin(); 
    
    // comment out the next line and upload again to set and keep the time from resetting 
    //setPCF8563();
      GPIOC->OUTDR &= ~(1 << 3); 

    // Initialize all pixels just to see 
   //  colorWipe(WS_RED); colorWipe(WS_GREEN);colorWipe(WS_BLUE);
}

void loop() {  
    readPCF8563();
    GPIOC->OUTDR &= ~(1 << 3);  //turn LED power on
    delay(10);
    led();
   
    // =============== Adjusting time =================
    // Pullup input PC5 and PC6 pin is connected to tactile switches 
    bool adjusted = false;
    
    while (digitalRead(PC5) == 0) {
        hour++; 
        if ((hour & 0x0f) == 0x0a) hour += 6;  
        if (hour > 0x23) hour = 0;
        led();
        delay(500); 
        adjusted = true;
    } // adjust hours
    
    while (digitalRead(PC6) == 0) {
        minute++; 
        if ((minute & 0x0f) == 0x0a) minute += 6;  
        if (minute > 0x59) minute = 0;
        led();
        delay(500); 
        adjusted = true;
    } // adjust minutes
    
    if (adjusted) {       // ============ writing to RTC ========
        setPCF8563();
    }
    
    delay(1000);
    GPIOC->OUTDR |= (1 << 3);  
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);
    // Reset follows after interrupt on PA2
}

void led(void) {   // displays global variables second, minute, hour
    for (byte i = 0; i < 16; i++) 
        WS_SetPixel(i, WS_BLACK); 
    
    // minute
    if (minute & 1)  WS_SetPixel(3, WS_RED);    // red WS_SetPixel(5, WS_ORANGE);
    if (minute & 2)  WS_SetPixel(2, WS_YELLOW); // yellow
    if (minute & 4)  WS_SetPixel(1, WS_GREEN);  // green
    if (minute & 8)  WS_SetPixel(0, WS_PURPLE); // magenta
    
    // 10 minute
    if (minute & 0x10)  WS_SetPixel(4, WS_RED);
    if (minute & 0x20)  WS_SetPixel(5, WS_YELLOW);
    if (minute & 0x40)  WS_SetPixel(6, WS_GREEN);
    
    // hour
    if (hour & 1)   WS_SetPixel(11, WS_RED);
    if (hour & 2)   WS_SetPixel(10, WS_YELLOW);
    if (hour & 4)   WS_SetPixel(9, WS_GREEN);
    if (hour & 8)   WS_SetPixel(8, WS_PURPLE);
    
    // 10 hour
    if (hour & 0x10) WS_SetPixel(12, WS_RED);
    if (hour & 0x20) WS_SetPixel(13, WS_YELLOW);

    WS_Show(); 
    delay(100);
}

// Fill the dots one after the other with a color
void colorWipe(WS_Color color) {
    for (uint16_t i = 0; i < 16; i++) {
        WS_SetPixel(i, color);
        WS_Show();
        delay(100);
    }
}

void GPIOConfig() {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4; // 0b00011000 GPIOC
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
    GPIO_Init(GPIOC, &GPIO_InitStructure);     
}

void EXTI2_INT_INIT(void) {
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2); 
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
}

byte bcdToDec(byte value) {
    return ((value / 16) * 10 + value % 16);
}

byte decToBcd(byte value) {
    return (value / 10 * 16 + value % 10);
}

void setPCF8563() {
    // this sets the time and date to the PCF8563
    Wire.beginTransmission(PCF8563address);
    Wire.write(0x02);  
    Wire.write(second);  
    Wire.write(minute);
    Wire.write(hour);     
    Wire.write(dayOfMonth);
    Wire.write(dayOfWeek);  
    Wire.write(month);
    Wire.write(year);
    Wire.endTransmission();
    GPIOC->OUTDR &= ~(1 << 3); // LED power on
}

void readPCF8563() {
    // this gets the time and date from the PCF8563
    Wire.beginTransmission(PCF8563address);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom(PCF8563address, 3);
    second = Wire.read() & 0x7f; // remove VL error bit
    minute = Wire.read() & 0x7f; //
    hour   = Wire.read() & 0x3f; 
    // dayOfMonth = bcdToDec(Wire.read() & B00111111);
    // dayOfWeek  = bcdToDec(Wire.read() & B00000111);  
    // month      = bcdToDec(Wire.read() & B00011111);  // remove century bit, 1999 is over
    // year       = bcdToDec(Wire.read());
}

void wakeUp() {
    // Handler for the pin interrupt.
}

static void send_bit(byte x) { // this setup runs on 24Mhz internal clock, commented out section 48Mhz clock
    if (x) { // Send a 1 bit (high for 0.8us, low for 0.45us, tolerance 150usec)
        GPIOC->OUTDR |= (1 << 4);  // Set high
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); /* __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop");
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); */
        
        GPIOC->OUTDR &= ~(1 << 4);    // Set low 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); /*__asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); */
    } else { // Send a 0 bit (high for 0.4us, low for 0.85us, tolerance 150usec)
        GPIOC->OUTDR |= (1 << 4);   
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); /*__asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop");*/ 
        
        GPIOC->OUTDR &= ~(1 << 4);  // Set low
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); /*__asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); 
        __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); */ 
    }  
    //GPIOC->OUTDR |=(1<<4); GPIOC->OUTDR &=~(1<<4); //Marker for oscilloscope waveform check
}

// Send a byte to WS2812B
static void send_byte(byte b) {
    for (byte x = 0x80; x > 0; x /= 2) 
        send_bit(b & x);
}

// Send array data to LED strip
void WS_Show(void) {
    // a should be 0-32 for 32 brightness levels
    for (byte i = 0; i < 16; i++) {
        // Multiply then shift right 5 bits (divide by 32)
        int scaled_g = (led_buffer[i].g * a) >> 5;
        int scaled_r = (led_buffer[i].r * a) >> 5;
        int scaled_b = (led_buffer[i].b * a) >> 5;
        
        send_byte(scaled_g);
        send_byte(scaled_r);
        send_byte(scaled_b);
    }  
    //GPIOC->OUTDR &=~(1<<4);  // exit with Data pin=Low
    delayMicroseconds(100);  // Reset pulse delay >50us 
}

// Fill all LEDs with the same color
void WS_Fill(WS_Color color) {
    for (byte i = 0; i < 16; i++) {
        led_buffer[i] = color;
    }
}

// Set pixel color 
void WS_SetPixel(byte index, WS_Color color) {
    led_buffer[index] = color;
}

