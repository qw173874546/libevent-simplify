
#ifndef MM_INTERNAL_H_INCLUDED_
#define MM_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

//这里直接用系统内存分配函�?  
#define mm_malloc(sz) malloc(sz)
#define mm_calloc(n, sz) calloc((n), (sz))
#define mm_strdup(s) strdup(s)
#define mm_realloc(p, sz) realloc((p), (sz))
#define mm_free(p) free(p)

#ifdef __cplusplus
}
#endif

#endif
