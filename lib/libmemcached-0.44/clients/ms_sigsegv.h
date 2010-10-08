/*
 * File:   ms_sigsegv.h
 * Author: Mingqiang Zhuang
 *
 * Created on March 15, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */
#ifndef MS_SIGSEGV_H
#define MS_SIGSEGV_H

#ifdef __cplusplus
extern "C" {
#endif

/* redirect signal seg */
int ms_setup_sigsegv(void);


/* redirect signal pipe */
int ms_setup_sigpipe(void);


/* redirect signal int */
int ms_setup_sigint(void);


#ifdef __cplusplus
}
#endif

#endif /* end of MS_SIGSEGV_H */
