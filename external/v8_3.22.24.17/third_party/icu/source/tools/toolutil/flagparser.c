/******************************************************************************
 *   Copyright (C) 2009-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 */

#include "flagparser.h"
#include "filestrm.h"
#include "cstring.h"
#include "cmemory.h"

#define DEFAULT_BUFFER_SIZE 512

static int32_t currentBufferSize = DEFAULT_BUFFER_SIZE;

static void extractFlag(char* buffer, int32_t bufferSize, char* flag, int32_t flagSize, UErrorCode *status);
static int32_t getFlagOffset(const char *buffer, int32_t bufferSize);

/*
 * Opens the given fileName and reads in the information storing the data in flagBuffer.
 */
U_CAPI int32_t U_EXPORT2
parseFlagsFile(const char *fileName, char **flagBuffer, int32_t flagBufferSize, int32_t numOfFlags, UErrorCode *status) {
    char* buffer = uprv_malloc(sizeof(char) * currentBufferSize);
    UBool allocateMoreSpace = FALSE;
    int32_t i;
    int32_t result = 0;

    FileStream *f = T_FileStream_open(fileName, "r");
    if (f == NULL) {
        *status = U_FILE_ACCESS_ERROR;
        return -1;
    }

    if (buffer == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return -1;
    }

    do {
        if (allocateMoreSpace) {
            allocateMoreSpace = FALSE;
            currentBufferSize *= 2;
            uprv_free(buffer);
            buffer = uprv_malloc(sizeof(char) * currentBufferSize);
            if (buffer == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return -1;
            }
        }
        for (i = 0; i < numOfFlags; i++) {
            if (T_FileStream_readLine(f, buffer, currentBufferSize) == NULL) {
                *status = U_FILE_ACCESS_ERROR;
                break;
            }

            if (uprv_strlen(buffer) == (currentBufferSize - 1) && buffer[currentBufferSize-2] != '\n') {
                /* Allocate more space for buffer if it didnot read the entrire line */
                allocateMoreSpace = TRUE;
                T_FileStream_rewind(f);
                break;
            } else {
                extractFlag(buffer, currentBufferSize, flagBuffer[i], flagBufferSize, status);
                if (U_FAILURE(*status)) {
                    if (*status == U_BUFFER_OVERFLOW_ERROR) {
                        result = currentBufferSize;
                    } else {
                        result = -1;
                    }
                    break;
                }
            }
        }
    } while (allocateMoreSpace && U_SUCCESS(*status));

    uprv_free(buffer);

    T_FileStream_close(f);

    if (U_SUCCESS(*status) && result == 0) {
        currentBufferSize = DEFAULT_BUFFER_SIZE;
    }

    return result;
}


/*
 * Extract the setting after the '=' and store it in flag excluding the newline character.
 */
static void extractFlag(char* buffer, int32_t bufferSize, char* flag, int32_t flagSize, UErrorCode *status) {
    int32_t i;
    char *pBuffer;
    int32_t offset;
    UBool bufferWritten = FALSE;

    if (buffer[0] != 0) {
        /* Get the offset (i.e. position after the '=') */
        offset = getFlagOffset(buffer, bufferSize);
        pBuffer = buffer+offset;
        for(i = 0;;i++) {
            if (i >= flagSize) {
                *status = U_BUFFER_OVERFLOW_ERROR;
                return;
            }
            if (pBuffer[i+1] == 0) {
                /* Indicates a new line character. End here. */
                flag[i] = 0;
                break;
            }

            flag[i] = pBuffer[i];
            if (i == 0) {
                bufferWritten = TRUE;
            }
        }
    }

    if (!bufferWritten) {
        flag[0] = 0;
    }
}

/*
 * Get the position after the '=' character.
 */
static int32_t getFlagOffset(const char *buffer, int32_t bufferSize) {
    int32_t offset = 0;

    for (offset = 0; offset < bufferSize;offset++) {
        if (buffer[offset] == '=') {
            offset++;
            break;
        }
    }

    if (offset == bufferSize || (offset - 1) == bufferSize) {
        offset = 0;
    }

    return offset;
}
