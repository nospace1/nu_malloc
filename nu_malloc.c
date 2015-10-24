#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> 
#include <string.h>

#define PREDEFAMT 1024   //default amount of bytes the user gets whening using nu_malloc
#define STRUCTSIZE 16    //the default byte size of the struct/header
#define NUL '\0'

typedef enum { false, true } boolean; 

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
    void *nu_calloc(size_t, size_t);
    void nu_free(void *);
    void *nu_realloc(void *, size_t);

    size_t testsize = 8000;
    size_t testsize2 = 500;
    size_t testsize3 = 2048;
    size_t testsize4 = 316;
    size_t testsize5 = 500;
    size_t ctestsize4 = 500;
 
    char *letters;
    char *letters2;
    char *letters3;
    char *letters4;

    heap_block *node;
    node = head;

    void *testPtr = nu_calloc(30,ctestsize4);
    void *testPtr6 = nu_malloc(testsize);
    void *testPtr2 = nu_malloc(testsize2);
    void *testPtr3 = nu_malloc(testsize3);
    void *testPtr4 = nu_malloc(testsize4);
    void *testPtr5 = nu_malloc(testsize5);

    node = head;
    printf("----------------\n");
     while(node != NUL)
    {
        printf("Node starts at %p. Free: %d. Size: %d. Next: %p\n", node, node->free, node->size, node->next);
        node = node->next;
    }
    

    nu_free(testPtr6);
    nu_free(testPtr5);
    nu_free(testPtr4);
    nu_free(testPtr3);
    nu_free(testPtr2);
    nu_free(testPtr);
 
    node = head;
    printf("----------------\n");
     while(node != NUL)
    {
        printf("Node starts at %p. Free: %d. Size: %d. Next: %p\n", node, node->free, node->size, node->next);
        node = node->next;
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
            head->next->size = n-(size+STRUCTSIZE); 
        }
        else
        {
            head = sbrk(PREDEFAMT+STRUCTSIZE);        
            head->next = (heap_block *)(((char *)head)+(STRUCTSIZE+size));
            head->next->size = PREDEFAMT - (size+STRUCTSIZE);
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
        while((node->next != NUL)) //if next node is null, then the current node is the last node
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
                    node->free = false;
                    ptr = (void *)(((char *)node)+STRUCTSIZE);
                    return ptr;
                } 
                else if((node->size < (size+STRUCTSIZE))) // && (node->next != NUL)) // the node doesn't have enough space for both memory and following struct
                {
                    node = node->next; //progress to next node
                }
                else if(node->size > (size+STRUCTSIZE)) //if there is space for the struct and memory
                {

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
        //I'm at the last node, check to see if it's free
        if(node->free == true)
        {
            if(node->size == size)
            {
                ptr = (void *)(((char *)node)+STRUCTSIZE);
                return ptr;
            } 
            else if(node->size < (size+STRUCTSIZE)) //at last node, and it's not enough room
            {
                if(size >= PREDEFAMT)
                {
                    int n = 0;
                    n = ((size/PREDEFAMT)+1)*PREDEFAMT;
                    node->next = sbrk(n+STRUCTSIZE);
                    node = node->next; // move foward, to last section/node so I can split it easier
                    node->size = size;
                    node->free = false;
                    node->next = (heap_block *)(((char *)node)+(STRUCTSIZE+size)); 
                    node->next->size = n - (size+STRUCTSIZE);
                    node->next->free = true;
                    ptr = (void *)(((char *)node)+STRUCTSIZE);
                    node->next->next = NUL;
                    return ptr;
                }
                else
                {
                    node->next = sbrk(PREDEFAMT+STRUCTSIZE);
                    node = node->next;
                    node->size = size;
                    node->free = false;
                    node->next = (heap_block *)(((char *)node)+(STRUCTSIZE+size));
                    node->next->size = PREDEFAMT - (size+STRUCTSIZE);
                    node->next->free = true;
                    ptr = (void *)(((char *)node)+STRUCTSIZE);
                    node->next->next = NUL;
                    return ptr; 
                } 
            }
            else if(node->size > (size+STRUCTSIZE)) //if there is space for the struct and memory
            {
                //insert a new node
                heap_block *tempNode;
                tempNode = (heap_block *)(((char *)node) + (STRUCTSIZE+size));
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
        else
        {
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
                node->next = sbrk(PREDEFAMT+STRUCTSIZE);        
                node->next->next = (heap_block *)(((char *)head)+(STRUCTSIZE+size));
                node->next->next->size = PREDEFAMT - size;
            }        
            // move to node to return, second to last node
            node = node->next;
            node->free = false;
            node->next->free = true;

            //the node after last node should be NULL
            node->next->next = NUL;
            ptr = (void *)(((char *)node)+STRUCTSIZE);
            return ptr;
        }
    }
}

void* nu_calloc(size_t count, size_t size)
{
    void *ptr = nu_malloc(size);
    memset(ptr, 0, count);
    return ptr;
}

void nu_free(void *ptr)
{
    heap_block *node;
    node = head;

    heap_block *prevNode = NUL;

    //find node user specified
    while(node != (heap_block*)(((char *)ptr)-STRUCTSIZE)) 
    {
        prevNode = node;
        node = node->next;
    }
    if(head != NUL)
    {    
        node->free = true;
    } 
    //if here we know we are somewhere in the middle
    if(prevNode != NUL && node->next != NUL)
    {
        if(prevNode->free == true && node->next->free == true)
        {
            prevNode->size = prevNode->size + STRUCTSIZE + node->size + STRUCTSIZE + node->next->size;
            prevNode->next = node->next->next;
        }
        else if(prevNode->free == true)
        {
            prevNode->size = prevNode->size + node->size + STRUCTSIZE;
            prevNode->next = node->next;
        }
        else if(node->next->free == true)
        {
            node->size = node->size + STRUCTSIZE + node->next->size;
            node->next = node->next->next;
        }
    }
    //check to see if there is null in prev and next
    //  if prev is null then we are at head node, if next is null then we are at end.
    if(prevNode == NUL)
    {
        if(node->next->free == true)
        {
            node->size = node->size + STRUCTSIZE + node->next->size;
            node->next = node->next->next;
        }  

    }
    // if here, we are at last node, do only checks for last node
    else if(node->next == NUL)
    {
        if(prevNode->free == true)
        {
            //prevNode->size = (int)(((char *)node->next - (char *)prevNode)-STRUCTSIZE);
            prevNode->size = prevNode->size + node->size + STRUCTSIZE;
            prevNode->next = node->next;
        }
    }
}


