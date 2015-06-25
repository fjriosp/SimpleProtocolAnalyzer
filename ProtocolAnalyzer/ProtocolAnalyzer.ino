// Data
const uint16_t MAXCAP=2048;
uint8_t data[MAXCAP];
// Clock Divider
uint8_t clk_div = 8;
// Signal mask
uint8_t mask = 0x03;

void setup() {
  // Paint memory
  mem_paint();
  
  // Disable ADC
  ADCSRA &= ~(_BV(ADEN));
  // Disable Analog Comparator
  ADCSRB &= ~(_BV(ACME));

  // Set PF[7-4] as input without pullups
  DDRF  &= 0x0F;
  //PORTF |= 0xF0;
  PORTF &= ~(0xF0);
  
  // Initialize data
  for(int i=0; i<MAXCAP; i++)
    data[i]=0;
  
  // Open serial port
  Serial.begin(9600);
}

void loop() {
  menu_loop();
}

//#############
//# Mem Trace #
//#############
extern uint8_t __data_start;
extern uint8_t __data_end;
extern uint8_t __bss_start;
extern uint8_t __bss_end;
extern uint8_t __heap_start;
extern uint8_t *__brkval;

const uint8_t STACK_CANARY=0xAA;

inline void mem_paint(void) {
  uint8_t *p = __brkval==0 ? &__heap_start : __brkval;
  
  while((uint16_t)p <= SP)
  {
      *p = STACK_CANARY;
      p++;
  }
}

uint16_t mem_unused() {
  uint8_t *p = __brkval==0 ? &__heap_start : __brkval;
  
  uint16_t c = 0;
  uint16_t maxc = 0;
  // Search max contiguous bytes with value STACK_CANARY
  while(p <= (uint8_t*)SP) {
    if(*p == STACK_CANARY) {
      c++;
    } else if(c > maxc) {
      maxc = c;
    }
    p++;
  }
  
  return maxc;
}

//########
//# Menu #
//########
inline void menu_loop() {
  while(Serial.available()) {
    char cmd = Serial.read();
    Serial.println(cmd);

    switch(cmd) {
      case 'h':
        menu_help();
        break;
      case 'C':
        menu_config();
        break;
      case 'm':
        menu_mem();
        break;
      case 'c':
        menu_capture();
        break;
      case 's':
        menu_show();
        break;
      case 'e':
        menu_export();
        break;
      default:
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        break;
    }
  }
}

inline void menu_help() {
  Serial.println();
  Serial.println(F("SimpleProtocolAnalyzer 1.0"));
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F(" h Show this help"));
  Serial.println(F(" C Configure"));
  Serial.println(F(" m Show mem stats"));
  Serial.println(F(" c Start capture data"));
  Serial.println(F(" s Show captured data"));
  Serial.println(F(" e Export captured data"));
  Serial.println();
}

inline void menu_config() {
  char c;
  do {
    Serial.println();
    Serial.println(F("Config Mode"));
    Serial.println();
    Serial.println(F("Parameters:"));
    Serial.print(F(" r rate ["));
    Serial.print(F_CPU/clk_div);
    Serial.println(F("]"));
    Serial.print(F(" m mask ["));
    Serial.print((char)((mask & _BV(3)) ? 'x' : '-'));
    Serial.print((char)((mask & _BV(2)) ? 'x' : '-'));
    Serial.print((char)((mask & _BV(1)) ? 'x' : '-'));
    Serial.print((char)((mask & _BV(0)) ? 'x' : '-'));
    Serial.println(F("]"));
    Serial.println(F(" q exit ConfigMode"));
    Serial.println();
    Serial.print(F("Option: "));

    c = Serial.read();
    Serial.println(c);

    switch(c) {
      case 'r':
        menu_config_rate();
      break;
      case 'm':
        menu_config_mask();
      break;
      case 'q': // Will exit
      break;
      default:
        Serial.print(F("Unknown option: "));
        Serial.println(c);
      break;
    }
  } while(c!='q');
}

inline void menu_config_rate() {
  char c;
  Serial.println();
  Serial.println(F("Config Rate"));
  Serial.println();
  Serial.println(F("Available rates:"));
  Serial.print(F(" 0 F_CPU/8  ["));
  Serial.print(F_CPU/8);
  Serial.println(F("]"));
  Serial.print(F(" 1 F_CPU/16 ["));
  Serial.print(F_CPU/16);
  Serial.println(F("]"));
  Serial.print(F(" 2 F_CPU/32 ["));
  Serial.print(F_CPU/32);
  Serial.println(F("]"));
  Serial.print(F(" 3 F_CPU/64 ["));
  Serial.print(F_CPU/64);
  Serial.println(F("]"));
  Serial.println();
  Serial.print(F("Rate: "));

  c = Serial.read();
  Serial.println(c);

  switch(c) {
    case '0':
      clk_div = 8;
    break;
    case '1':
      clk_div = 16;
    break;
    case '2':
      clk_div = 32;
    break;
    case '3':
      clk_div = 64;
    break;
    default:
      Serial.print(F("Unknown Rate: "));
      Serial.println(c);
      return;
    break;
  }
}

inline void menu_config_mask() {
  char c;
  Serial.println();
  Serial.println(F("Config mask"));
  Serial.println();
  Serial.println(F("Channels    :  3210"));
  Serial.print(  F("Current mask: ["));
  Serial.print((char)((mask & _BV(3)) ? 'x' : '-'));
  Serial.print((char)((mask & _BV(2)) ? 'x' : '-'));
  Serial.print((char)((mask & _BV(1)) ? 'x' : '-'));
  Serial.print((char)((mask & _BV(0)) ? 'x' : '-'));
  Serial.println(F("]"));
  Serial.println();
  Serial.print(F("Toggle channel: "));

  c = Serial.read();
  Serial.println(c);

  switch(c) {
    case '0':
    case '1':
    case '2':
    case '3':
      mask ^= _BV(c-'0');
    break;
    default:
      Serial.print(F("Unknown Channel: "));
      Serial.println(c);
      return;
    break;
  }
}

inline void menu_mem() {
  uint16_t total_size = RAMEND-((uint16_t)&__data_start)+1;
  uint16_t data_size  = ((uint16_t)&__data_end)-((uint16_t)&__data_start);
  uint16_t bss_size   = ((uint16_t)&__bss_end)-((uint16_t)&__bss_start);
  uint16_t heap_size  = ((uint16_t)__brkval) == 0 ? 0 : (((uint16_t)__brkval)-((uint16_t)&__heap_start));
  uint16_t stack_size = RAMEND-SP;
  uint16_t total_used = data_size+bss_size+heap_size+stack_size;
  Serial.print(F("Total:  "));
  Serial.println(total_size);
  Serial.print(F("Data:   "));
  Serial.println(data_size);
  Serial.print(F("BSS:    "));
  Serial.println(bss_size);
  Serial.print(F("Heap:   "));
  Serial.println(heap_size);
  Serial.print(F("Stack:  "));
  Serial.println(stack_size);
  Serial.print(F("Free:   "));
  Serial.println(RAMEND-total_used);
  Serial.print(F("Unused: "));
  Serial.println(mem_unused());
  Serial.println();
}

unsigned long capture_8() {
  uint16_t cnt = MAXCAP - 1;
  uint8_t  *p  = data+1;
  unsigned long start = micros();
  asm volatile(
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    "nop                  \n\t" // CK+1 = 4
    "rjmp .+0             \n\t" // CK+2 = 6
    "rjmp .+0             \n\t" // CK+2 = 8
  
    "cloop8:               \n\t"
    
    "in   r17, %[PF]      \n\t" // CK+1 = 1
    "andi r17, 0xF0       \n\t" // CK+1 = 2
    "or   r17, r16        \n\t" // CK+1 = 3
    "st   x+,  r17        \n\t" // CK+2 = 5
    "nop                  \n\t" // CK+1 = 6
    "rjmp .+0             \n\t" // CK+2 = 8
    
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    "nop                  \n\t" // CK+1 = 4
    
    "sbiw %[CNT], 1       \n\t" // CK+2 = 6
    "brne cloop8           \n\t" // CK+2 = 8
    : 
    : [PF]   "I"  (_SFR_IO_ADDR (PINF)),
      [CNT]  "w"  (cnt),
      [DATA] "x"  (p)
    : "r16","r17"
  );
  unsigned long end = micros();
  return end-start;
}

unsigned long capture_16() {
  uint16_t cnt = MAXCAP - 1;
  uint8_t  *p  = data+1;
  unsigned long start = micros();
  asm volatile(
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    
    // 3*4 = 12 CK
    "ldi  r18, 4          \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+12 = 15
    "nop                  \n\t" // CK+1  = 16
  
    "cloop16:             \n\t"
    
    "in   r17, %[PF]      \n\t" // CK+1 = 1
    "andi r17, 0xF0       \n\t" // CK+1 = 2
    "or   r17, r16        \n\t" // CK+1 = 3
    "st   x+,  r17        \n\t" // CK+2 = 5
    
    // 3*3 = 9 CK
    "ldi  r18, 3          \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+9 = 14
    "rjmp .+0             \n\t" // CK+2 = 16
    
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    
    // 3*3 = 9 CK
    "ldi  r18, 3          \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+9 = 12
    
    "sbiw %[CNT], 1       \n\t" // CK+2 = 14
    "brne cloop16         \n\t" // CK+2 = 16
    : 
    : [PF]   "I"  (_SFR_IO_ADDR (PINF)),
      [CNT]  "w"  (cnt),
      [DATA] "x"  (p)
    : "r16","r17","r18"
  );
  unsigned long end = micros();
  return end-start;
}

unsigned long capture_32() {
  uint16_t cnt = MAXCAP - 1;
  uint8_t  *p  = data+1;
  unsigned long start = micros();
  asm volatile(
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    
    // 3*9 = 27 CK
    "ldi  r18, 9          \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+27 = 30
    "rjmp .+0             \n\t" // CK+2  = 32
    
    "cloop32:             \n\t"
    
    "in   r17, %[PF]      \n\t" // CK+1 = 1
    "andi r17, 0xF0       \n\t" // CK+1 = 2
    "or   r17, r16        \n\t" // CK+1 = 3
    "st   x+,  r17        \n\t" // CK+2 = 5
    
    // 3*9 = 27 CK
    "ldi  r18, 9          \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+27 = 32
    
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    
    // 3*8 = 24 CK
    "ldi  r18, 8          \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+24 = 27
    "nop                  \n\t" // CK+1  = 28
    
    "sbiw %[CNT], 1       \n\t" // CK+2 = 30
    "brne cloop32         \n\t" // CK+2 = 32
    : 
    : [PF]   "I"  (_SFR_IO_ADDR (PINF)),
      [CNT]  "w"  (cnt),
      [DATA] "x"  (p)
    : "r16","r17","r18"
  );
  unsigned long end = micros();
  return end-start;
}

unsigned long capture_64() {
  uint16_t cnt = MAXCAP - 1;
  uint8_t  *p  = data+1;
  unsigned long start = micros();
  asm volatile(
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    
    // 3*20 = 60 CK
    "ldi  r18, 20         \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+60 = 63
    "nop                  \n\t" // CK+1  = 64
    
    "cloop64:             \n\t"
    
    "in   r17, %[PF]      \n\t" // CK+1 = 1
    "andi r17, 0xF0       \n\t" // CK+1 = 2
    "or   r17, r16        \n\t" // CK+1 = 3
    "st   x+,  r17        \n\t" // CK+2 = 5
    
    // 3*19 = 57 CK
    "ldi  r18, 19         \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+57 = 62
    "rjmp .+0             \n\t" // CK+2  = 64
    
    "in   r16, %[PF]      \n\t" // CK+1 = 1
    "andi r16, 0xF0       \n\t" // CK+1 = 2
    "swap r16             \n\t" // CK+1 = 3
    
    // 3*19 = 57 CK
    "ldi  r18, 19         \n\t"
    "dec  r18             \n\t"
    "brne .-4             \n\t" // CK+57 = 60
    
    "sbiw %[CNT], 1       \n\t" // CK+2 = 62
    "brne cloop64         \n\t" // CK+2 = 64
    : 
    : [PF]   "I"  (_SFR_IO_ADDR (PINF)),
      [CNT]  "w"  (cnt),
      [DATA] "x"  (p)
    : "r16","r17","r18"
  );
  unsigned long end = micros();
  return end-start;
}

inline void menu_capture() {
  data[0]  = PINF & 0xF0;
  data[0] |= data[0] >> 4;

  // Wait some change
  while((data[0] & mask) == ((PINF>>4) & mask));

  unsigned long time;
  cli();
  switch(clk_div) {
    case 8:
    	time=capture_8();  // 2MHz
        break;
    case 16:
    	time=capture_16(); // 1MHz
        break;
    case 32:
    	time=capture_32(); // 500kHz
        break;
    case 64:
    	time=capture_64(); // 250kHz
        break;
  }
  sei();
  
  unsigned long rate = (MAXCAP * 1000000) / time;
  
  Serial.print(F("Buffer filled in: "));
  Serial.print(time);
  Serial.println(F(" us"));
  Serial.print(F("Freq: "));
  Serial.print(F_CPU/clk_div);
  Serial.print(F("Measured Freq: "));
  Serial.print(rate);
  Serial.println(F(" Hz"));
}

const uint16_t LINESIZE = 64;
const uint16_t NLINES = 2;
const char GRAPH[] = {'.','+'};

inline void menu_show() {
  int i=0;
  while(i<MAXCAP) {
    // Signal0
    for(uint8_t s=0;s<NLINES;s++) {
      for(int j=0;j<LINESIZE;j++) {
        Serial.print(GRAPH[(data[i+j]&(0x01 << s)) == 0 ? 0 : 1]);
        Serial.print(GRAPH[(data[i+j]&(0x10 << s)) == 0 ? 0 : 1]);
      }
      Serial.println();
    }
    i += LINESIZE;
    Serial.println();
  }
}

inline void menu_export() {
  int s=1;
  int i=0;
  int c=0;
  uint8_t last = (data[0] & mask);
  while(i<MAXCAP) {
    if(last != (data[i] & mask)) {
      last = (data[i] & mask);
      s++;
    }
    c++;
    if(last != ((data[i]>>4) & mask)) {
      last = ((data[i]>>4) & mask);
      s++;
    }
    c++;
    i++;
  }
  
  Serial.print(F(";Size: "));
  Serial.println(s);
  Serial.print(F(";Rate: "));
  Serial.println(F_CPU/clk_div);
  Serial.println(F(";Channels: 2"));
  Serial.println(F(";EnabledChannels: 15"));
  Serial.println(F(";TriggerPosition: 0"));
  Serial.println(F(";Compressed: true"));
  Serial.print(F(";AbsoluteLength: "));
  Serial.println(c);
  Serial.println(F(";CursorEnabled: true"));
  
  i=c=0;
  last = (data[0] & mask);
  Serial.print(F("0000000"));
  Serial.print(last);
  Serial.print(F("@"));
  Serial.println(i);
  
  while(i<MAXCAP) {
    if(last != (data[i] & mask)) {
      last = (data[i] & mask);
      Serial.print(F("0000000"));
      Serial.print(last);
      Serial.print(F("@"));
      Serial.println(c);
    }
    c++;
    if(last != ((data[i]>>4) & mask)) {
      last = ((data[i]>>4) & mask);
      Serial.print(F("0000000"));
      Serial.print(last);
      Serial.print(F("@"));
      Serial.println(c);
    }
    c++;
    i++;
  }
}
