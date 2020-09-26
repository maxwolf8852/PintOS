
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"


struct bridge_data BD;
struct semaphore car_e_left;
struct semaphore car_s_right;
struct semaphore car_e_right;
struct semaphore car_s_left;
static int bridge_count = 0;
bool change = false;
int two_change = 0;  // check for changes to run 2 threads in one time

static int set_dir (int dir){
return (BD.left_e<BD.right_e)?1:(BD.left_e>BD.right_e)?0:(BD.left_e==0)?(BD.left_s<BD.right_s)?1:(BD.left_s>BD.right_s)?0:dir:dir; // if there are more cars in one direction then the priority goes to it     
}
static int set_type(void){
return (BD.left_e==0 && BD.right_e==0)?0:1;
}

static void minus_count (int type, int dir){
if(type == 0 && dir == 0)
BD.left_s--;
else if (type == 0 && dir == 1)
BD.right_s--;
if(type == 1 && dir == 0)
BD.left_e--;
else if (type == 1 && dir == 1)
BD.right_e--;
}

static void car_sema_down (int type, int dir){
if(type == 0 && dir == 0)
sema_down(&car_s_left);
else if (type == 0 && dir == 1)
sema_down(&car_s_right);
else if(type == 1 && dir == 0)
sema_down(&car_e_left);
else if (type == 1 && dir == 1)
sema_down(&car_e_right);
}

static void car_sema_up (int type, int dir){
if(type == 0 && dir == 0)
sema_up(&car_s_left);
else if (type == 0 && dir == 1)
sema_up(&car_s_right);
else if(type == 1 && dir == 0)
sema_up(&car_e_left);
else if (type == 1 && dir == 1)
sema_up(&car_e_right);
}

bool car_exist (int type, int dir){
if(type == 0 && dir == 0)
return (BD.left_s>1);
else if (type == 0 && dir == 1)
return (BD.right_s>1);
else if(type == 1 && dir == 0)
return (BD.left_e>1);
else if (type == 1 && dir == 1)
return (BD.right_e>1);
}


void narrow_bridge_init(unsigned int num_vehicles_left, unsigned int num_vehicles_right,
        unsigned int num_emergency_left, unsigned int num_emergency_right)
{

	sema_init(&car_e_left, 0);
	sema_init(&car_s_right, 0);
	sema_init(&car_e_right, 0);
	sema_init(&car_s_left, 0);

        BD.left_s = num_vehicles_left;
	BD.right_s = num_vehicles_right;
	BD.left_e = num_emergency_left;
	BD.right_e = num_emergency_right;

	BD.dir = set_dir(1);
	BD.type = set_type();

}




void arrive_bridge(enum car_priority prio, enum car_direction dir)
{
if(BD.type!= prio || BD.dir != dir || bridge_count==2) // lock
	car_sema_down(prio, dir);

// main body
bridge_count++;	
minus_count(prio, dir);
//

// set directions, types && changes
int temp = set_dir(dir);
change = (temp!=BD.dir);
BD.dir = temp;
temp = set_type();
change = (change)?change:(temp!=BD.type);
BD.type = temp;
//
}

void exit_bridge(enum car_priority prio, enum car_direction dir)
{
if(two_change==0) car_sema_up(BD.type, BD.dir);
if(change){
if(car_exist(BD.type, BD.dir) && two_change==0) { car_sema_up(BD.type, BD.dir); two_change++;}
}
else two_change = 0;

if(bridge_count == 1) two_change = 0;

bridge_count--;
}
