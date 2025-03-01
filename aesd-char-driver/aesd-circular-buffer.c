/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"
#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end (ABSOLUTE POSITION)
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
*/
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
	size_t curr_searched_buffer_length=0;
	//printf("runing funct 1\n");
	//printf("char_offset: %ld\n",char_offset);
	//size_t offset_of_the_last_entry_buffer_considered=0;
	//char * ptr_to_beginning_of_last_entry_buffer_considered=NULL;
	//uint8_t * myptr = &(buffer->in_offs);
	//uint8_t full_buffer_size=AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	//bool found_location=false;
	//if (buffer->full){myptr = &full_buffer_size;};
	////printf("buffer output position is %d\n",buffer->out_offs);
	//if (buffer->in_offs == buffer->out_offs){
	//	//printf("returning null because inoffs == outoffs\n");
	//	return NULL;}//there is nothing to parse
	size_t bufferinpos=buffer->in_offs;
	size_t bufferoutpos=buffer->out_offs;
	bool first_run=true;
	while (bufferinpos != bufferoutpos || first_run){
		if (first_run){
			//printf("FIRST RUN!\n");
		}
		first_run=false;
		//printf("considering buffer for entry %ld, bufferoutpos=%ld\n",bufferinpos,bufferoutpos);
		curr_searched_buffer_length += buffer->entry[bufferinpos].size;
		if (curr_searched_buffer_length > char_offset){
			//printf("returning buffer entry: %ld\n",bufferinpos);
			//found_location=true;
			size_t number_of_bytes_considered_up_to_first_byte_in_the_entry_char_buffer=curr_searched_buffer_length - buffer->entry[bufferinpos].size;
			size_t offset_within_this_entry_buffer = char_offset -  number_of_bytes_considered_up_to_first_byte_in_the_entry_char_buffer;
			* entry_offset_byte_rtn = offset_within_this_entry_buffer;
			return(&buffer->entry[bufferinpos]);
		}
			
		
		if ((bufferinpos + 1) != (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)){
			bufferinpos++;	
			//printf("funct1 incrementing buffinpos to %ld\n",bufferinpos);
			
		}	
		else{
			bufferinpos=0;
		}
	}
	//printf("returning null,bufferinpos:%ld == bufferoutpos:%ld\n",bufferinpos,bufferoutpos);
    return NULL;
		
	/*	
	
	//while  (size_t i=0; i< *myptr; i++){
	//	curr_searched_buffer_length += buffer->entry[i].size;
	//	////printf("curr_searched_buffer_length:%ld buffer->entry[i].size: %ld i=%ld\n",curr_searched_buffer_length,buffer->entry[i].size,i);
		if (curr_searched_buffer_length > char_offset){
			//printf("returning buffer entry: %ld\n",i);
			//now get the offset from the beginning of the entry
			//const char * address_of_first_byte_in_the_entry_char_buffer = buffer->entry[i].buffptr;
			size_t number_of_bytes_considered_up_to_first_byte_in_the_entry_char_buffer=curr_searched_buffer_length - buffer->entry[i].size;
			
			size_t offset_within_this_entry_buffer = char_offset -  number_of_bytes_considered_up_to_first_byte_in_the_entry_char_buffer;
			////printf("offset_within_this_entry_buffer=%ld\n",offset_within_this_entry_buffer);
			////printf("address_of_first_byte_in_the_entry_char_buffer:%p\n", address_of_first_byte_in_the_entry_char_buffer);
			////printf("offset_within_this_entry_buffer:%ld\n",offset_within_this_entry_buffer);
			//entry_offset_byte_rtn = (size_t *)( ((char *) address_of_first_byte_in_the_entry_char_buffer) + offset_within_this_entry_buffer);
			* entry_offset_byte_rtn = offset_within_this_entry_buffer;
			////printf("entry_offset_byte_rtn %p\n",entry_offset_byte_rtn);
			//char * position_of_the_selected_byte_in_the_current_buffer= 
			//size_t entry_offset_byte_rtn=
			return(&buffer->entry[i]);
		}
		
	}
	*/	
	
	
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)

{
		bool need_freed=false;
		size_t offsetter = buffer->in_offs;
		struct aesd_circular_buffer * dummy=buffer;
		char * oldbufferptr=buffer->entry[buffer->in_offs].buffptr;
		
		if (oldbufferptr != NULL){
			PDEBUG("_____________THERE IS AN EXISTING ENTRY BUFFER AT: %p, SIZE: %ld",oldbufferptr,buffer->entry[buffer->in_offs].size);
			PDEBUG("_____________*ACTIVATING NEEDFREED MODE");
			need_freed=true;
		}
		
		PDEBUG("_____________ADDRESS OF ENTRY CHAR BUFFER BEING ADDED: %p",add_entry->buffptr);
		PDEBUG("_____________SIZE OF ENTRY CHAR BUFFER BEING ADDED: %ld",add_entry->size);
		PDEBUG("_____________CHAR AT POS 1 of INCOMING BUFFER: %c",add_entry->buffptr[1]);
		
		
		buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
		buffer->entry[buffer->in_offs].size =  add_entry->size;
		
		PDEBUG("____________VALUE OF CURRENT ENTRY AFTER ADDING=%p",buffer->entry[buffer->in_offs].buffptr);
		if (buffer->in_offs == buffer->out_offs){
			buffer->out_offs+=1;
			if (buffer->out_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)){
				buffer->out_offs=0;
			}
		}
		
		buffer->in_offs++;
		
		if (buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
				buffer->in_offs=0;
				buffer->full=true;
				//printf("buffer is full, set in_offs to zero\n");
				
		}
		
		//if the buffer is full and the in_offset == the outoffset, we will overwrite something so advance outoffset. If that turns it to greater than the size of the buffer, 
		if (need_freed){
			
			return(oldbufferptr);
		}
		return(NULL);
		
	
		
	
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
