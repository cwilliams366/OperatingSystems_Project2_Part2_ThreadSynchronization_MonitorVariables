#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

typedef struct KeyandValueStruct
{
    MenuItem key;
    int value;
    struct KeyandValueStruct *next;
}KeyandValue;

//Prototype Functions
bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void IsCompleted(BENSCHILLIBOWL* bcb);
void free_orders(BENSCHILLIBOWL *bcb);
Order favorite_meal(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

//Global variables
bool orders_completed = false;


/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem()
{
    //Pick a random number between 0 and the length of the main menu
    int menu_item_index = rand() % (BENSCHILLIBOWLMenuLength-1) + 0;
    
    //Return the menu item at the random index
    return BENSCHILLIBOWLMenu[menu_item_index];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) 
{

    
    //Allocate memory for the restaurant
    BENSCHILLIBOWL* bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    
    //Initialize the restaurant   
    //Create the mutex and condition variables needed to instantiate the restaurant
    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);
    
    //Set the restaurant's max size and expected number of orders and initialize the restaurant's orders queue
    bcb->max_size = max_size;
    bcb->expected_num_orders = expected_num_orders;
    bcb->current_size = 0;
    bcb->next_order_number = 1;
    bcb->orders_handled  = 0;
    bcb->orders = (Order *) malloc(sizeof(Order) * max_size);

     
    //Open the restaurant
    printf("Restaurant is open!\n");
    return bcb;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */

void CloseRestaurant(BENSCHILLIBOWL* bcb) 
{
    //Check that all orders have been fulfilled
    printf("--------------------------------------------------\n");
    printf("Orders Fulfilled: %d\n", bcb->orders_handled);
    printf("Expected Orders: %d\n", bcb->expected_num_orders);
    printf("Total Customers: %d\n",bcb->orders_handled);
    //Deallocate your resources
    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);
    //Free the space of the restaurant order queue
    free_orders(bcb);
    //Close the restaurant
    printf("Restaurant is closed!\n");
}

//Completely clear the restaurant orders queue
void free_orders(BENSCHILLIBOWL *bcb) 
{
 Order *current = bcb->orders;
  while (current)
    {
    Order *next = current->next;
    free(current);
    current = next;
  }
  bcb->orders = NULL;
}


/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) 
{
    //Lock the mutex for the restaurant 
    pthread_mutex_lock(&bcb->mutex);
    
    //Check if the restaurant is full
    while(IsFull(bcb))
    {
        //If the restaurant queue is full, wait for a signal from a cook to add another order
        printf("Restaurant order queue is full...Please wait for a cook to add an order.\n");
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    //Add the order to the back of the queue
    Order *head = bcb->orders;
    if(IsEmpty(bcb))
    {
        //Populate the order number of the order
        order->order_number = bcb->next_order_number++;
        //Simply add the new order to order queue as the head of the queue
        bcb->orders = order;
        bcb->orders->next = NULL;
    }
    else
    {
        //Traverse the order to add to the last node of the queue
        while(head)
        {
            //If there isn't a another order, add the new order to the end of the queue
            if(head->next == NULL)
            {
                //Add order to the end of the queue
                head->next = order;
                //Populate the order number of the order
                order->order_number = bcb->next_order_number++;
                break;
            }
            else
            {
                //Move to the next order within queue
                head = head->next;
            }
        }
    }

    //Increment the current size of the queue
    bcb->current_size++;
    
    //Report that customer's new order has been added to the queue
    printf("Order #%d has been added to the queue.\n", order->order_number);

    // Notify the cooks that there is a new order
    pthread_cond_broadcast(&bcb->can_get_orders);
    
    //Unlock the mutex for the restaurant
    pthread_mutex_unlock(&bcb->mutex);
    
    //Return the order's number
    return order->order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) 
{
    //Lock the mutex for the restaurant
    pthread_mutex_lock(&bcb->mutex);

    //Check if all of the customer orders are complete
    IsCompleted(bcb);
    
    //Check to see if the restaurant's order queue is empty
    while(IsEmpty(bcb) && !orders_completed)
    {
        //If the restaurant's order queue is empty, wait for a signal from a customer to add an order
        printf("There are currently no orders availble...Please wait for a customer to add an order.\n");
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    //Check if there are no more orders left
    if(IsEmpty(bcb) && orders_completed)
    {
        //Notify the other cooks that there are no orders left
        pthread_cond_broadcast(&bcb->can_get_orders);
        //Unlock the mutex for the restaurant
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }
    
    //Get the order from the front of the queue
    Order *currentOrder = bcb->orders;
    //Remove the current front order out of the queue
    bcb->orders = bcb->orders->next;
    //Reduce the total order amount
    bcb->current_size--;
  
     //Announce the retrieved order
    printf("Order #%d has been retrieved!\n", currentOrder->order_number);
    
    //Unlock the mutex for the restaurant
    pthread_mutex_unlock(&bcb->mutex);
    
    //Return the order
    return currentOrder;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) 
{
    //Check if the restaurant's order queue is empty
    if(bcb->current_size <= 0)
    {
        return true;
    }
  return false;
}

//Optional helper function for generic implementation
bool IsFull(BENSCHILLIBOWL* bcb) 
{
    //CHeck if the restaurant's order queue is full
    if(bcb->current_size >= bcb->max_size)
    {
        return true;
    }
  return false;
}

//Optional helper function; Checks to see if all customer orders have been fulfilled
void IsCompleted(BENSCHILLIBOWL* bcb)
{
    //Check if the orders handled is equal to the expected number of orders
    if(bcb->orders_handled == bcb->expected_num_orders)
    {
        orders_completed = true;
    }
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) 
{
    
}

