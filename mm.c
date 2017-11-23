/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

/////////////////////  Current Version ////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

//#define DEBUG_1 1

#ifdef DEBUG_1
# define DEBUG_PRINT1(x) printf x
#else
# define DEBUG_PRINT1(x) do {} while (0)
#endif

#define DEBUG_2 0
#ifdef DEBUG_2
# define DEBUG_MARK(p,w) mark_payload(p,w) 
#else
# define DEBUG_MARK(p) do {} while (0)
#endif

// My structures

typedef struct {
 size_t size;
 char allocated;
} block_header;

//typedef struct chunk_header chunk_header;

typedef struct{
    size_t chunk_size;
    char chunk_allocated;
    //chunk_hdr* next_chunk;
}chunk_header;

typedef struct{
    chunk_header* next_chunk;    
    char chunk_allocated;    
    
}chunk_footer;

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))


// MY DECLARATIONS
#define CHUNK_SIZE (1 << 14)
#define CHUNK_ALIGN(size) (((size)+(CHUNK_SIZE-1)) & ~(CHUNK_SIZE-1))
#define GET_CHUNK_SIZE(p) ((chunk_header *)(p))->chunk_size
#define GET_CHUNK_ALLOC(p) (((chunk_header *)(p))->chunk_allocated)
#define GET_CHUNK_FTR_PTR(p)  ((chunk_footer*)(p))->next_chunk
#define CHUNK_OVERHEAD (sizeof(chunk_header) * 3)
#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated
#define BYT_TO_BLK(bytes) (bytes >> 1)
#define BLK_TO_BYT(blks) (blks << 1)
#define SZ_OF_BLKS (sizeof(block_header)) 


#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define GO_TO_FTR(bp) ( HDRP(HDRP(NEXT_BLKP(bp))) )
#define OVERHEAD (sizeof(block_header) * 2)


static void set_allocated(void *bp, size_t req_size);
static int check_aligned(void* bp, void* other);
static void mark_payload(block_header* p, int hdr_ftr);
static void setACE(int* ace, int al_unal);
static void setBAC(int* bac, int al_unal);
static void *mm_malloc_dbg(size_t size);
static void set_allocated_dbg(void *bp, size_t req_size_including_overhead);
static unsigned long calc_offset(void* new_ptr);
static void* set_new_chunk(size_t new_size);
static void* extend(size_t new_size);
static void* extend_dbg(size_t new_size);
static char* print_block_info(void* bp);


void *current_avail = NULL;
int current_avail_size = 0;
void *first_bp;
int debug_on = 1;
void* first_allocation_ptr;
int block_count = 0;
int allocated_block_count = 0;
int unalloc_block_count = 0;
int chunk_count = 0;
int num_calls = 0;
int malloc_req = 1;


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init() {
          //printf("\nsizeof chunk_ftr = %d\n\n", sizeof(chunk_footer));
         printf("\n\n");  
         size_t initial_num_pages = 4;                              
         size_t num_aligned_pages_in_bytes = PAGE_ALIGN(initial_num_pages);  
         size_t num_bytes = num_aligned_pages_in_bytes;    
         size_t num_blocks = num_bytes >> 1;                        
        
        //  prolog chunk hdr, two chunk hdrs
         chunk_header* first_chunk = mem_map(num_aligned_pages_in_bytes);    
         int fc_off = first_chunk - first_chunk;
         GET_CHUNK_SIZE(first_chunk) = BLK_TO_BYT(2);                           
         GET_CHUNK_ALLOC(first_chunk) = 1;                          
         chunk_header* prolog_chunk_ftr = ((char*)first_chunk + (sizeof(chunk_header))); 
         unsigned long cftr_off = (void*)prolog_chunk_ftr - (void*)first_chunk;
         
         // set end of chunk
         chunk_footer* end_of_chunk = (((char*) first_chunk) +  num_bytes - sizeof(chunk_footer));
         GET_CHUNK_FTR_PTR(end_of_chunk) = NULL;
         GET_CHUNK_SIZE(end_of_chunk) = BLK_TO_BYT(0); 
         unsigned long end_of_chunk_off = (void*)end_of_chunk - (void*)first_chunk;


         block_header* first_block_hdr = ((char*)prolog_chunk_ftr + (sizeof(chunk_footer))); 
         GET_ALLOC(first_block_hdr) = 0;
         unsigned long f_blk_hdr_off = (void*)first_block_hdr - (void*)first_chunk;
//         DEBUG_MARK(first_block_hdr, 0);
         setBAC(first_block_hdr, 0);

         size_t first_block_size_bytes = num_bytes - (CHUNK_OVERHEAD);
         size_t first_block_size_blks = BYT_TO_BLK(first_block_size_bytes);
         GET_SIZE(first_block_hdr) = first_block_size_bytes;
        
         unsigned long size_of_block_in_bytes = GET_SIZE(first_block_hdr);
         unsigned long first_blk_ftr_offset_est = f_blk_hdr_off + size_of_block_in_bytes - SZ_OF_BLKS;
         //void* f_blk_ftr_ptr = HDRP(NEXT_BLKP(first_block_hdr));
         void* f_blk_ftr_ptr = GO_TO_FTR((void*)first_block_hdr + sizeof(block_header));
         block_header* first_block_ftr = ((void*)first_chunk) + first_blk_ftr_offset_est;
         unsigned long f_blk_ftr_off = (void*)first_block_ftr - (void*)first_chunk;                  
         GET_SIZE(first_block_ftr) = GET_SIZE(first_block_hdr);
         GET_ALLOC(first_block_ftr) = 0;

         first_bp = ((void*)first_block_hdr + sizeof(block_header));
         first_allocation_ptr = first_chunk;
         unsigned long bp_off = (void*)first_bp - (void*)first_chunk;
         block_count++;
         unalloc_block_count++;
         chunk_count++;

         if(!  (  check_aligned((void*)first_chunk, (void*)first_chunk) && 
                  check_aligned((void*)first_chunk, (void*)prolog_chunk_ftr) && 
                  check_aligned((void*)first_chunk, (void*)end_of_chunk) && 
                  check_aligned((void*)first_chunk, (void*)first_block_hdr) && 
                  check_aligned((void*)first_chunk, (void*)first_block_ftr) ) )
           DEBUG_PRINT1(("NOT ALIGNED\n"));

         print_block_info(first_bp);
          
         return 0;
}

static void* set_new_chunk(size_t new_size)
{
        
        //  prolog chunk hdr, two chunk hdrs
         size_t num_bytes = new_size;
         chunk_header* first_chunk = mem_map(num_bytes);    
         int fc_off = first_chunk - first_chunk;
         GET_CHUNK_SIZE(first_chunk) = BLK_TO_BYT(2);                           
         GET_CHUNK_ALLOC(first_chunk) = 1;                          
         chunk_header* prolog_chunk_ftr = ((char*)first_chunk + (sizeof(chunk_header))); 
         unsigned long cftr_off = (void*)prolog_chunk_ftr - (void*)first_chunk;
         
         // set end of chunk
         chunk_footer* end_of_chunk = (((char*) first_chunk) +  num_bytes - sizeof(chunk_footer));
         GET_CHUNK_FTR_PTR(end_of_chunk) = NULL ;
         GET_CHUNK_SIZE(end_of_chunk) = BLK_TO_BYT(0); 
         unsigned long end_of_chunk_off = (void*)end_of_chunk - (void*)first_chunk;


         block_header* first_block_hdr = ((char*)prolog_chunk_ftr + (sizeof(chunk_footer))); 
         GET_ALLOC(first_block_hdr) = 0;
         unsigned long f_blk_hdr_off = (void*)first_block_hdr - (void*)first_chunk;
         setBAC(first_block_hdr, 0);

         size_t first_block_size_bytes = num_bytes - (CHUNK_OVERHEAD);
         size_t first_block_size_blks = BYT_TO_BLK(first_block_size_bytes);
         GET_SIZE(first_block_hdr) = first_block_size_bytes;
        
         unsigned long size_of_block_in_bytes = GET_SIZE(first_block_hdr);
         unsigned long first_blk_ftr_offset_est = f_blk_hdr_off + size_of_block_in_bytes - SZ_OF_BLKS;
         //void* f_blk_ftr_ptr = HDRP(NEXT_BLKP(first_block_hdr));
         void* f_blk_ftr_ptr = GO_TO_FTR((void*)first_block_hdr + sizeof(block_header));
         block_header* first_block_ftr = ((void*)first_chunk) + first_blk_ftr_offset_est;
         unsigned long f_blk_ftr_off = (void*)first_block_ftr - (void*)first_chunk;                  
         GET_SIZE(first_block_ftr) = GET_SIZE(first_block_hdr);
         GET_ALLOC(first_block_ftr) = 0;

         char* new_bp = ((void*)first_block_hdr + sizeof(block_header));
         unsigned long bp_off = (void*)new_bp - (void*)first_chunk;

         if(!  (  check_aligned((void*)first_chunk, (void*)first_chunk) && 
                  check_aligned((void*)first_chunk, (void*)prolog_chunk_ftr) && 
                  check_aligned((void*)first_chunk, (void*)end_of_chunk) && 
                  check_aligned((void*)first_chunk, (void*)first_block_hdr) && 
                  check_aligned((void*)first_chunk, (void*)first_block_ftr) ) )
           DEBUG_PRINT1(("NOT ALIGNED\n"));
         unalloc_block_count++;
         block_count++;
         chunk_count++;
         num_calls++;

         print_block_info(first_bp);         
         return new_bp;

}

// Arbitrary comment


void *mm_malloc(size_t size) {
  if(debug_on == 1)
    return mm_malloc_dbg(size);

  int new_size = ALIGN(size + OVERHEAD);
  void *bp = first_bp;
  while (GET_SIZE(HDRP(bp)) != 0) {
    if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size)) {
      set_allocated(bp, new_size);
      return bp; 
    }
      bp = NEXT_BLKP(bp);
  }

  extend(new_size);
  set_allocated(bp, new_size);
         num_calls++;  
  return bp;
}

static char* print_block_info(void* bp)
{
  if(!debug_on)
  return;
  
  printf("////////////////////////////////////////////////////////////////////////////////\n\n");

  int i = 1;
  char in;

  while (GET_SIZE(HDRP(bp)) != 0) {
      printf("BLOCK HDR:\t ptr:%p\t\t\toff: %lu\t\t\t\tsize: %d\t\t\talloc: %d\t\t\tNUMBER IN CHUNK: %d\n", HDRP(bp), calc_offset(HDRP(bp)),GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), i);
      block_header* ftr = GO_TO_FTR(bp);

      printf("BLOCK FTR:\t ptr:%p\t\t\toff: %lu\t\t\tsize: %d\t\t\talloc: %d\t\t\tNUMBER IN CHUNK: %d\n", ftr, calc_offset(ftr),GET_SIZE(ftr), GET_ALLOC(ftr), i++);

      printf("DIST IN BYTES HDR -> FTR: %d\n\n\n", calc_offset(ftr) - calc_offset(HDRP(bp)) );

      bp = NEXT_BLKP(bp);
  }

  scanf("%c",&in);

}

/*
  ///////////// DEBUG ONLY ///////////////////////
*/

static void* mm_malloc_dbg(size_t size) {

  printf("Malloc REQ: %d\t\t\t FOR SIZE: %d\n", malloc_req++, size);
  int new_size = ALIGN(size + OVERHEAD);
  void *bp = first_bp;
  while (GET_SIZE(HDRP(bp)) != 0) {
      size_t size_block_dbg = GET_SIZE(HDRP(bp));
      size_t is_alloc = GET_ALLOC(HDRP(bp));
      int diff_hdr_ftr = GO_TO_FTR(bp) - HDRP(bp);
      unsigned long curr_blk_off = (void*)HDRP(bp) - (void*)first_allocation_ptr;
      unsigned long curr_blk_ftr_off = (void*)GO_TO_FTR(bp) - (void*)first_allocation_ptr;
    if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size)) {

      set_allocated_dbg(bp, new_size);
      num_calls++;        
      return bp; 
    }
      bp = NEXT_BLKP(bp);
  }

  chunk_footer* chk_ftr = (void*)bp - sizeof(chunk_footer);
  void* new_bp = extend_dbg(new_size);
  GET_CHUNK_FTR_PTR((chk_ftr)) = new_bp;
  set_allocated_dbg(new_bp, new_size);
  num_calls++;    
  print_block_info(first_bp);  
  return new_bp;
}
 //arb arb
/////////////// DEBUG ONLY  /////////////////////////

static void* extend(size_t new_size) {
 size_t chunk_size = CHUNK_ALIGN(new_size);
 char *bp = set_new_chunk(chunk_size);
 //GET_SIZE(HDRP(bp)) = chunk_size;
 //GET_ALLOC(HDRP(bp)) = 0;
 //GET_SIZE(HDRP(NEXT_BLKP(bp))) = 0;
 //GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 1;
 num_calls++;   
 chunk_count++; 
 return bp;
}

static void* extend_dbg(size_t new_size) {
 size_t chunk_size = CHUNK_ALIGN(new_size);
 char *bp = set_new_chunk(chunk_size);
 num_calls++;   
 chunk_count++;
 return bp;
}

static void set_allocated_dbg(void *bp, size_t req_size_including_overhead) {
 size_t extra_size = GET_SIZE(HDRP(bp)) - req_size_including_overhead;
 printf("SETTING ALLOC OF SIZE: %d\t\t\t EXTRA SIZE: %d\n", req_size_including_overhead, extra_size);  
 if (extra_size > ALIGN(1 + OVERHEAD)) {

  printf("SPLITTING BLOCK\n");
  unsigned long new_blk_ftr_off = (void*)GO_TO_FTR(bp) - (void*)bp; // DEBUG ONLY
  GET_SIZE(HDRP(bp)) = req_size_including_overhead; // SET HDR
  GET_SIZE(GO_TO_FTR(bp)) = req_size_including_overhead; // SET FTR
  //DEBUG_MARK(bp, 1);

  void* blk_ftr_ptr = GO_TO_FTR(bp);
  void* unalloc_bp = blk_ftr_ptr + sizeof(block_header) + sizeof(block_header);
  block_header* unalloc_blk_hdr = HDRP(unalloc_bp);
  unsigned long unalloc_blk_hdr_off = (void*)unalloc_blk_hdr - (void*)first_allocation_ptr;
  size_t unalloc_blk_size = extra_size;  // CALC SIZE OF UNALLOC BLOCK
  GET_SIZE(HDRP(unalloc_bp)) = extra_size; // 

  block_header* unalloc_ftr = GO_TO_FTR(unalloc_bp);
  unsigned long unalloc_ftr_off = (void*)unalloc_ftr - (void*)first_allocation_ptr;  
  GET_SIZE(GO_TO_FTR(unalloc_bp)) = extra_size; // SET FTR
  GET_ALLOC(HDRP(unalloc_bp)) = 0;
  GET_ALLOC(GO_TO_FTR(unalloc_bp)) = 0; // SET FTR
  //DEBUG_MARK((void*)unalloc_bp,0);
  print_block_info(first_bp);
  unalloc_block_count++;
 }
 allocated_block_count++;
 num_calls++;   
 GET_ALLOC(HDRP(bp)) = 1;
 GET_ALLOC(GO_TO_FTR(bp)) = 1;

 print_block_info(first_bp);
}

static void set_allocated(void *bp, size_t req_size_including_overhead) {

 size_t extra_size = GET_SIZE(HDRP(bp)) - req_size_including_overhead;
 printf("SETTING ALLOC OF SIZE: %d\t\t\t EXTRA SIZE: %d\n", req_size_including_overhead, extra_size); 
 if (extra_size > ALIGN(1 + OVERHEAD)) {
  unsigned long new_blk_ftr_off = (void*)GO_TO_FTR(bp) - (void*)bp; // DEBUG ONLY
  GET_SIZE(HDRP(bp)) = req_size_including_overhead; // SET HDR
  GET_SIZE(GO_TO_FTR(bp)) = req_size_including_overhead; // SET FTR
//  DEBUG_MARK(bp, 1);

  void* blk_ftr_ptr = GO_TO_FTR(bp);
  void* unalloc_bp = blk_ftr_ptr + sizeof(block_header);

  size_t unalloc_blk_size = extra_size;  // CALC SIZE OF UNALLOC BLOCK
  GET_SIZE(unalloc_bp) = extra_size; // 
  GET_SIZE(GO_TO_FTR(unalloc_bp)) = extra_size; // SET FTR
  GET_ALLOC(HDRP(unalloc_bp)) = 0;
  GET_ALLOC(GO_TO_FTR(unalloc_bp)) = 0; // SET FTR
  //DEBUG_MARK((void*)unalloc_bp,0);
 }
 GET_ALLOC(HDRP(bp)) = 1;
 GET_ALLOC(GO_TO_FTR(bp)) = 1;
 num_calls++;   
}



/*
 * Basic mm_free
 */
void mm_free(void *bp) {

 GET_ALLOC(HDRP(bp)) = 0;
 GET_ALLOC(GO_TO_FTR(bp)) = 0;
 allocated_block_count--; 
 print_block_info(first_bp); 
}

static unsigned long calc_offset(void* new_ptr)
{
  return (void*)new_ptr - (void*)first_allocation_ptr;
}

static void setACE(int* ace, int al_unal){
  if(al_unal == 0 ) {ace = (void*)ace + sizeof(block_header);   *ace = (unsigned int)2766;} // ace
  else{ace = (void*)ace + sizeof(block_header);   *ace = (unsigned int)3055;} //bef
}

static void setBAC(int* bac, int al_unal){
  if(al_unal == 0) {(void*)bac - sizeof(block_header);  *bac = (unsigned int)2988;}  // bac
  else{(void*)bac - sizeof(block_header);  *bac = (unsigned int)3773;} // ebd
}

static void mark_payload(block_header* p, int hdr_ftr){
  setACE(p, hdr_ftr);
  setBAC(HDRP(NEXT_BLKP(p)), hdr_ftr);
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  return 1;
}


static int check_aligned(void* bp, void* other){
  DEBUG_PRINT1(("PTR: %p, \tSZ: %d\n",bp, GET_SIZE(bp))); 
  DEBUG_PRINT1(("Offset: %d, \tSZ: %d\n",other - bp, GET_SIZE(other))); 
  unsigned long ptr = (unsigned long)bp;
  if(ptr % 16 != 0)
    return 0;
  return 1;
}
