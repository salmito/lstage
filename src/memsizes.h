#ifndef MEM_SIZES_H        
#define MEM_SIZES_H      

#ifdef PAGE_SIZE          
#undef PAGE_SIZE          
#endif                    
#define PAGE_SIZE (4096) 

#ifdef CACHE_LINE_SIZE          
#undef CACHE_LINE_SIZE          
#endif                    
#define CACHE_LINE_SIZE (64) 

#endif /* MEM_SIZES_H */   
