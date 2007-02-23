#ifndef CALLBACKCOUNTER_H_
#define CALLBACKCOUNTER_H_

#ifdef __cplusplus
extern "C" {
#endif

/** A type for cbc_counters_t structure */
typedef struct cbc_counters_t cbc_counters_t;

/** A container for callback counters */
struct cbc_counters_t {
	
	/** Call counter for the create function */
	int create;
	
	/** Call counter for the start function */
	int start;
	
	/** Call counter for the logger function */
	int logger;
	
	/** Call counter for the plug-in listener function */
	int listener;
	
	/** Call counter for the run function */
	int run;
	
	/** Call counter for the stop function */
	int stop;
	
	/** Call counter for the destroy function */
	int destroy;
};

#ifdef __cplusplus
}
#endif

#endif /*CALLBACKCOUNTER_H_*/
