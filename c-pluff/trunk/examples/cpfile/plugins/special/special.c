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

/**
 * Classifies a file by using stat(2). This classifier does not need
 * any classifier data so we use NULL as dummy data pointer. Therefore
 * we do not need a plug-in instance either as there is no data to be
 * initialized.
 */
static int classify(void *dummy, const char *path) {
	struct stat s;
	const char *type;
	
	// Stat the file
	if (lstat(path, &s)) {
		fflush(stdout);
		perror("stat failed");
		
		// No point for other classifiers to classify this
		return 1;
	}
	
	// Check if this is a special file
	if (S_ISDIR(s.st_mode)) {
		type = "directory";
	} else if (S_ISCHR(s.st_mode)) {
		type = "character device";
	} else if (S_ISBLK(s.st_mode)) {
		type = "block device";
	} else if (S_ISLNK(s.st_mode)) {
		type = "symbolic link";
	} else {
		
		// Did not recognize it, let other plug-ins try
		return 0;
	}
		
	// Print recognized file type
	fputs(type, stdout);
	putchar('\n');
	return 1;
}


/* ------------------------------------------------------------------------
 * Exported classifier information
 * ----------------------------------------------------------------------*/

CP_EXPORT classifier_t cp_ex_cpfile_special_classifier = { NULL, classify };
