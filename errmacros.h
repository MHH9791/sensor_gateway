#ifndef __errmacros_h__
#define __errmacros_h__

#include <errno.h>

#define LINE_PRINT() fprintf(stderr, "\033[31m[ERROR]\033[0m In %s - function %s at line %d:\n", __FILE__, __func__, __LINE__);
//some from code in the lecture
#define FILE_ERROR(fp, filename) \
	do { \
		if ((fp)==NULL) { \
			LINE_PRINT(); \
			printf("Fail to open file: %s\n",filename); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#define FFLUSH_ERROR(err)                                     \
do {                                                \
    if ( (err) == EOF )                                \
    {                                                \
        perror("fflush failed");                    \
        exit( EXIT_FAILURE );                        \
    }                                                \
} while(0)

// errors when using system calls
#define SYS_ERR(rc) \
	do { \
		if (rc == -1) { \
			LINE_PRINT(); \
			fprintf(stderr, "Error Code %d\n", rc); \
			perror("Syscall Error"); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors regarding TCP
#define TCP_ERR(rc) \
    do { \
        if (rc != TCP_NO_ERROR) { \
            LINE_PRINT(); \
            printf("TCP error %d\n", rc); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// error when use mkfifo
#define CHECK_MKFIFO(err)\
		do {\
			if ( (err) == -1 )\
			{\
				if ( errno != EEXIST )\
				{\
					perror("Error executing mkfifo");\
					exit( EXIT_FAILURE );\
				}\
			}\
		} while(0)

// error when close file 
#define FILE_CLOSE_ERROR(err) 								\
		do {												\
			if ( (err) == -1 )								\
			{												\
				LINE_PRINT();	 							\
				perror("File close failed");				\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)


//error when malloc
#define MALLOC_ERR(dummy)                                   \
	do {                                                    \
		if (dummy == NULL) {                                \
			LINE_PRINT();                                   \
			fprintf(stderr, "Malloc error\n");   \
			exit(EXIT_FAILURE);                             \
		}                                                   \
	} while(0)

// error macro for asprintf
#define ASPRINTF_ERR(rc)									\
	do { 													\
		if (rc == -1) { 									\
			LINE_PRINT();	 								\
			perror("Asprintf error"); 						\
			exit(EXIT_FAILURE); 							\
		} 													\
	} while(0)

// error macro for thread
#define PTHR_ERR(rc)                                        \
    do { 													\
		if (rc != 0) { 										\
			LINE_PRINT();	 								\
			fprintf(stderr, "Pthread error, error Code %d\n", rc); \
	        perror("Pthread error"); 						\
	        exit(EXIT_FAILURE); 							\
		} 													\
    } while(0)

// error macro for mutex
#define MUTX_ERR(rc)                                        \
    do { 													\
		if (rc != 0) { 										\
			LINE_PRINT();	 								\
			fprintf(stderr, "Mutx error, error Code %d\n", rc); \
	        perror("Pthread error"); 						\
	        exit(EXIT_FAILURE); 							\
		} 													\
    } while(0)

//sbuffer error
#define SBUFFER_ERR(rc) 									\
	do { 													\
		if (rc == SBUFFER_FAILURE) { 						\
			LINE_PRINT(); 									\
			fprintf(stderr, "Sbuffer error %d\n", rc); 		\
			exit(EXIT_FAILURE); 							\
		} 													\
	} while(0)
#endif



