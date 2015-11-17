/*
    Name: Eric Darr
    Project: Implmentation of malloc, realloc, calloc and free
    Notes: I still get some errors on the tester though everything seems to work ok thus far.
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PREDEFAMT 1024   //default amount of bytes the user gets whening using nu_malloc
#define STRUCTSIZE 16    //the default byte size of the struct/header
#define NUL '\0'

typedef enum { false, true } boolean; 

//the struct/headers of all the nodes
//  it's total size is 16 bytes worth
typedef struct _heap_block heap_block; 
struct _heap_block { 
    int size;   
    boolean free; 
    heap_block *next; //THIS IS 8 BYTES ON 64-bit 
};

//this is the head for the entire linked list
heap_block *head = NUL;

/*
        The inner workings of this implementation of malloc is fairly straight foward.
    Using a singly-linked list to connect the nodes to each other, through a struct/header called heap_block.
    With these headers we can navigate and "book-keep" our memory usage for the user.
    In order to do this, we implement a linked list and use the system call sbrk, requesting large amounts of memory
    from the operating system for usage.
    
    To start, the user will request size of memory that they want. 

        Suppose they want 2000 bytes of memory.
            
        1.      nu_malloc will first see if there is an already existing linked list of memory.
            Most likely this is the first time the user asked for memory, so a new chunk of memory
            is granted.
        2.      Next, nu_malloc will determine how big of a chunk of memory to give. nu_malloc assumes
            the user will ask for more later. nu_malloc has a predefined amount of memory it will always give.
            This is adjustable but for now it is 1024 bytes. Since the user asked for 2000 bytes, a multiple
            of the predefined amount will be given, which is a multiple bigger than the asked for amount.
            In this circumstance, nu_malloc would give 2048 bytes, 2000 bytes for the user to use, and
            48 bytes extra. However 16 bytes of the 48 bytes will be used for the header of the extra bytes.
            So in reality the user will only be given an extra 32 bytes of data.
        3.      Once this process has been completed the for the first time, the first node will contain a
            node that the user requested and a free node.
        4.      If the user asks for more memory from nu_malloc then the linked list is traversed searching
            for a free node for which to give the user. If a free node exists, it is examined to determine
            if it is large enough to be given to the user. If the free node's size is larger than the users
            asked for amount PLUS the size of the struct/header used which is 16, then it is given to the user
            and divided into two seperate nodes, where the leftovers are used as a free node. If there exists
            no free node has enough memory, a new node is created and attached. This new node follows the same
            rules as the node head, in which it exists as a multiple and a free is appended afterwards. 
            Of course, if a node is found that is the exact size I user wants, the free node is given.
*/

void* nu_malloc(size_t size)
{
    //ptr to be returned 
    void *ptr;

    //check for evil magic attempts?
    if((signed)size < 0)
    {
        //do nothing I suppose?
        //exit(EXIT_FAILURE);
    }
    //see if there exists a linked list to start, if not make one
    else if(head == NUL)
    {
        //if the size the user wants is less than the predefined amount
        //  give them a bit extra incase they want more
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
                else if((node->size < (size+STRUCTSIZE))) //the node doesn't have enough space for both memory and following struct
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


/*
    The implementation of nu_calloc is simply an extension of nu_malloc.
    Assuming nu_malloc is properly implemented then nu_calloc should work with the usage
    of memset.
*/

void* nu_calloc(size_t count, size_t size)
{
    void *ptr = nu_malloc(size);
    memset(ptr, 0, count);
    return ptr;
}

/*
        It is important to note that this implementation of free does not sbrk and give memory back
    to the operating system. It is assumed the user will be responsible with their memory allocation.
    
    The implentation goes as follows:

    1. The user asks to free a pointer to one of the headers.
    2. nu_free searches through the linked list for the pointer, setting
        the node as free.
    3. Next nu_free looks at adjacent nodes and tries to combine them into
        a single free node space.
    4. That's pretty much it.
*/

void nu_free(void *ptr)
{
    heap_block *node;
    node = head;

    heap_block *prevNode = NUL;

    //find node user specified
    while(node != (heap_block*)(((char *)ptr)-STRUCTSIZE) && node != NUL) 
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
            prevNode->size = prevNode->size + node->size + STRUCTSIZE;
            prevNode->next = node->next;
        }
    }
}

/*
    Implementation of nu_realloc:

    1. First, a user will specify a pointer and how much memory they would
        like out of that pointer.
    2. A traversial will begin searching for the specified pointer.
    3. Once the pointer is found the node will be examined as well as adjacent nodes.
    4. If the the free node or adjacent nodes contain enough free memory for the realloc size
        specified for the user, it is merged into a single node and given back to the user
        as an appropriate size. Any leftover free memory will be split and set as a new node
        for which the user may ask for again.
*/


void *nu_realloc(void *ptr, size_t size)
{

    heap_block *tempNode;
    heap_block *node;
    node = head;

    heap_block *prevNode = NUL;

    //find node user specified
    while(node != (heap_block*)(((char *)ptr)-STRUCTSIZE)) 
    {
        prevNode = node;
        node = node->next;
    }

    //check nodes adjacent for free space to use for node that needs to be reallocated
    //  if the total space of all adjacent nodes(including structsizes) is enough for specified
    //  amount then set pointer to beginning and readjust linked list 
    //  otherwise
    //  
    if(head != NUL)
    {    
        node->free = true;
    }

    //insert the node in the current location if there is enough space plus structsize. Splitting and reattaching
    if((node->size) > (size + STRUCTSIZE))
    {
        if(node->size == (size + STRUCTSIZE))
        {
            //return ptr to current node
            return (void *)((char *)node + STRUCTSIZE);
        }
        if(node->next != NUL)
        {
            //we are currently at the node to be resized
            //we know it's large enough for resize (we have to account for the header)
            //if there is a free node after we can reattach new split node to it
            if(node->next->free == true)
            {
                //get total size of current node and next node including headers
                int totalSize = node->size + STRUCTSIZE + node->next->size;
                //set tempnode's next to node's next
                tempNode = (heap_block *)((char *)node + (size + STRUCTSIZE));
                tempNode->next = node->next;
                tempNode->size = totalSize - size - STRUCTSIZE;
                tempNode->free = true;
                node->next = tempNode;
                node->free = false;
                node->size = size;
                return (void *)((char *)node + STRUCTSIZE);
            }
        }
        //ensure we are not attempting to read or write from a unaccessable memory
        if(prevNode != NUL)
        {
            if(prevNode->free == true)
            {
                //tempNode will be clumped with previous free node
                //stretch prevNode to node->next, erasing current node, then inserting tempNode in
                int totalSize = prevNode->size + STRUCTSIZE + node->size;
                prevNode->next = node->next;
                prevNode->free = false;
                tempNode = (heap_block *)((char *)prevNode + (STRUCTSIZE+size));
                tempNode->next = node->next;//prevNode->next->next;
                tempNode->free = true;
                prevNode->next = tempNode;
                prevNode->size = size;
                tempNode->size = totalSize - prevNode->size - STRUCTSIZE;
                return (void *)((char *)prevNode + STRUCTSIZE);
            }
        }
        //if prevNode and next node are not free
        else
        {
            //node will just be split because neither previous or next nodes are free
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

    //  if prev is null then we are at head node, if next is null then we are at end.
    if(prevNode == NUL)
    {
        if(node->next->free == true)
        {
            int tempTotalSize = node->size + STRUCTSIZE + node->next->size;
            if(tempTotalSize >= (size + STRUCTSIZE))
            {
                tempNode->free = true;
                tempNode->size = tempTotalSize - size - STRUCTSIZE;
                tempNode->next = node->next->next;
                node->free = false;
                node->size = size;
                node->next = (heap_block *)((char *)prevNode + (STRUCTSIZE + size));
                return (void *)((char *)node + STRUCTSIZE);
            }
        }  
    }
    //check to see if there is null in prev and next
    //if nearly by nodes are free and can be used
    //if here we know we are somewhere in the middle
    else if(prevNode != NUL && node->next != NUL)
    {
        int tempTotalSize;
        heap_block *tempNode = NUL;
        
        if(prevNode->free == true && node->next->free == true)
        {
            //gather total size and ask...Does it fit?
            int tempTotalSize = prevNode->size + STRUCTSIZE + node->size + STRUCTSIZE + node->next->size; 
            if(tempTotalSize >= (size + STRUCTSIZE)) 
            { 
                tempNode = (heap_block *)((char *)node + (size + STRUCTSIZE));
                tempNode->free = true;
                tempNode->size = tempTotalSize - size - STRUCTSIZE;
                tempNode->next = node->next->next;
                prevNode->free = false;
                prevNode->size = size;
                prevNode->next = (heap_block *)((char *)prevNode + (STRUCTSIZE + size));
                return (void *)((char *)prevNode + STRUCTSIZE);
            }
        }
        else if(prevNode->free == true)
        {
            tempTotalSize = prevNode->size + STRUCTSIZE + node->size;
            if(tempTotalSize >= (size + STRUCTSIZE))
            {
                tempNode = (heap_block *)((char *)node + (size + STRUCTSIZE));
                tempNode->free = true;
                tempNode->size = tempTotalSize - size - STRUCTSIZE;
                tempNode->next = node->next;
                prevNode->free = false;
                prevNode->size = size;
                prevNode->next = (heap_block *)((char *)prevNode + (STRUCTSIZE + size));
                return (void *)((char *)prevNode + STRUCTSIZE);
            }
        }
        else if(node->next->free == true)
        {
            tempTotalSize = node->size + STRUCTSIZE + node->next->size;
            if(tempTotalSize >= (size + STRUCTSIZE))
            {
                tempNode = (heap_block *)((char *)node + (size + STRUCTSIZE));
                tempNode->free = true;
                tempNode->size = tempTotalSize - size - STRUCTSIZE;
                tempNode->next = node->next->next;
                node->free = false;
                node->size = size; 
                node->next = tempNode;
                return (void *)((char *)node + STRUCTSIZE);
            }
        }
    }
    // if here, we are at last node, do only checks for last node
    else if(node->next == NUL)
    {
        if(prevNode->free == true)
        {
            int tempTotalSize = prevNode->size + STRUCTSIZE + node->size;
            if(tempTotalSize >= (size + STRUCTSIZE))
            {
                tempNode = (heap_block *)((char *)node + (size + STRUCTSIZE));
                tempNode->free = true;
                tempNode->size = tempTotalSize - size - STRUCTSIZE;
                tempNode->next = node->next;
                prevNode->free = false;
                prevNode->size = size;
                prevNode->next = (heap_block *)((char *)prevNode + (STRUCTSIZE + size));
                return (void *)((char *)node + STRUCTSIZE);
            }
        }
    }
    //give them a free node at the end of the linked list
    else
    {
        //go to the last node
        while(node->next != NUL)
        {
            node = node->next;
        }
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

