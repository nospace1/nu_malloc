#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define PREDEFAMT 1024   //default amount of bytes the user gets whening using nu_malloc
#define STRUCTSIZE 16    //the default byte size of the struct/header
#define NUL '\0'

typedef enum 
{ false, true } boolean;

/* 
void* nu_malloc(size_t size);
First give the user the default amount of bytes with sbrk, including the header.
Next embed the struct at the start of the free space created. It is a header to the
free space.
    
        typedef struct _heap_block {
        ???int canary;      //likely not implemented
        int size;           //bytes requested by user
        boolean free;       //whether the following data is being used
        heap_block* next;   //a pointer the next heap header
        } heap_block;

UNDER THE CIRCUMSTANCE THAT A USER WISHES TO ENTER MEMORY 

1. First check to see if head node is null or not. If it is null we know that there is
    no linked list currently. We need to make one while giving the user memory.
    
    IIIIFFFFF
    - if the linked list is not null, then we need to examine each header for possible
        free amounts of memory. If there is a segment which is less than the amount
        asked for, give them a pointer to it, while creating a new header and node
        within the linked list.
IIIIFFFFF
    - if the head is null and the user requests memory outside of the predefined 
        amount given (512) then we need to sbrk them a multiple of our predefined 
        amount, that is greater than what is requested. This also requires the 
        addition of the size of 2 structs worth, one for the current and next memory segment.
            - this is followed by step 2, attaching or creating a new linked list

    IIIIFFFFF
    - if the user requests memory smaller than the predefined amount (512), then


tl;dr - user requests memory
            1. check if head is null
            2. if not null check for free memory space in linked list.
            3. if greater than predefined amount, give multiple of pre amount.
            4. 
 

2 .See if there is a header of memory that is FREE AND IS THE EXACT AMOUNT OF MEMORY 
    THE USER REQUESTED.
        - simply give them a pointer to that memory location. 


*/
typedef struct _heap_block heap_block;
struct _heap_block {
    int size;   
    boolean free;
    heap_block *next; //THIS IS 8 BYTES ON 64-bit
};

heap_block *head = NUL;

int main(void)
{
    void *nu_malloc(size_t);
    
    size_t testsize = 1200;
    size_t testsize2 = 300;
    size_t testsize3 = 32;
 
    void *testPtr = nu_malloc(testsize);
    char *letters;
    char *letters2;
    char *letters3;

    letters = (char *)testPtr;
    void *testPtr2 = nu_malloc(testsize2);
    letters2 = (char *)testPtr2;
    void *testPtr3 = nu_malloc(testsize3);
    letters3 = (char *) testPtr3;
 
    int i = 0;
    for(i = 0; i < testsize; i++)
    {
        *(letters+i) = 'A'; 
        if(i < testsize2)
            *(letters2+i) = 'B'; 
        if(i < testsize3)
            *(letters3+i) = 'C'; 
    }
    return 0;
}

void* nu_malloc(size_t size)
{
    //ptr to be returned 
    void *ptr;

    //check for evil magic attempts
    if((signed)size < 0)
    {
        //printf("NO NEGATIVE NUMBERS FUCKER");
        exit(EXIT_FAILURE);
        //return (void *)-1;
    }
    else if(head == NUL)
    {
        if(size >= PREDEFAMT)
        {
            //sbrk multiple of predefamt until greater than
            int n = 0;
            n = ((size/PREDEFAMT)+1)*PREDEFAMT;
            head = sbrk(n+STRUCTSIZE);
            head->next = (heap_block *)(((char *)head)+(STRUCTSIZE+size));
            head->next->size = n-size; 
        }
        else
        {
            head = sbrk(size+STRUCTSIZE);        
            head->next = (heap_block *)(((char *)head)+(STRUCTSIZE+size));
            head->next->size = PREDEFAMT - size;
        }
        //cut into two pieces
        head->size = size;
        head->free = false;
        head->next->free = true;
        //set the node after the final NUL
        head->next->next = NUL;

        ptr = (void *)(((char *)head)+STRUCTSIZE);
        return ptr;
    }
    else //head is not null
    {
        //we must search the list
        //create new node for searching
        heap_block *node;
        node = head;
        
        //start traversial
        while((node->next != NUL) || (node->free == true)) //if next node is null, then the current node is the last node
        {
            if(node->free == false)
            {
                //keep looking
                node = node->next;
            }
            else if(node->free == true)
            {
               if(node->size == size)
                {
                    ptr = (void *)(((char *)node)+STRUCTSIZE);
                    return ptr;
                } 
                else if(node->size < (size+STRUCTSIZE)) // the node doesn't have enough space for both memory and following struct
                {
                    node = node->next; //progress to next node
                }
                else if(node->size > (size+STRUCTSIZE)) //if there is space for the struct and memory
                {
                    //insert a new node
                    heap_block *tempNode;
                    tempNode = (heap_block *)((char *)node + (STRUCTSIZE+size));
                    tempNode->next = node->next;
                    node->next = tempNode;
                    node->next->size = node->size - (int)(size+STRUCTSIZE);
                    node->size = size;
                    node->free = false;
                    node->next->free = true;
                    ptr = (void *)(((char *)node)+STRUCTSIZE);
                    return ptr;
                }
            }
        }
        //If I got to this point there was no free node that could hold the data
        //currently at last node
        //attach to end of list
        if(size >= PREDEFAMT)
        {
            //sbrk multiple of predefamt until greater than
            int n = 0;
            n = ((size/PREDEFAMT)+1)*PREDEFAMT;
            node->next  = sbrk(n+STRUCTSIZE);
            node->next->next = (heap_block *)(((char *)head)+(STRUCTSIZE+size));
            node->next->next->size = n-size; 
        }
        else
        {
            node->next = sbrk(size+STRUCTSIZE);        
            node->next->next = (heap_block *)(((char *)head)+(STRUCTSIZE+size));
            node->next->next->size = PREDEFAMT - size;
        }
        // move to node to return, second to last node
        node = node->next;
        //the node after last node should be NULL
        node->next->next = NUL;
        ptr = (void *)(((char *)node)+STRUCTSIZE);
    }
}




