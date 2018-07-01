#include <cab202_graphics.h>
#include <cab202_sprites.h>
#include <cab202_timers.h>
#include <stdlib.h> 
#include <math.h>
#include <curses.h>


// PART A------------------------------------------------------------ //

/* GLOBALS-------------------------------------------------------------
---------------------------------------------------------------------*/

// CONSTANTS
#define DELAY 3 /* milliseconds */

// GLOBAL VARIABLES
int count = 0;
int game_state = 0; // 0: start screen, 1: gameplay: 2: gameover
int health;
double fuel;
int gear;
double distance;
double tacho;
double speed;
double start_time;
double end_time;
double game_time;
int lane_left;
int lane_right;
int car_collided = 0;
int car_start_x;
int car_start_y;
bool reached_destination = false;
bool petrol_collided = false;
bool refueling = false;
double refueling_start;

//function declarations
void create_dash(void);
void initialise(void);
void process_input(void);
void draw_border(void);
void draw_sprites(void);
void draw_dash(void);
void motion(void);
void speed_gear_sync(void);
bool check_collision(sprite_id sprite_1, sprite_id sprite_2);
void check_refuel(void);


// Sprite Declarations
sprite_id main_menu;
sprite_id game_over;
sprite_id car;
sprite_id trees[20];
sprite_id roadblocks[3];
sprite_id petrol;


// Sprite Images
char * main_menu_image =
/**/  "**************************************************"
/**/  "*           N9730877: Rab Dawanincura            *"
/**/  "*            RACE TO ZOMBIE MOUNTAIN             *"
/**/  "*                                                *"
/**/  "*        Use Left/Right to steer the car         *"
/**/  "*        Up to Accelerate. Down To Brake         *"
/**/  "*             Press W key to upshift.            *"
/**/  "*            Press S key to downshift.           *"
/**/  "*                                                *"
/**/  "* Stop near fuel station for 3 seconds to refuel *"
/**/  "*                                                *"
/**/  "*      Downshift when braking is Automatic.      *"
/**/  "*       Lift off throttle when upshifting.       *"
/**/  "*                                                *"
/**/  "*               Press Q to End Game              *"
/**/  "*          Press any key to continue!            *"
/**/  "*    (BEST PLAYED ON LARGER THAN 80x24 screen)   *"
/**/  "*                                                *"
/**/  "* Rules: Stop near fuel pump to refuel, dont hit *"
/**/  "*  fuel station, travel 5km to reach zombie mt.  *"
/**/  "**************************************************";

char * game_over_image =
/**/  "                                                  "
/**/  "                                                  "
/**/  "                                                  "
/**/  "                                                  "
/**/  "                    GAME OVER                     "
/**/  "                                                  "
/**/  "             RACE TO ZOMBIE MOUNTAIN              "
/**/  "                                                  "
/**/  "                                                  "
/**/  "                 Press R to Restart               "
/**/  "                Press Q to Quit Game              "
/**/  "                                                  "
/**/  "                                                  "
/**/  "                                                  "
/**/  "                                                  "
/**/  "                                                  ";


//DIMENSIONS 14x8
char * car_image =
/**/ "    __AA__    "
/**/ "   /------\\   "
/**/ " []--------[] "
/**/ "   |------|   "
/**/ "   |------|   "
/**/ "   \\------/   "
/**/ "[]--|____|--[]"
/**/ "[]--|____|--[]";

// 7 x 5 Dimensions
char * tree_image =
/**/ "  ***  "
/**/ "*******"
/**/ "*******"
/**/ "*******"
/**/ "  ***  ";

// 14 x 5
char * roadblock_image =
/**/ "  ,----------,"
/**/ " /   STOP!  / "
/**/ "-----------/  "
/**/ "  ||    ||    "
/**/ "  ||    ||    ";


char * petrol_image =
/**/ "_____"
/**/ "| f |"
/**/ "| u |"
/**/ "| e |"
/**/ "|_l_|";

/* FUNCTIONS --------------------------------------------------
--------------------------------------------------------------*/

void setup_trees(void) {
    for (int i=0; i<10; i++) {
        int randx = rand() % (screen_width()/2)-24-7;
        int randy = (screen_height()/5)*(i+1);
        trees[i] = sprite_create(randx, randy, 7, 5, tree_image);
    }
    for (int i=10; i<20; i++) {
        int randx = rand() % (screen_width() -(screen_width()/2)+25 ) + (screen_width()/2)+25;
        int randy = (screen_height()/5)*(i-9);
        trees[i] = sprite_create(randx, randy, 7, 5, tree_image);
    }
}

void setup_roadblocks(void) {
    int base = (screen_width()/2)-23;
    for (int i=0; i<1; i++) {
        roadblocks[(3*i)] = sprite_create(base,(i*20)+6 ,14,5,roadblock_image);
        roadblocks[(3*i)+1] = sprite_create(base + 16,(i*30)+8 ,14,5,roadblock_image);
    }
}
void no_collision_trees(void) {
    bool collision = true;
    while (collision ) {
        for (int i=0; i<20; i++) {
            collision = check_collision(trees[i], trees[i+1]);
            if (collision) {
                sprite_move(trees[i], 3, 15);
            }
        }

    }
}


void car_collisions(void) {
    if (check_collision(car,petrol)) {
        petrol_collided = true;
        speed = 0;
        health=-1;

    }
    for (int i= 0; i<20; i++) {
        if (check_collision(car, trees[i])) {
            speed = 0;
            sprite_move_to(trees[i],0, -10);
            sprite_move_to(car, car_start_x, car_start_y);
            health -= 12.5;
            fuel = 100;
        }
    }
    for (int i= 0; i<2; i++) {
        if (check_collision(car, roadblocks[i])) {
            speed = 0;
            sprite_move_to(roadblocks[i],rand()%(lane_right-lane_left-14)+lane_left, -10);
            sprite_move_to(car, car_start_x, car_start_y);
            fuel = 100;
            health -= 12.5;
        }
    }
}

void check_refuel(void) {
    if ((sprite_x(petrol) - (sprite_x(car)) < 20) && speed == 0 && !refueling) {
        int refueling_window = sprite_y(car) - sprite_y(petrol);
        if (refueling_window > -5 && refueling_window < 4) {
            refueling = true;
            refueling_start = get_current_time();
        }
    }
}
// Initlise Function
void initialise(void) {
    // initial values
    health = 100;
    fuel = 100;
    distance = 0;
    speed = 0;
    gear = 0;
    start_time = get_current_time();
    car_start_x=(screen_width()/2)-7;
    car_start_y=(screen_height())-14,

    // create sprites
    petrol = sprite_create(car_start_x+32,-10,5,5,petrol_image);
    main_menu = sprite_create((screen_width()/2)-25, (screen_height()/2)-10,50,21, main_menu_image);
    game_over = sprite_create((screen_width()/2)-25, (screen_height()/2)-8,50,16, game_over_image);
    car = sprite_create(car_start_x, car_start_y,14,8,car_image);
    setup_trees();
    setup_roadblocks();

}

// Process inputs
void process_input(void) {
	int key = get_char();
    int xh;
    switch(game_state) {
        // welcome screen
        case 0:
            initialise();
            if (key > 0) {
                game_state=1;
            }
            break;

        // game play
        case 1:
            if (health < 1.0) {
                game_state=2;
            }
            else if (refueling && fuel != 100) {
                if ((get_current_time() - refueling_start) > 3.0) {
                    fuel = 100;
                    refueling = false;
                }
            } else if (fuel<1) {
                game_state=2;

            } else {
                if (distance > 0.3) {
                    game_state=2;
                    reached_destination = true;
                }
                xh = round(sprite_x(car));
                if (key == KEY_LEFT && xh > 1) {
                    if (speed > 0) {
                        if (speed < 3) {
                            sprite_move(car, -2, 0);
                        } else if (speed < 7) {
                            sprite_move(car, -3, 0);
                        } else if (speed < 10) {
                            sprite_move(car, -5, 0);
                        }
                    }
        	    }
                else if (key == KEY_RIGHT && xh < screen_width() - sprite_width(car) - 1 ) {
                    if (speed > 0) {
                        if (speed < 3) {
                            sprite_move(car, 2, 0);
                        } else if (speed < 7) {
                            sprite_move(car, 3, 0);
                        } else if (speed < 10) {
                            sprite_move(car, 5, 0);
                        }sprite_move(car, +2, 0);
                    }
                }

                if (key == KEY_UP) {
                    if (sprite_x(car)>lane_left && (sprite_x(car)+13)<lane_right) {
                        if(gear > 0) {
                            if (gear == 3) {
                                if (speed < 10) {
                                    speed += 0.065;
                                    if (speed > 10) {
                                        speed = 10;
                                    }
                                }
                            } else if (gear == 2){
                                if (speed < 7) {
                                    speed += 0.07;
                                    if (speed > 7) {
                                        speed = 7;
                                    }
                                }

                            } else if (gear == 1) {
                                if (speed < 4) {
                                    speed += 0.075;
                                    if (speed > 4) {
                                        speed = 4;
                                    }
                                }

                            }
                        }
                    }
                    else {
                        if (speed>3) {
                            speed = 3;
                        } else {
                            speed += 0.065;
                        }
                    }

                }
                else {
                    if (speed > 3) {
                    }
                    if (speed>0.025) {
                        speed-= 0.0065;
                    }
                    else {
                        speed = 0;
                    }
                }

                if (key == KEY_DOWN) {
                    if (speed > 0) {
                        speed -= 0.6;
                        speed_gear_sync();
                    }
                }

                if ((key == 'w' || key == 'W') && gear < 3) {
                    gear++;
                }
                else if ((key == 'S' || key == 's') && gear > -1) {
                    gear--;
                    if (gear == 2) {
                        speed = 4;
                    } else if (gear == 1) {
                        speed = 4;
                    }
                }
            }
                break;

        // game over
        case 2:
            if (key == 'R' || key == 'r') {
                game_state = 0;
            } else if (key == 'Q' || key == 'q') {
                game_state = 4;
            }
            break;

    }

}

void speed_gear_sync(void) {
    if (speed < 0) {
        speed = 0;
    }
    if (speed > 7) {
        gear = 3;
    } else if (speed > 4) {
        gear = 2;
    } else if (speed > 0 && speed < 4){
        gear = 1;
    } else {
        gear = 0;
    }
}


// motion
void motion(void) {
    for (int i=0; i<20; i++) {

        if (sprite_y(trees[i]) < screen_height()+20 && sprite_y(trees[i]) > - 20) {
            sprite_move(trees[i], 0, speed/15);
            if (!reached_destination) {
                fuel -= (speed/10000);
                distance += (speed/150000);
                if (fuel < 0) {
                    speed = 0;
                }
            }
        } else {
            sprite_move_to(trees[i],sprite_x(trees[i]),(rand()%15)-20);
        }
    }

    for (int i=0; i<2; i++) {
        sprite_move(roadblocks[i], 0, speed/15);
    }
    for (int i=0; i<2; i++) {
        if (sprite_y(roadblocks[i]) > screen_height()+(rand()%30)) {
            sprite_move_to(roadblocks[i],(rand()%(lane_right-lane_left-sprite_width(roadblocks[i])-1))+lane_left,-screen_height()-(rand()%30));
            if (i == 1) {
                if(check_collision(roadblocks[0],roadblocks[1])) {
                    i--;
                }
            }

        }
    }
    if (((int)game_time)%15 ==0) {
        sprite_move(petrol,0,speed/15);
    }
    if (sprite_y(petrol) > screen_height()) {
        sprite_move_to(petrol, car_start_x+32, -400);
    }

}

bool check_collision(sprite_id s1, sprite_id s2) {
    int hx = sprite_x(s1);
	int hy = sprite_y(s1);
	int hr = hx + sprite_width(s1) - 1;
	int hb = hy + sprite_height(s1) - 1;
	int zx = sprite_x(s2);
	int zy = sprite_y(s2);
	int zr = zx + sprite_width(s2) - 1;
	int zb = zy + sprite_height(s2) - 1;
	if ( hr < zx ) return false;
	if ( zr < hx ) return false;
	if ( hb < zy ) return false;
	if ( zb < hy ) return false;
	return true;
}



// Draw_border function
void draw_border(void) {
    // Minimum values
    int min_w = 80;
    int min_h = 24;

    // Saving screen_dimensions
    int w = screen_width(), h = screen_height(), ch = '\'';

    // comparing dimensions with minimum values
    if (w <min_w) {
        w = min_w;
    }
    if (h < min_h) {
        h = min_h;
    }

    // drawing border

    // top horizonal
	draw_line(0, 0, w - 1, 0, ch);
    // left vertical
    draw_line(0, 0, 0, h - 1, ch);
    // bottom horizonal
    draw_line(0, h - 1, w - 1, h - 1, ch);
    // right vertical
    draw_line(w - 1, 0, w - 1, h - 1, ch);
}


void draw_lanes(void) {
    draw_line((screen_width()/2)-24,1,(screen_width()/2)-24,screen_height()-7,'|');
    draw_line((screen_width()/2)-8,1,(screen_width()/2)-8,screen_height()-7,'|');
    draw_line((screen_width()/2)+8,1,(screen_width()/2)+8,screen_height()-7,'|');
    draw_line((screen_width()/2)+24,1,(screen_width()/2)+24,screen_height()-7,'|');
    lane_left = (screen_width()/2)-24;
    lane_right = (screen_width()/2)+24;
}

void clear_dash(void) {
    draw_line(1, screen_height()-5, screen_width()-2, screen_height()-5, ' ');
    draw_line(1, screen_height()-4, screen_width()-2, screen_height()-4, ' ');
    draw_line(1, screen_height()-3, screen_width()-2, screen_height()-3, ' ');
    draw_line(1, screen_height()-2, screen_width()-2, screen_height()-2, ' ');
}
void draw_dash(void) {
    // Dashboard Separator
    draw_line(1, screen_height()-6, screen_width()-2, screen_height()-6, '-');
    // dashboard items
    draw_formatted(05, (screen_height()-5), "speed:  %.1f", speed);
    draw_formatted(05, (screen_height()-3), "health:  %d", health);
    draw_formatted(25, (screen_height()-3), "fuel:  %.1f", fuel);
    draw_formatted(40, (screen_height()-3), "distance:  %.2f km", distance);
    draw_formatted(60, (screen_height()-3), "time:  %d: %2.2f",((int)(get_current_time() - start_time)/60), (get_current_time() - start_time)-(60*((int)(get_current_time() - start_time)/60)));
    if (gear > 0) {
        draw_formatted(25, (screen_height()-5), "gear:  %d", gear);
    }
    switch (gear) {
        case 0:
            draw_formatted(25, (screen_height()-5), "gear:  N");
            break;
        case -1:
            draw_formatted(25, (screen_height()-5), "gear:  R", gear);
            break;
        default:
            draw_formatted(25, (screen_height()-5), "gear:  %d", gear);
            break;
    }
    double max_speed;
    if (gear == 1) {
        max_speed = 4.0;
    } else if (gear == 2) {
        max_speed = 7.0;
    } else if (gear == 3) {
        max_speed = 10.0;
    }
    tacho = (speed/max_speed)*10000.0;
    draw_formatted(45, (screen_height()-5), "tacho:  ##");
    if (tacho > 2000 && tacho < 3000) {
        draw_formatted(45, (screen_height()-5), "tacho:  ###");
    } else if (tacho > 3000 && tacho < 4000) {
        draw_formatted(45, (screen_height()-5), "tacho:  ####");
    } else if (tacho > 4000 && tacho < 5000) {
        draw_formatted(45, (screen_height()-5), "tacho:  #####");
    } else if (tacho > 5000 && tacho < 6000) {
        draw_formatted(45, (screen_height()-5), "tacho:  ######");
    } else if (tacho > 6000 && tacho < 7000) {
        draw_formatted(45, (screen_height()-5), "tacho:  #######");
    } else if (tacho > 7000 && tacho < 8000) {
        draw_formatted(45, (screen_height()-5), "tacho:  ########");
    } else if (tacho > 8000 && tacho < 9000) {
        draw_formatted(45, (screen_height()-5), "tacho:  #########");
    } else if (tacho > 9000 && tacho < 10000) {
        draw_formatted(45, (screen_height()-5), "tacho:  ##########");
    }
}


// Draw Sprites function
void draw_sprites(void) {
    switch(game_state) {
        // main menu
        case 0:
            sprite_draw(main_menu);
            break;

        // gameplay
        case 1:
            draw_lanes();
            // obstacles
            for (int i=0; i<20; i++) {
                sprite_draw(trees[i]);
            }
            for (int i=0; i<2; i++) {
                sprite_draw(roadblocks[i]);
            }

            //standard items
            draw_border();
            sprite_draw(car);
            sprite_draw(petrol);
            clear_dash();
            draw_dash();
            end_time = get_current_time();
            break;

        // game over
        case 2:
            game_time = end_time-start_time;
            sprite_draw(game_over);
            if (reached_destination) {
                draw_formatted((screen_width()/2)-13, ((screen_height()/2)-10), "You reached Zombie Mountain");
            }
            else {
                draw_formatted((screen_width()/2)-16, ((screen_height()/2)-10), " You didn't reach Zombie Mountain");

            }
            if (petrol_collided) {
                draw_formatted((screen_width()/2)-16, ((screen_height()/2)-11), "You crashed into the fuel station!");
            } else if (fuel < 1) {
                draw_formatted((screen_width()/2)-16, ((screen_height()/2)-11), " Unfortunately you ran out fuel!");
                process_input();
            } else if (health < 1) {
                draw_formatted((screen_width()/2)-16, ((screen_height()/2)-11), "    Unfortunately you died!");
                process_input();
            }
            draw_formatted((screen_width()/2)-7, ((screen_height()/2)-8), "game time:  %.2f sec", game_time);
            draw_formatted((screen_width()/2)-16, ((screen_height()/2)-7), "distance travelled:  %.2f km", distance);



            break;

    }

}

/* ENTRY POINT------------------------------------------------------- //
---------------------------------------------------------------------*/
int main() {
    setup_screen();
    initialise();
    while (game_state >= 0 && game_state < 3) {
        clear_screen();
        process_input();
        check_refuel();
        motion();
        car_collisions();
        draw_sprites();
        show_screen();
        timer_pause(10);
    }
    cleanup_screen();

}
