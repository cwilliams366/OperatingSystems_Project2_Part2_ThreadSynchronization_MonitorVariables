#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "BENSCHILLIBOWL.h"

// Feel free to play with these numbers! This is a great way to
// test your implementation.
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS NUM_CUSTOMERS


// Global variable for the restaurant.
BENSCHILLIBOWL *bcb;

int order_series = 1;
/**
 * Thread funtion that represents a customer. A customer should:
 *  - allocate space (memory) for an order.
 *  - select a menu item.
 *  - populate the order with their menu item and their customer ID.
 *  - add their order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) 
{   
    //Create a customer and their respective order by allocating space
    //Cast the thread ID to an integer
    int i = 0;
    int customer_id = (int)(long) tid;
    Order* order = (Order*) malloc(sizeof(Order));
    //Collect and store their ID
    order->customer_id = customer_id;
    //Select a menu item and add it to the order
    order->menu_item = PickRandomMenuItem();
    //Add the order to the restaurant
    AddOrder(bcb, order);
    
    return NULL;
}

/**
 * Thread function that represents a cook in the restaurant. A cook should:
 *  - get an order from the restaurant.
 *  - if the order is valid, it should fulfill the order, and then
 *    free the space taken by the order.
 * The cook should take orders from the restaurants until it does not
 * receive an order.
 */
void* BENSCHILLIBOWLCook(void* tid) 
{
    //Create a cook
    //Cast the thread ID to an integer
    int cook_id = (int)(long) tid;

    //Start a loop that continues until the cook receives an order
    int order_fulfillment = 0;
    while(1)
    {
        //Get an order from the restaurant  
        Order *currentOrder = GetOrder(bcb);
        //Check if the order is valid
        if(!currentOrder)
        {
            //Mark the completed series of restaurant orders
            order_series++;
            break;
        }

        //If the order is valid, the cook should fulfill the order
        order_fulfillment++;
        //Lock the mutex for the restaurant
        pthread_mutex_lock(&bcb->mutex);
        //Increment the overall amount of fulfilled orders
        bcb->orders_handled++;
        //Unlock the mutex for the restaurant
        pthread_mutex_unlock(&bcb->mutex);
        
        //Announce that the cook hahd successfully the current order
        printf("Cook #%d has successfully fulfilled order #%d: %s.\n",cook_id, currentOrder->order_number, currentOrder->menu_item);
        
        //Free the space taken by the order
        free(currentOrder);
    }
    
    //Report results of the series of orders
    printf("At the end of customer order rush #%d, Cook #%d has fulfilled %d restaurant order(s)\n", cook_id, order_series, order_fulfillment);
    

    return NULL;
}



/**
 * Runs when the program begins executing. This program should:
 *  - open the restaurant
 *  - create customers and cooks
 *  - wait for all customers and cooks to be done
 *  - close the restaurant.
 */
int main() 
{
    //Open the restaurant
    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);
   
    //Random number seed
    srand(time(NULL));
    
    //Create customers and cooks
    int i = 0;
    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];
    
    //Create the customer threads
    for(i = 0;i < NUM_CUSTOMERS; i++)
    {
        pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void*)(long)i+1);  
    }

    //Create the cook threads
    for(i = 0; i < NUM_COOKS; i++)
    {
        pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook, (void*)(long)i+1);
    }

    //Now, wait for all customers and cooks to be done
    for(i = 0; i < NUM_CUSTOMERS;i++)
    {
        pthread_join(customers[i], NULL);
    }

    pthread_cond_broadcast(&bcb->can_get_orders);

    for(i = 0; i < NUM_COOKS;i++)
    {
        pthread_join(cooks[i],NULL);
    }

    //Close the restaurant
    printf("Closing the restaurant....Posting today's report!\n");
    printf("Today's report:\n");
    CloseRestaurant(bcb);
    return 0;
}