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
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdcircularbuffer: " fmt, ## args)
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
 *      character index if all buffer strings were concatenated end to end (ABSOLUTE POSITION) FIND the 
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 THIS GIVES YOU A CHAR OFFSET. YOU RETURN THE BUFFER ENTRY FOR THE START OF READING FROM THIS OFFSET, AS WELL AS THE OFFSET WITHIN THE CHAR BUFFER OF THAT ENTRY
 PRESUMABLY YOU THEN READ UP TO THE END OF THAT BUFFER
THIS RETURNS THE ENTRY and POSITION of CHAR OFFSETer
READ THE BUFFERS STARTING AT WHEREVER OUTPOINTER IS UNTIL YOU GET TO THE ENTRY THAT THIS RETURNS, AND WHEN YOU GET TO THIS ONE READ UP TO THE OFFSET BYTE

*/
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
	size_t curr_searched_buffer_length=0;
	size_t bufferinpos=buffer->in_offs;
	size_t bufferoutpos=buffer->out_offs;
	bool first_run=true;
	PDEBUG("CHAR_OFFSET=%ld",char_offset);
	while (bufferinpos != bufferoutpos || first_run){
		if (first_run){
			//printf("FIRST RUN!\n");
		}
		first_run=false;
		PDEBUG("considering buffer for entry %ld",bufferoutpos);
		
		
		
		if (curr_searched_buffer_length + buffer->entry[bufferoutpos].size >= char_offset){
			curr_searched_buffer_length = curr_searched_buffer_length + buffer->entry[bufferoutpos].size;
			PDEBUG("CURR_SEARCHED_BUFFER_LENGTH=%ld",curr_searched_buffer_length);
			PDEBUG("returning buffer entry: %ld\n",bufferoutpos);
			//found_location=true;
			size_t number_of_bytes_considered_up_to_first_byte_in_this_entry_char_buffer = curr_searched_buffer_length - buffer->entry[bufferoutpos].size;
			//total - everything we looked for before we got to this one
			size_t offset_within_this_entry_buffer = char_offset -  number_of_bytes_considered_up_to_first_byte_in_this_entry_char_buffer;
			PDEBUG("OFFSET WITHIN THIS ENTRY BUFFER:%ld",offset_within_this_entry_buffer);
			* entry_offset_byte_rtn = offset_within_this_entry_buffer;
			return(&buffer->entry[bufferoutpos]);
		}
		curr_searched_buffer_length += buffer->entry[bufferoutpos].size;
		if ((bufferoutpos + 1) != (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)){
			bufferoutpos++;	
			
		}	
		else{
			bufferoutpos=0;
		}
	}
	PDEBUG("returning null,bufferinpos:%ld == bufferoutpos:%ld\n",bufferinpos,bufferoutpos);
    return NULL;
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
		if (buffer->in_offs == buffer->out_offs && buffer->full){
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

size_t get_buffer_size_of_current_entry(struct aesd_buffer_entry* entry_to_check){
	PDEBUG("*** get buffer size of %p, returning %ld",entry_to_check,entry_to_check->size);
	return(entry_to_check->size);
}

size_t get_index_of_current_entry(struct aesd_buffer_entry* entry_to_check,struct aesd_circular_buffer * circle_buf){
	for (size_t position=0;position < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; position++){
		if (entry_to_check == &(circle_buf->entry[position])) {return(position);}
	}
	PDEBUG("COULD NOT FIND A POSITION IN THE CIRCLE BUFFER FOR ENTRY");
	return(-1);
}

struct aesd_buffer_entry *get_next_entry_in_buffer(struct aesd_buffer_entry* entry_to_check,struct aesd_circular_buffer * circle_buf){
	size_t current_index = get_index_of_current_entry(entry_to_check,circle_buf);
	size_t next_index = -1;
	
	next_index=current_index+1;
	if (next_index == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
			next_index=0;
		}
	return(&(circle_buf->entry[next_index]));			
}
size_t current_outbuffer_position=0;
size_t original_outbuffer_position=0;