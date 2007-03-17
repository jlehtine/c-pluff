/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#ifndef CORE_H_
#define CORE_H_

/**
 * A function that classifies a file. If the classification succeeds then
 * the function should print file description to standard output and
 * return a non-zero value. Otherwise the function must return zero.
 *
 * @param path the file path 
 * @return whether classification was successful
 */
typedef int (*classify_func_t)(const char *path);

/** A short hand typedef for classifier_t structure */
typedef struct classifier_t classifier_t;

/**
 * A container for classifier information. We use a container instead of
 * direct function pointer to comply with ANSI C.
 */
struct classifier_t {
	
	/** The classifying function */
	classify_func_t classify;
}; 

#endif /*CORE_H_*/
