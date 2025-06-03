#include "pragmas.h"

/* **** Ring-buffers for incoming and outgoing data **** */
// These buffer functions are modularized to handle both the input and
// output buffers with an input argument.
typedef enum {INBUF = 0, OUTBUF = 1} buf_t;

#define BUFSIZE 1024        /* Static buffer size. Maximum amount of data */
uint8_t inbuf[BUFSIZE];   /* Preallocated buffer for incoming data */
uint8_t outbuf[BUFSIZE];  /* Preallocated buffer for outgoing data  */
uint8_t head[2] = {0, 0}; /* head for pushing, tail for popping */
uint8_t tail[2] = {0, 0};


unsigned int carbuf[64], chead, ctail;


uint8_t timer_flag, timer_flag3, counter7, receive_buffer_index, command_arrived, number_of_available_spaces, exit_flag, carbufsize, floor_places[4];
unsigned int timer_counter;
unsigned int total_money, result, display, state;
uint8_t prev;

typedef struct park_place{
    uint8_t available;
    uint8_t reserved;
    uint8_t car_number;
    unsigned int entrance_time;
}park_place;

park_place Parking_Lot[4][10];

uint8_t cars_reserved[1000];

unsigned int global_time;



void init_ports();
void init_interrupts();
void init_timers();
void start_system();
void init_adc();
void init_serial();
void disable_rxtx();
void enable_rxtx();
void receive_isr();
void transmit_isr();
uint8_t buf_isempty( buf_t buf );
void buf_push( uint8_t v, buf_t buf);
uint8_t buf_pop( buf_t buf );
uint8_t carbuf_isempty();
void carbuf_push( unsigned int v);
unsigned int carbuf_pop();
void load_empty_message();
char pop_all_before$();
void clear_parking_lot();
void clear_cars_reserved();
void park(unsigned int car_number);
void unpark(unsigned int car_number);
void subscribe(unsigned int car_number, unsigned int floor, unsigned int no);
void load_parking_space_message(unsigned int x, unsigned int y, unsigned int z);
void load_parking_fee_message(unsigned int x, unsigned int m);
void load_reserved_message(unsigned int x, unsigned int y);
uint8_t converter(unsigned int x);
void error_overflow();
void error_underflow();

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //
__interrupt(high_priority)
void HandleInterrupt()
{
    // ISR ...
    if(INTCONbits.TMR0IF){ // timer interrupt
        INTCONbits.TMR0IF = 0;
        
        TMR0H = 0x3c;
        TMR0L = 0xc3;
        
        global_time += 5;
        
        timer_counter++;
        
        if(timer_counter % 20 == 0){
            timer_flag = 1;
        }
        
        if(timer_counter % 100 == 0){
            timer_flag3 = 1;
            timer_counter = 0;
        }
        
        if(INTCONbits.RBIF){    // RB4 Interrupt
            INTCONbits.RBIF = 0;
            
            uint8_t curr = PORTBbits.RB4;
            
            if(!prev && curr)  state ^= 1;
            
            prev = curr;
        }
        
        if(state){      // This state shows number of empty spaces in the chosen floor
            
            if(result < 256)        display = floor_places[0];
            else if(result < 512)   display = floor_places[1];
            else if(result < 768)   display = floor_places[2];
            else                    display = floor_places[3];
            
            
            switch(counter7++ % 4){
                case 0:
                    LATH = 0b00001000;
                    LATJ = converter(display % 10);
                    break;
                case 1:
                    LATH = 0b00000100;
                    LATJ = converter(display / 10);
                    break;
                case 2:
                    //LATH = 0b00000010;
                    //LATJ = converter((display / 100) % 10);
                    break;
                case 3:
                    //LATH = 0b00000001;
                    //LATJ = converter(display / 1000);
                    break;
            }
        }
        
        else{       //      This state shows total accumulated money
            
            display = total_money;
            
            switch(counter7++ % 4){
                case 0:
                    LATH = 0b00001000;
                    LATJ = converter(display % 10);
                    break;
                case 1:
                    LATH = 0b00000100;
                    LATJ = converter((display / 10)  % 10);
                    break;
                case 2:
                    LATH = 0b00000010;
                    LATJ = converter((display / 100) % 10);
                    break;
                case 3:
                    LATH = 0b00000001;
                    LATJ = converter(display / 1000);
                    break;
            }
        }   
    }
    
    if(PIR1bits.ADIF){  //      ADC interrupt
        result = (ADRESH << 8) + ADRESL; // Get the result;
    }
    
    if (PIR1bits.RC1IF) receive_isr();
    if (PIR1bits.TX1IF) transmit_isr();
}





// ============================ //
//            MAIN              //
// ============================ //
void main()
{   
    // Main ...
    
    timer_flag = timer_flag3 = display = counter7 = receive_buffer_index = command_arrived = exit_flag = chead = ctail = carbufsize = total_money = state = result = prev = 0;
    timer_counter = 0;
    number_of_available_spaces = 40;
    global_time = 0;
    floor_places[0] = floor_places[1] = floor_places[2] = floor_places[3] = 10;
    clear_parking_lot();
    clear_cars_reserved();
    
    init_ports();
    
    init_interrupts();
    
    init_timers();
    
    init_adc();
    
    init_serial();
    
    start_system();
    
    while(1){   // wait until receive GO command
        if(command_arrived){
            command_arrived = 0;
            disable_rxtx();
            char dollar,g,o,hash;
            dollar = pop_all_before$();    // Pop everything before $
            g = buf_pop(INBUF);
            o = buf_pop(INBUF);
            hash = buf_pop(INBUF);
            enable_rxtx();
            if(dollar == '$' && g == 'G' && o == 'O' && hash == '#') break;
        } 
    }
    
    // Set up 7-segment display
    TRISJ = 0;
    TRISHbits.RH0 = 0;
    TRISHbits.RH1 = 0;
    TRISHbits.RH2 = 0;
    TRISHbits.RH3 = 0;
    
    
    while(1){   // Infinite loop during running
        if(timer_flag){ // 100ms
            timer_flag = 0;

            if(buf_isempty(OUTBUF)) load_empty_message();
            
            TXSTA1bits.TXEN = 1;
        }
        
        if(timer_flag3) GODONE = 1;     //  500ms
         
        if(command_arrived){
            command_arrived--;
            
            disable_rxtx();
            
            char c1, c2, c3, hash, x1, x2, x3, y, z1, z2;
            
            pop_all_before$();   // In order to properly receive a command we pop anything that comes before dollar
            
            switch(c1 = buf_pop(INBUF)){
                case 'E':
                   
                    switch(c2 = buf_pop(INBUF)){
                        case 'N':
                            if(((c3 = buf_pop(INBUF)) == 'D') && ((hash = buf_pop(INBUF)) == '#'))  exit_flag = 1;
                            break;
                            
                        case 'X':
                            if(((c3 = buf_pop(INBUF)) == 'T') && 
                                ((x1 = buf_pop(INBUF)) >= '0') && (x1 <= '9') && ((x2 = buf_pop(INBUF)) >= '0') && (x2 <= '9') && ((x3 = buf_pop(INBUF)) >= '0') && (x3 <= '9') && 
                            ((hash = buf_pop(INBUF)) == '#')){
                        
                                enable_rxtx();
                                
                                unsigned int d1, d2, d3, car_no;
                                d1 = x1 - '0';
                                d2 = x2 - '0';
                                d3 = x3 - '0';
                                car_no = 100 * d1 + 10 * d2 + d3;    

                                unpark(car_no);
                            }
                            break;
                            
                        default:
                            break;
                    }
                    break;
                    
                    
                case 'P':
                    if(((c2 = buf_pop(INBUF)) == 'R') && ((c3 = buf_pop(INBUF)) == 'K') && 
                      ((x1 = buf_pop(INBUF)) >= '0') && (x1 <= '9') && ((x2 = buf_pop(INBUF)) >= '0') && (x2 <= '9') && ((x3 = buf_pop(INBUF)) >= '0') && (x3 <= '9') && 
                            ((hash = buf_pop(INBUF)) == '#')){
                        
                        enable_rxtx();
                        
                        unsigned int d1, d2, d3, car_no;
                        d1 = x1 - '0';
                        d2 = x2 - '0';
                        d3 = x3 - '0';
                        car_no = 100 * d1 + 10 * d2 + d3;
                        
                        park(car_no);
                    }
                    break;
                    
                    
                case 'S':
                    if(((c2 = buf_pop(INBUF)) == 'U') && ((c3 = buf_pop(INBUF)) == 'B') && 
                      ((x1 = buf_pop(INBUF)) >= '0') && (x1 <= '9') && ((x2 = buf_pop(INBUF)) >= '0') && (x2 <= '9') && ((x3 = buf_pop(INBUF)) >= '0') && (x3 <= '9') && 
                            ((y = buf_pop(INBUF)) >= 'A') && (y <= 'D') && ((z1 = buf_pop(INBUF)) >= '0') && (z1 <= '9') && ((z2 = buf_pop(INBUF)) >= '0') && (z2 <= '9') &&
                            ((hash = buf_pop(INBUF)) == '#')){
                        
                        enable_rxtx();
                        
                        unsigned int d1, d2, d3, car_no, floor, no;
                        d1 = x1 - '0';
                        d2 = x2 - '0';
                        d3 = x3 - '0';
                        car_no = 100 * d1 + 10 * d2 + d3;
                        floor = y - 'A';
                        no = (z1 - '0') * 10 + (z2 - '0') - 1;
                        
                        subscribe(car_no, floor, no);
                    }
                    break;
                    
                default:
                    break;
            }
            
            enable_rxtx();
           
        }
        
        if(exit_flag) break;   // Finish the process 
    }
    
    while(1){

    }
}


void init_ports(){
    TRISB = 0x10;
    LATB = 0x00;
    
    TRISC = 0b10000000;
    LATC = 0x00;
    
    TRISD = 0x00;
    LATD = 0x00;
    
    //TRISH = 0x00;   // PORTH for output
    LATH = 0x00;
    
    //TRISJ = 0x00;   // PORTJ for output
    LATJ = 0x00;
}

void init_interrupts(){
    INTCON = 0;
    INTCONbits.INT0IE = 1;
    INTCONbits.PEIE = 1;
    
    INTCONbits.RBIE = 1;
}

void init_timers(){
    T0CON = 0;
    INTCONbits.TMR0IE = 1;
    T0CONbits.PSA = 1;
    TMR0H = 0x3c;
    TMR0L = 0xc3;
}

void init_adc(){
  // Set ADC Inputs
  TRISHbits.RH4 = 1; // AN12 input RH4
  // Configure ADC
  ADCON0 = 0x31; // Channel 12; Turn on AD Converter
  ADCON1 = 0x00; // All analog pins
  ADCON2 = 0xAA; // Right Align | 12 Tad | Fosc/32
  ADRESH = 0x00;
  ADRESL = 0x00;
}

void init_serial() {
    // We will configure EUSART1 for 115200 bps, 8N1, no parity
    
    /* configure USART transmitter/receiver */
    TXSTA1 = 0x04;      // (= 00000100) 8-bit transmit, transmitter NOT enabled,TXSTA1.TXEN not enabled!
    // asynchronous, high speed mode
	RCSTA1 = 0x90;      // (= 10010000) 8-bit receiver, receiver enabled,
                        // continuous receive, serial port enabled RCSTA.CREN = 1
    
    BAUDCON1 = 0;
    
    SPBRG1 = 21;        //   for 40 MHz, to have 115200 baud rate, it should be 21
}

void start_system() { 
    INTCONbits.GIE = 1;     // Enable Global Interrupt bit
    T0CONbits.TMR0ON = 1;   // Start timer
    enable_rxtx();
}


// These are used to disable/enable UART interrupts before/after
// buffering functions are called from the main thread. This prevents
// critical race conditions that result in corrupt buffer data and hence
// incorrect processing
void disable_rxtx( void ) { PIE1bits.RC1IE = 0;PIE1bits.TX1IE = 0;}
void enable_rxtx( void )  { PIE1bits.RC1IE = 1;PIE1bits.TX1IE = 1;}

void receive_isr() {
    PIR1bits.RC1IF = 0;      // Acknowledge interrupt
    
    char c = RCREG1;
    
    buf_push(c, INBUF);
    
    if(c == '#') command_arrived++;
    
}
void transmit_isr() {
//    PIR1bits.TX1IF = 0;    // Acknowledge interrupt

    char c = buf_pop(OUTBUF);
    TXREG1 = c;
    // If all bytes are transmitted, turn off transmission
    if(c == '#'){
        // wait until the TSR1 register is empty
        while (TXSTA1bits.TRMT == 0){}
        TXSTA1bits.TXEN = 0;// disable transmission
    }
}

/* Check if a buffer had data or not */
#pragma interrupt_level 2 // Prevents duplication of function
uint8_t buf_isempty( buf_t buf ) { return (head[buf] == tail[buf])?1:0; }
/* Place new data in buffer */
//#pragma interrupt_level 2 // Prevents duplication of function
void buf_push( uint8_t v, buf_t buf) {
    if (buf == INBUF) inbuf[head[buf]] = v;
    else outbuf[head[buf]] = v;
    head[buf]++;
    if (head[buf] == BUFSIZE) head[buf] = 0;
    if (head[buf] == tail[buf]) { error_overflow(); }
}
/* Retrieve data from buffer */
#pragma interrupt_level 2 // Prevents duplication of function
uint8_t buf_pop( buf_t buf ) {
    uint8_t v;
    if (buf_isempty(buf)) { 
        error_underflow(); return 0; 
    } else {
        if (buf == INBUF) v = inbuf[tail[buf]];
        else v = outbuf[tail[buf]];
        tail[buf]++;
        if (tail[buf] == BUFSIZE) tail[buf] = 0;
        return v;
    }
}

uint8_t carbuf_isempty() { return (chead == ctail)?1:0; }

void carbuf_push( unsigned int v){
    carbuf[chead] = v;
    chead++;
    if(chead == 64) chead = 0;
}

unsigned int carbuf_pop(){
    unsigned int v;
    if(carbufsize){    
        v = carbuf[ctail];
        ctail++;
        if(ctail == 64) ctail = 0;
        return v;
    }
    else return 0;
}

void load_empty_message(){      // $EMPCC#
    char c1,c2;
    c1 = '0' + number_of_available_spaces / 10;
    c2 = '0' + number_of_available_spaces % 10;
    
    disable_rxtx();
    buf_push('$', OUTBUF);
    buf_push('E', OUTBUF);
    buf_push('M', OUTBUF);
    buf_push('P', OUTBUF);
    buf_push(c1, OUTBUF);
    buf_push(c2, OUTBUF);
    buf_push('#', OUTBUF);
    enable_rxtx();
}

char pop_all_before$(){
    char c = buf_pop(INBUF);
    while(c != '$') c = buf_pop(INBUF);
    return c;
}

void clear_parking_lot(){
    int i,j;
    for(i=0; i<4; i++){
        for(j=0; j<10; j++){
            Parking_Lot[i][j].available = 1;
            Parking_Lot[i][j].reserved = 0;
            Parking_Lot[i][j].entrance_time = 0;
        }
    }
}

void clear_cars_reserved(){
    int i;
    for(i=0; i<1000; i++) cars_reserved[i] = 0;
}


void subscribe(unsigned int car_number, unsigned int floor, unsigned int no){
    if(Parking_Lot[floor][no].reserved == 0 && Parking_Lot[floor][no].available == 1){
        Parking_Lot[floor][no].reserved = 1;
        Parking_Lot[floor][no].car_number = car_number;
        cars_reserved[car_number] = 1;
        total_money += 50;
        load_reserved_message(car_number, 50);
    }
    else{
        load_reserved_message(car_number, 0);
    }
}

void unpark(unsigned int car_number){
    int i,j;
    for(i=0; i<4; i++){
        for(j=0; j<10; j++){
            if(Parking_Lot[i][j].car_number == car_number){
                
                if(Parking_Lot[i][j].reserved){ // Exit subscribed car
                    
                    load_parking_fee_message(car_number, 0);
                    
                    Parking_Lot[i][j].available = 1;
                
                    number_of_available_spaces++;
                
                    floor_places[i]++; 
                    
                    return;
                }
                
                // Otherwise exit normal car and calculate parking fee
                
                unsigned int fee = (1 + (global_time - Parking_Lot[i][j].entrance_time) / 250);
                
                total_money += fee;

                // Send Parking Fee Message
                load_parking_fee_message(car_number, fee);
                // If at least 1 car in queue send SPC message
                if(!carbuf_isempty()){
                    unsigned int car_no = carbuf_pop();
                    carbufsize--;
                    
                    
                    Parking_Lot[i][j].car_number = car_no;
                    Parking_Lot[i][j].entrance_time = global_time;
                    load_parking_space_message(car_no, i, j+1);
                    return;
                }
                
                Parking_Lot[i][j].available = 1;
                
                number_of_available_spaces++;
                
                floor_places[i]++; 
                
                return;
            }
        }
    }
}

void park(unsigned int car_number){
    int i,j;
    
    if(cars_reserved[car_number]){  // Park subscribed car
        for(i=0; i<4; i++){
            for(j=0; j<10; j++){
                if(Parking_Lot[i][j].car_number == car_number){
                    Parking_Lot[i][j].available = 0;


                    // Send Parking Space Message
                    load_parking_space_message(car_number, i, j+1);

                    number_of_available_spaces--;
                    
                    floor_places[i]--; 

                    return;
                }
            }
        }
    }
    
    // Park normal car
    for(i=0; i<4; i++){ 
        for(j=0; j<10; j++){
            if(Parking_Lot[i][j].available == 1 && Parking_Lot[i][j].reserved == 0){
                Parking_Lot[i][j].available = 0;
                Parking_Lot[i][j].car_number = car_number;
                Parking_Lot[i][j].entrance_time = global_time;
                
                // Send Parking Space Message
                load_parking_space_message(car_number, i, j+1);
                
                number_of_available_spaces--;
                
                floor_places[i]--; 
                
                return;
            }
        }
    }
    
    // If no place, put car into buffer
    if(carbufsize <= 16){
        carbuf_push(car_number);
        carbufsize++;
    }
}


void load_parking_space_message(unsigned int x, unsigned int y, unsigned int z){    // $SPCXXXYZZ#
    char x1, x2, x3, y1, z1, z2;
    
    x1 = '0' + x / 100;
    x2 = '0' + (x % 100) / 10;
    x3 = '0' + x % 10;
    y1 = 'A' + y;
    z1 = '0' + z / 10;
    z2 = '0' + z % 10;
    
    disable_rxtx();
    buf_push('$', OUTBUF);
    buf_push('S', OUTBUF);
    buf_push('P', OUTBUF);
    buf_push('C', OUTBUF);
    buf_push(x1, OUTBUF);
    buf_push(x2, OUTBUF);
    buf_push(x3, OUTBUF);
    buf_push(y1, OUTBUF);
    buf_push(z1, OUTBUF);
    buf_push(z2, OUTBUF);
    buf_push('#', OUTBUF);
    enable_rxtx();
}

void load_parking_fee_message(unsigned int x, unsigned int m){  //  $FEEXXXMMM#
    char x1, x2, x3, m1, m2, m3;
    
    x1 = '0' + x / 100;
    x2 = '0' + (x % 100) / 10;
    x3 = '0' + x % 10;
    
    m1 = '0' + m / 100;
    m2 = '0' + (m % 100) / 10;
    m3 = '0' + m % 10;
    
    disable_rxtx();
    buf_push('$', OUTBUF);
    buf_push('F', OUTBUF);
    buf_push('E', OUTBUF);
    buf_push('E', OUTBUF);
    buf_push(x1, OUTBUF);
    buf_push(x2, OUTBUF);
    buf_push(x3, OUTBUF);
    buf_push(m1, OUTBUF);
    buf_push(m2, OUTBUF);
    buf_push(m3, OUTBUF);
    buf_push('#', OUTBUF);
    enable_rxtx();
}

void load_reserved_message(unsigned int x, unsigned int m){ //  $RESXXXMM#
    char x1, x2, x3, m1, m2;
    
    x1 = '0' + x / 100;
    x2 = '0' + (x % 100) / 10;
    x3 = '0' + x % 10;
    
    m1 = '0' + m / 10;
    m2 = '0' + m % 10;
    
    disable_rxtx();
    buf_push('$', OUTBUF);
    buf_push('R', OUTBUF);
    buf_push('E', OUTBUF);
    buf_push('S', OUTBUF);
    buf_push(x1, OUTBUF);
    buf_push(x2, OUTBUF);
    buf_push(x3, OUTBUF);
    buf_push(m1, OUTBUF);
    buf_push(m2, OUTBUF);
    buf_push('#', OUTBUF);
    enable_rxtx();
}

uint8_t converter(unsigned int x){           // Convert a number to the 7-segment display format
    switch(x){
        case 0:
            return 0b00111111;
        case 1:
            return 0b00000110;
        case 2:
            return 0b01011011;
        case 3:
            return 0b01001111;
        case 4:
            return 0b01100110;
        case 5:
            return 0b01101101;
        case 6:
            return 0b01111101;
        case 7:
            return 0b00000111;
        case 8:
            return 0b01111111;
        case 9:
            return 0b01101111;
            
        default:
            break;
    }
}

/* **** Error outputs and functions **** */
char * err_str = 0;
// Prevents the compiler from duplicating the subsequently function
// which is done when the function is called from both the main and ISR
// contexts
#pragma interrupt_level 2 
void error_overflow()  { PORTD |= 0x01; err_str = "IO buffer overflow!";}
#pragma interrupt_level 2 // Prevents duplication of function
void error_underflow() { PORTD |= 0x02; err_str = "IO buffer underflow!";}
//void error_packet()    { PORTB |= 0x04; err_str = "Packet format error!";}
//void error_syntax()    { PORTB |= 0x08; err_str = "Syntax error!";}
//void error_stack()     { PORTB |= 0x10; err_str = "Stack error!";}
//void error_variable()  { PORTB |= 0x20; err_str = "Variable error!";}
//void error_clear()     { /*PORTB &= 0xC0*/; err_str = 0; }

