/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cpluff.h>
#include <core.h>

/* ------------------------------------------------------------------------
 * Internal functions
 * ----------------------------------------------------------------------*/

static int classify(const char *path) {
	struct stat s;
	
	/* Stat the file */
	if (lstat(path, &s)) {
		perror("stat failed");
		
		/* No point for other classifiers to classify this */
		return 1;
	}
	
	/* Check if this is a special file */
	// TODO
	
	/* Did not recognize so let other plug-ins try */
	return 0;
}


/* ------------------------------------------------------------------------
 * Exported classifier information
 * ----------------------------------------------------------------------*/

classifier_t cp_ex_cpfile_special_classifier = { classify };
