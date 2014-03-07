/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * jambase.h - declaration for the internal jambase
 *
 * The file Jambase is turned into a C array of strings in jambase.c
 * so that it can be built in to the executable.  This is the
 * declaration for that array.
 */

extern char *jambase[];
