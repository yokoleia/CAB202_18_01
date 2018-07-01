/*

RAB DAWANINCURA
n9730877
TEENSY ASSIGNMENT

*/

//--------------------------------------------------------------------------
//CAB202 Libraries--------------------
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "lcd.h"
#include "graphics.h"
#include "cpu_speed.h"
#include "sprite.h"
#include "macros.h"

/* Volatiles -------------------------------------------
------------------------------------------------------*/
volatile unsigned long ovf_count = 0;

/* Sprites ------------------------------------------
---------------------------------------------------*/
Sprite car;
Sprite trees[10];
Sprite roadblocks[3];
Sprite petrol;

/* CONSTANTS -----------------------------------------
-----------------------------------------------------*/
#define FREQUENCY 8000000.0
#define PRESCALER_0 1024.0
#define PRESCALER_1 1024.0
#define H_SHIP 4
#define W_SHIP 4
// CONSTANTS


/* GLOBAL VARIABLES-------------------------------
-------------------------------------------------*/
// hardware tracking
int button_state[2] = {0,0};
int JS_Val;

int GAME_STATE = 0;
int LIVES = 5;
double speed = 0;

// game timing
int EL_TIME = 0;
int EL_MIN = 0;
int EL_SEC = 0;
double game_start_time;
double pause_start = 0;
bool reached_destination = false;



// Gameplay functionality
int health;
double distance = 0;



/*BITMAPS --------------------------------------
------------------------------------------------*/
unsigned char car_image[9] = {
    0b01111110,
    0b11000011,
    0b11111111,
    0b01000010,
    0b11000011,
    0b11000011,
    0b01111110,
    0b00100100,
    0b01111110
};

unsigned char tree_image[5] = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b01111110,
    0b00111100
};

unsigned char roadblock_image[5] = {
    0b11111111,
    0b10000001,
    0b11111111,
    0b11000011,
    0b11000011
};

unsigned char petrol_image[9] = {
    0b11111111,
    0b11111111,
    0b11111111,
    0b11100111,
    0b11100111,
    0b11100111,
    0b11111111,
    0b11111111,
    0b11111111
};


/* FUNCTIONS --------------------------------------------------
--------------------------------------------------------------*/
// SYSTEM TIME FUNCTION ----------------------
double get_system_time(void) {
    return (ovf_count * 65536 + TCNT1) * PRESCALER_1 / FREQUENCY;
}


int screen_height() {
    return (int) LCD_Y;
}

int screen_width() {
    return (int) LCD_X;
}
// HARDWARE INITIALISING FUNCTION---------

void init_hardware(void) {
    // Initialising the LCD screen
    lcd_init(LCD_DEFAULT_CONTRAST);

    // Initalising the buttons as inputs
    DDRF &= ~(1 << 5);
  	DDRF &= ~(1 << 6);


    // init thumbwheels
    DDRF &= ~(1 << 0);
    DDRF &= ~(1 << 0);

    // initialising the joystick
    DDRD &= ~(1 << 1);
  	DDRB &= ~(1 << 7);
  	DDRB &= ~(1 << 1);
  	DDRD &= ~(1 << 0);
  	DDRB &= ~(1 << 0);

    // Setup two timers with overflow interrupts:
    // 0 - button press checking @ ~30Hz
    // 1 - system time overflowing every 8.3s
    TCCR0B |= ((1 << CS02) | (1 << CS00));
    TCCR1B |= ((1 << CS12) | (1 << CS10));
    TIMSK0 |= (1 << TOIE0);
    TIMSK1 |= (1 << TOIE1);

    sei();
}

void direct_draw_health(void) {
    int x=screen_width()-1;
    for (int i=0; i<7; i++) {
        draw_pixel(x,i,FG_COLOUR);
        draw_pixel(x-1,i,FG_COLOUR);
    }
}

bool check_collision(Sprite s1, Sprite s2) {
    int hx = s1.x;
	int hy = s2.x;
	int hr = hx + (s1.width) - 1;
	int hb = hy + (s1.height) - 1;
	int zx = (s2.x);
	int zy = (s2.y);
	int zr = zx + (s2.width) - 1;
	int zb = zy + (s2.height) - 1;
	if ( hr < zx ) return false;
	if ( zr < hx ) return false;
	if ( hb < zy ) return false;
	if ( zb < hy ) return false;
	return true;
}

// THUMBWHEEL ACTIVE  ---------------------------

// BUTTONS PRESSED ---------------------------
void buttons_pressed(void) {
    /*Button Pressed checks the button state array and alters it accordingly*/
    // LEFT BUTTON
    if ((PINF  >> 6) & 0b1 ){ //Check if pin5 is active (left button)
        if (button_state[0]==0) {
            button_state[0] = 1;
        } else {
            button_state[0] = 0;
        }
    }
    // RIGHT BUTTON
    if ((PINF  >> 5) & 0b1 ){ //Check if pin5 is active (right button)
        if (button_state[1]==0) {
            button_state[1] = 1;
        } else {
            button_state[1] = 0;
        }
    }

    //debounce
    _delay_ms(100);
}

void JS_Dir(void) {
    if(PIND >> 0 & 1){
        JS_Val=1;
    } else if(PIND >> 1 & 1){
    		JS_Val=2;
    }	else if(PINB >> 1 & 1){
        JS_Val=3;
  	} else if(PINB >> 7 & 1){
    		JS_Val=4;
  	} else {
        JS_Val=0;
    }
}


// intro menu function
void intro_menu(void) {
    // Draw the static information to the screen screen=(84x48) 17 leters across
    draw_string(0, 0, "   ZOMBIE RACE   ",FG_COLOUR);
    draw_string(0, 8, " Rab Dawanincura ",FG_COLOUR);
    draw_string(0, 16, "    n9730877     ",FG_COLOUR);
    draw_string(0, 24, "  Press A Button ",FG_COLOUR);
    draw_string(0, 32, "   To Continue!  ",FG_COLOUR);

    if (button_state[0]==1 || button_state[1]==1){
        GAME_STATE=1;
    }

}

// same as intro menu, displayed at the end of the game
void game_over(void) {
    draw_string(0, 8, "      YOU WON   ",FG_COLOUR);
    draw_string(2, 24, "Press Any Button",FG_COLOUR);
    draw_string(2, 32, " To Play Again! ",FG_COLOUR);
    if (button_state[0]==1 || button_state[1]==1){
        GAME_STATE=0;
    }
}

void pause(void) {
    draw_string(0, 12,  "      PAUSE     ",FG_COLOUR);
    draw_string(2, 24, " Press A Button ",FG_COLOUR);
    draw_string(2, 32, "  To Continue!  ",FG_COLOUR);

    char buffer [50];
    snprintf ( buffer, 50, "%0.1f m %0.02f s", distance, (get_system_time()-pause_start));
    draw_string(8, 0, buffer, FG_COLOUR);



    if (button_state[0]==1 || button_state[1]==1){
        GAME_STATE=1;
    }
}


/* game_state_overflow ------------------------------
For when the game_state gets to a value that isn't part of the
gamestate dict */
void game_state_overflow(void) {
  draw_string(0, 8, "      ERROR       ",FG_COLOUR);
  draw_string(0, 16, "GAMESTATEOVERFLOW",FG_COLOUR);
  draw_string(0, 24, " Press A Button ",FG_COLOUR);
  draw_string(0, 32, " To Play Again! ",FG_COLOUR);
  if (button_state[0]==1 || button_state[1]==1){
      GAME_STATE=0;
  }
}

void border(void) {
    draw_line(0, 8, 83, 8,FG_COLOUR);//Top Border
    draw_line(0, 47, 83, 47,FG_COLOUR);//Bottom border
    draw_line(0, 8, 0, 47,FG_COLOUR);//left border
    draw_line(83, 8, 83, 47,FG_COLOUR);//right border
}


// countdown function run before start of game
void countdown(void) {
    int count = 3;
    draw_string(0, 8, "     GET READY   ",FG_COLOUR);
    char buff[80];
    while (count!=0) {
        sprintf(buff, "%i", count);
        draw_string(42, 20, buff,FG_COLOUR);
        show_screen();
        _delay_ms(1000);
        count--;
    }
    //sprite_create();
    game_start_time=get_system_time();
    GAME_STATE=2;
    EL_MIN=0;
}

//reseting status
void status_reset(void) {
    LIVES = 5;
    speed = 0;
    EL_TIME = 0;
    distance = 0;
}

// Drawing status bar
void status_bar(void) {
    for (int i=0; i<2; i++) {
        draw_string(0,i,"                                                                                                                                                                ",FG_COLOUR);
    }
    EL_TIME = (int)(get_system_time()-game_start_time);
    EL_MIN = (int)((EL_TIME/60)+0.5);
    EL_SEC = (int)(EL_TIME-(60*EL_MIN));
    draw_string(0,0,"D:",FG_COLOUR);
    draw_string(20,0,"L:",FG_COLOUR);
    draw_string(35, 0, "T:  :",FG_COLOUR);
    char buff[80];
    sprintf(buff, "%i", (int)distance);
    draw_string(9,0,buff,FG_COLOUR);

    sprintf(buff, "%i", LIVES);
    draw_string(29,0,buff,FG_COLOUR);

    int min_1st_digit = (int)EL_MIN/10;
    sprintf(buff, "%i", min_1st_digit);
    draw_string(45,0,buff,FG_COLOUR);


    sprintf(buff, "%i", EL_MIN-(min_1st_digit*10));
    draw_string(50,0,buff,FG_COLOUR);

    int sec_1st_digit = (int)EL_SEC/10;
    sprintf(buff, "%i", sec_1st_digit);
    draw_string(60,0,buff,FG_COLOUR);

    sprintf(buff, "%i", EL_SEC-(sec_1st_digit*10));
    draw_string(65,0,buff,FG_COLOUR);

    direct_draw_health();

}

//SPRITE RELATED FUNCTIONS
void create_sprites(void) {
    sprite_init(&car, screen_width()/2-12, screen_height()-11, 8,9, car_image);

    //trees
    for (int i=0; i<5; i++) {
        int randx = rand() % (screen_width()/2)-24-7;
        int randy = (screen_height()/5)*(i+1);
        sprite_init(&trees[i],randx, randy, 8, 5, tree_image);
    }

    for (int i=5; i<10; i++) {
        int randx = rand() % (screen_width()/2) + (screen_width()/2);
        int randy = (screen_height()/5)*(i-9);
        sprite_init(&trees[i],randx, randy, 8, 5, tree_image);
    }

    // roadblocks
    for (int i=0; i<3; i++) {
        int randx = rand() % (screen_width()/2) + (screen_width()/2);
        int randy = (screen_height()/5)*(i-9);
        sprite_init(&roadblocks[i],randx, randy, 8,5,roadblock_image);
    }
    roadblocks[0].x = screen_width()/2;
    roadblocks[1].y = 9;
}

void draw_sprites(void) {
    sprite_draw(&car);
    for (int i=0; i<10; i++) {
        sprite_draw(&trees[i]);
    }
    for (int i=0; i<3; i++) {
        sprite_draw(&roadblocks[i]);
    }
}


// road
void draw_road(void) {
    for (int i=0; i<LCD_Y; i++) {
        int x = 20*pow(sin(0.02*(distance-i)),3);
        draw_pixel(x+30,i,FG_COLOUR);
        draw_pixel(x+50,i,FG_COLOUR);
    }
}





// checks the inputs each screen refresh
void inputs(void) {
    // clear button press buffer
    button_state[0] = 0;
    button_state[1] = 0;
    //checking if buttons have been pressed since clearing button press buffer
    buttons_pressed();

    // clear JS input buffer
    //JS_Val=0;
    //check if joystick moved
    JS_Dir();
}

bool on_road(int i) {
    int rl = 20*pow(sin(0.02*(distance-i)),3)+30;
    int rr = 20*pow(sin(0.02*(distance-i)),3)+50;
    if (i > rl && i < rr) {
        return true;
    }
    return false;
}


void move_sprites(void) {
    for (int i=0; i<10; i++) {
        trees[i].y+=speed/2;

        if (trees[i].y > screen_height()) {
            while (on_road((int)trees[i].x)) {
                trees[i].y = rand() % 60;
            }
            trees[i].y = 0;


        }
    }
    for (int i=0; i<3; i++) {
        roadblocks[i].y += speed/2;
        if (roadblocks[i].y > screen_height()) {
            roadblocks[i].y=0;
        }
    }
}







void game(void) {
    if (button_state[0]==1){
        pause_start = get_system_time();
        GAME_STATE = 3;
    }
    if (button_state[1]==1) {
        speed++;
    }


    // motion
    if (JS_Val == 1) {
        car.x += speed/2;
    }else if (JS_Val == 2) {
        speed+=2;


    } else if (JS_Val == 3) {
        car.x-=speed/2;
    } else if (JS_Val == 4) {
        speed-=2;
    } else {
        speed-=1;
    }
    if (speed > 10) {
        speed = 10;
    } else if (speed < 0) {
        speed = 0;
    }
    distance+=speed/2;

    move_sprites();

    if (distance > 59) {
        GAME_STATE = 4;
    }

    for (int i=0; i<10; i++) {
        if(check_collision(car, trees[i])) {
            car.x = screen_width()/2;
            trees[i].y = -10;
        }
    }
    for (int i=0; i<3; i++) {
        if (check_collision(car, roadblocks[i])) {
            car.x = screen_width()/2;
            roadblocks[i].y = -10;
        }
    }
}







//-------MAIN function--------------------------------
int main() {
    set_clock_speed(CPU_8MHz);
    srand(get_system_time());
    // Setup the hardware
    init_hardware();
    create_sprites();
    while (GAME_STATE >= 0 && GAME_STATE < 5) {
        // clearing screen
        clear_screen();

        // checking inputs
        inputs();
        // intro
        if (GAME_STATE==0) {
            intro_menu();
            status_reset();
        //countdown
        } else if (GAME_STATE==1){
            countdown();
        // Game
        } else if (GAME_STATE==2){
            draw_sprites();
            game();
            draw_road();
            status_bar();
            border();

        // pause
        } else if (GAME_STATE==3) {
            pause();
        // game over
        } else if (GAME_STATE==4) {
            game_over();
        }
        // Show the screen and have a short nap
        show_screen();
        _delay_ms(100);
    }
    // We'll never get here...
    clear_screen();
    return 0;
}






/*INTERUPTS --------------------------------------
------------------------------------------------*/

/*
* Interrupt service routines
*/
ISR(TIMER0_OVF_vect) {



}


ISR(TIMER1_OVF_vect) {
    ovf_count++;
}
