/* PROJECT DESCRIPTION (ADDED 5/16/2016): 

	Kernel module implementation of simulating an elevator
	
	Used to demonstrate C skills
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm-generic/uaccess.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/unistd.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris");
MODULE_DESCRIPTION("Module that simulates an elevator");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL
#define MAX_WEIGHT 16
#define MAX_PASS 8
#define MAX_FLOOR 10
#define MIN_FLOOR 1
#define KMALLOC_FLAGS __GFP_WAIT | __GFP_IO | __GFP_FS


typedef struct PassengerStruct {
int type;  /* 0 = adult, 1 = child, 2 = bellhop, 3 = room service */
int numPassengers; /* adult, child, and room service = 1; bellhops count for 2 */
int weight; /* 2 = 1 weight unit; 1 = .5 WU, 4 = 2 WU, etc */
int destFloor;
int startFloor;
struct list_head list;
} Passenger; 

static struct task_struct *elevator_thread;

static struct mutex _mutex;

static struct file_operations fops;

static char *msg1;
static char *msg2;
static char *movementStateString;

static int elevatorHasStarted = 0;
static int elevatorIsStopping = 0;
static int stopThread = 1;
int floorStops[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static int read_p;
static int currentFloor;
static int nextFloor;
static int currentWeight;
static int currentPassegLoad;

enum MovementType { IDLE, UP, DOWN, LOADING, STOPPED };
enum MovementType movementState = STOPPED;

int numServiced[11]; 
struct list_head floors[11]; /* floor[1] through floor[10] - floor[0] is ignored */
struct list_head elevator;

/* converts enum to string */
void set_movement_state_string(void) {
	if (movementState == IDLE)
		strcpy(movementStateString, "IDLE");
	
	else if (movementState == STOPPED)
		strcpy(movementStateString, "STOPPED");

	else if (movementState == UP) 
		strcpy(movementStateString, "UP");

	else if (movementState == DOWN)
		strcpy(movementStateString, "DOWN");

	else if (movementState == LOADING)
		strcpy(movementStateString, "LOADING");

}

/* called when /proc/elevator is opened */
int elevator_proc_open(struct inode *sp_inode, struct file *sp_file) {
	struct list_head *ptr;
	Passenger *pass;
	int totalWeight = 0;
	int currentNumPass = 0;
	int i;
	char temp[6];
	
	
	printk("proc called open\n");
	
	read_p = 1;
	msg1 = kmalloc(sizeof(char) * 128, KMALLOC_FLAGS);
	msg2 = kmalloc(sizeof(char) * 4096, KMALLOC_FLAGS);	
	movementStateString = kmalloc(sizeof(char) * 16, KMALLOC_FLAGS);

	strcpy(msg1, "");	
	strcpy(msg2, "");	
	strcpy(movementStateString, "");

	if (msg1 == NULL || msg2 == NULL || movementStateString == NULL) {
		printk("ERROR, elevator_proc_open");
		return -ENOMEM;
	}
	
	mutex_lock_interruptible(&_mutex);
	set_movement_state_string();
	
	sprintf(msg1, "movement state: %s\n", movementStateString);
	strcat(msg2, msg1);

	sprintf(msg1, "current floor: %d\n", currentFloor);
	strcat(msg2, msg1);
	
	sprintf(msg1, "next floor serviced: ");

	if (nextFloor < MIN_FLOOR) 
		strcat(msg1, "None\n\n");
	
	else {
		sprintf(temp, "%d\n\n", nextFloor);
		strcat(msg1, temp); 
	}
	
	strcat(msg2, msg1);
	
	/* tally up the load of the passengers in the elevator */
	list_for_each(ptr, &elevator) {
		pass = list_entry(ptr, struct PassengerStruct, list);
		currentNumPass += pass->numPassengers;
		totalWeight += pass->weight;
	}

	sprintf(msg1, "elevator's current load:\n  total passenger units: %d\n  total weight units: %d", currentNumPass, totalWeight / 2);
	strcat(msg2, msg1);
	
	if (totalWeight % 2 != 0)
		strcat(msg2, ".5");
	
	strcat(msg2, "\n\n");
	
	
	/* tally weight of the passengers of each floor */
	for (i = MIN_FLOOR; i <= MAX_FLOOR; i++) {
		totalWeight = 0;
		list_for_each(ptr, &floors[i]) {	
			if (list_empty(&floors[i]) == 0) {
				pass = list_entry(ptr, struct PassengerStruct, list);
				totalWeight += pass->weight;
			}
		}
		sprintf(msg1, "floor %d:\n", i);
		strcat(msg2, msg1);
		sprintf(msg1, "  current weight load: %d", totalWeight / 2);
		strcat(msg2, msg1);
		
		if (totalWeight % 2 != 0)
			strcat(msg2, ".5");
	
		strcat(msg2, "\n");	
		
		sprintf(msg1, "  passengers serviced: %d\n\n", numServiced[i]);
		strcat(msg2, msg1);
	}
	
	
	mutex_unlock(&_mutex);
	return 0;
}

ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len;

 	len = strlen(msg2);
	
	read_p = !read_p;
	if (read_p) 
		return 0;
	
	
	printk("proc called read\n");
	copy_to_user(buf, msg2, len);
	
	return len;
}

int elevator_proc_release(struct inode *sp_inode, struct file *sp_file) {
	printk("proc called release\n");

	kfree(msg2);
	kfree(msg1);
	kfree(movementStateString);
	
	return 0;
}

int load_passengers(void) {
	Passenger *passenger = NULL;
	struct list_head *ptr = NULL, *temp = NULL;
	int numLoaded = 0;
	
	/* load as many people from floor into elevator as possible */
	list_for_each_safe(ptr, temp, &floors[currentFloor]) {
		passenger = list_entry(ptr, struct PassengerStruct, list);
		if (currentWeight + passenger->weight <= MAX_WEIGHT && currentPassegLoad + passenger->numPassengers <= MAX_PASS) {
			list_del_init(&passenger->list);
			list_add(&passenger->list, &elevator); 
			currentWeight += passenger->weight;
			currentPassegLoad += passenger->numPassengers;
			
			numLoaded++;
			printk("added passenger to elevator\n");
		}
	}
	
	return numLoaded;
}

int unload_passengers(void) {
	struct list_head *ptr, *temp;
	Passenger *pass;
	int numUnloaded = 0;
	
	list_for_each_safe(ptr, temp, &elevator) {
		pass = list_entry(ptr, struct PassengerStruct, list);
		if (pass->destFloor == currentFloor) {
			numServiced[pass->startFloor]++;
			currentWeight -= pass->weight;
			currentPassegLoad -= pass->numPassengers;
			list_del_init(&pass->list);
			kfree(pass);
			numUnloaded++;
		}
	}
	
	return numUnloaded;
}

int movement_to_int(void) {
	if (movementState == UP)
		return 1;
	
	else if (movementState == DOWN)
		return -1;
	
	else
		return 0;
}

int is_request_above(void) {
	int i;
	
	for (i = currentFloor + 1; i <= MAX_FLOOR; i++) {
		if (floorStops[i] == 1)
			return 1;
	}
	
	return 0;
}

int is_request_below(void) {
	int i;
	
	for (i = currentFloor - 1; i >= MIN_FLOOR; i--) {
		if (floorStops[i] == 1)
			return 1;
	}
	
	return 0;
}

void set_next_floor(void) {
	int curr = currentFloor;

	while(1) {
		if (floorStops[curr + movement_to_int()] == 1) {
			nextFloor = curr + movement_to_int();
			break;
		} 
		else 
			curr = curr + movement_to_int();

		if (curr < MIN_FLOOR || curr > MAX_FLOOR)
			break;
	}
}

void run_elevator(void) {
	struct list_head *ptr;
	Passenger *pass;
	int i = 0;
	int lastFloorUp = MAX_FLOOR, lastFloorDown = MIN_FLOOR;
	int hasUnloaded = 0;
	int numFloorStops = 0;
	
	printk("elevator has entered operational mode\n");

	while(!kthread_should_stop()) {
		mutex_lock_interruptible(&_mutex);
		
		/* unload passengers that want currentFloor as destination */
		if (unload_passengers() > 0) {
			printk("unloading passengers at floor %d.\n", currentFloor);
			movementState = LOADING;
			mutex_unlock(&_mutex);
			ssleep(1);
			mutex_lock_interruptible(&_mutex);
			movementState = IDLE;
			hasUnloaded = 1;
			
			printk("unloaded passenger.\n");
		}
		
		/* load passengers at current floor */
		if (elevatorIsStopping == 0) {
			if (load_passengers() > 0) {
				printk("loading passengers at floor %d.\n", currentFloor);
				if (hasUnloaded == 0) {
					movementState = LOADING;
					mutex_unlock(&_mutex);
					ssleep(1);
					mutex_lock_interruptible(&_mutex);
					movementState = IDLE;
				}
				printk("loaded passenger.\n");
			}
		}
		hasUnloaded = 0;
	
		/* reset floorStops */
		for (i = MIN_FLOOR; i <= MAX_FLOOR; i++)
			floorStops[i] = 0;
	
		/* determine current passengers destination floors */
		list_for_each(ptr, &elevator) {
			pass = list_entry(ptr, struct PassengerStruct, list);
			floorStops[pass->destFloor] = 1;
		}
		
		/* determine start floors of waiting passengers if elevator isn't in stop sequence
		 * and if elevator can fit at least one waiting passenger */
		if (elevatorIsStopping == 0) {
			for (i = MIN_FLOOR; i <= MAX_FLOOR; i++) {
				list_for_each(ptr, &floors[i]) {
					pass = list_entry(ptr, struct PassengerStruct, list);	
					if (currentWeight + pass->weight <= MAX_WEIGHT) 
						floorStops[i] = 1;
				}
			}
		}
		
		/* 
		 * use LOOK scheduling algorithm to offload and load passengers, 
		 * start UP if current floor is > 5 and a floorStop is above currentFloor
		 * start DOWN if current floor is <= 5 and a floorStop is below currentFloor
		 * otherwise set to IDLE
		 */
		if ((currentFloor > 5 && is_request_above()) || (is_request_below() == 0 && is_request_above())) 
			movementState = UP;
		
		else if (is_request_below())
			movementState = DOWN;

		else 
			movementState = IDLE;
			

		if (movementState == UP || movementState == DOWN) {
			/* find the highest and lowest the elevator needs to go */
			for (i = MAX_FLOOR; i > currentFloor; i--) {
				if (floorStops[i] == 1) {
					lastFloorUp = i;
					break;
				}
			}
			for (i = MIN_FLOOR; i < currentFloor; i++) {
				if (floorStops[i] == 1) {
					lastFloorDown = i;
					break;
				}
			}
			
			set_next_floor();
			
			/* if it needs to switch directions, do so, if not, move towards nextFloor */
			if (currentFloor == lastFloorUp && movementState == UP) {
				movementState = DOWN;
				lastFloorUp = MAX_FLOOR;
			}
			else if (currentFloor == lastFloorDown && movementState == DOWN) {
				movementState = UP;
				lastFloorDown = MIN_FLOOR;
			}
			else {
				printk("moving to floor: %d\n", currentFloor + movement_to_int());
				mutex_unlock(&_mutex);
				ssleep(2);
				mutex_lock_interruptible(&_mutex);
				currentFloor = currentFloor + movement_to_int();
				printk("currentFloor is now: %d\n", currentFloor);
				set_next_floor();
			}
		}

		if (elevatorIsStopping && list_empty(&elevator)) {
			movementState = STOPPED;
			nextFloor = -1;
			mutex_unlock(&_mutex);
			return;
		}
		
		/* count number of floor stops */
		for (i = MIN_FLOOR; i <= MAX_FLOOR; i++) {
			if (floorStops[i] ==  1)
				numFloorStops++;
		}

		/* if no stops requested, go to IDLE */
		if (numFloorStops == 0) {
			movementState = IDLE;
			nextFloor = -1;
			mutex_unlock(&_mutex);
			ssleep(1);
			mutex_lock_interruptible(&_mutex);
		}
		numFloorStops = 0;
		
		mutex_unlock(&_mutex);
	}
}

int wait_for_elevator_to_start(void* data) {
	while (!kthread_should_stop() && elevatorIsStopping == 0) {
		ssleep(1);
		mutex_lock_interruptible(&_mutex);
		if (elevatorHasStarted == 1) {
			mutex_unlock(&_mutex);
			run_elevator();
			break;
		}
		mutex_unlock(&_mutex);
	}
	printk("outside loop in wait_for_elevator_to_start\n");

	stopThread = 0;
	return 0;
}

extern long (*STUB_start_elevator)(void);
long start_elevator(void) {
	if (elevatorHasStarted == 1) {
		printk("Elevator already started\n");
		return 1;
	}

	printk("Starting elevator\n");
	
	movementState = IDLE; 
	elevatorHasStarted = 1;
	
	return 0;
}

void set_pass_wgt_and_num(Passenger *passenger) {
	if (passenger->type != 2) {
		passenger->numPassengers = 1;

		if (passenger->type == 0) passenger->weight = 2;
		else if (passenger->type == 1) passenger->weight = 1;
		else if (passenger->type == 3) passenger->weight = 4; 
	} 
	else {
		passenger->numPassengers = 2;
		passenger->weight = 4;
	}
}

extern long (*STUB_issue_request)(int,int,int);
long issue_request(int passenger_type, int start_floor, int destination_floor) {
	Passenger *newPassenger = kmalloc(sizeof(Passenger), KMALLOC_FLAGS);

	if (passenger_type < 0 || passenger_type > 3) 
		return 1;

	if (start_floor < 1 || start_floor > 10) 
		return 1;

	if (destination_floor < 1 || destination_floor > 10) 
		return 1;

	if (destination_floor == start_floor)
		return 1;

	printk("New request: %d, %d => %d\n", passenger_type, start_floor, destination_floor);

	newPassenger->startFloor = start_floor;
	newPassenger->destFloor = destination_floor;
	newPassenger->type = passenger_type;
	set_pass_wgt_and_num(newPassenger);
	
	mutex_lock_interruptible(&_mutex);
	list_add_tail(&newPassenger->list, &floors[start_floor]);
	mutex_unlock(&_mutex);
	return 0;
}

extern long (*STUB_stop_elevator)(void);
long stop_elevator(void) {
	
	printk("Stopping elevator\n");
	
	if (elevatorIsStopping == 1) {
		printk("Elevator is already in the process of deactivating or is already deactivated");
		return 1;
	}
	
	elevatorIsStopping = 1;
	
	return 0;
}


static int elevator_init(void) {
	int i;

	mutex_init(&_mutex);
	
	elevator_thread = kthread_run(wait_for_elevator_to_start, NULL, "elevator");
	if (IS_ERR(elevator_thread)) {
		printk("ERROR! kthread_run, elevator_thread\n");
		return PTR_ERR(elevator_thread);
	}
	
	printk("/proc/%s create\n", ENTRY_NAME); 
	fops.open = elevator_proc_open;
	fops.read = elevator_proc_read;
	fops.release = elevator_proc_release;
	
 	STUB_start_elevator =& (start_elevator);
	STUB_issue_request =& (issue_request);
	STUB_stop_elevator =& (stop_elevator);

	movementState = STOPPED;
	currentFloor = MIN_FLOOR;
	nextFloor = -1;
	currentWeight = 0;
	currentPassegLoad = 0;
	
	for (i = MIN_FLOOR; i <= MAX_FLOOR; i++) {
		INIT_LIST_HEAD(&floors[i]);
		numServiced[i] = 0;
	}
	
	INIT_LIST_HEAD(&elevator);

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk("ERROR! proc_create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}

	return 0;
}

static void elevator_exit(void) {
	int ret, i;
	struct list_head *temp, *ptr;
	Passenger *pass;

	if (stopThread == 1) {
		ret = kthread_stop(elevator_thread);
		if (ret != EINTR)
			printk("elevator thread has stopped\n");
		else
			printk("error in stopping elevator thread!\n");
	}

	list_for_each_safe(ptr, temp, &elevator) {
		pass = list_entry(ptr, struct PassengerStruct, list);
		list_del_init(ptr);
		kfree(pass);
	}
	
	for (i = MIN_FLOOR; i <= MAX_FLOOR; i++) {
		list_for_each_safe(ptr, temp, &floors[i]) {
			pass = list_entry(ptr, struct PassengerStruct, list);
			list_del_init(ptr);
			kfree(pass);
		}
	} 
	
	remove_proc_entry(ENTRY_NAME, NULL);
	printk("Removing /proc/%s.\n", ENTRY_NAME);

	STUB_issue_request = NULL;
 	STUB_start_elevator = NULL;
	STUB_stop_elevator = NULL;

}

module_init(elevator_init);
module_exit(elevator_exit);
