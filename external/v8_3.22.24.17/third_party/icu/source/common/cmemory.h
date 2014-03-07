/*
******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File CMEMORY.H
*
*  Contains stdlib.h/string.h memory functions
*
* @author       Bertrand A. Damiba
*
* Modification History:
*
*   Date        Name        Description
*   6/20/98     Bertrand    Created.
*  05/03/99     stephen     Changed from functions to macros.
*
******************************************************************************
*/

#ifndef CMEMORY_H
#define CMEMORY_H

#include <stddef.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/localpointer.h"

#define uprv_memcpy(dst, src, size) U_STANDARD_CPP_NAMESPACE memcpy(dst, src, size)
#define uprv_memmove(dst, src, size) U_STANDARD_CPP_NAMESPACE memmove(dst, src, size)
#define uprv_memset(buffer, mark, size) U_STANDARD_CPP_NAMESPACE memset(buffer, mark, size)
#define uprv_memcmp(buffer1, buffer2, size) U_STANDARD_CPP_NAMESPACE memcmp(buffer1, buffer2,size)

U_CAPI void * U_EXPORT2
uprv_malloc(size_t s);

U_CAPI void * U_EXPORT2
uprv_realloc(void *mem, size_t size);

U_CAPI void U_EXPORT2
uprv_free(void *mem);

/**
 * This should align the memory properly on any machine.
 * This is very useful for the safeClone functions.
 */
typedef union {
    long    t1;
    double  t2;
    void   *t3;
} UAlignedMemory;

/**
 * Get the least significant bits of a pointer (a memory address).
 * For example, with a mask of 3, the macro gets the 2 least significant bits,
 * which will be 0 if the pointer is 32-bit (4-byte) aligned.
 *
 * ptrdiff_t is the most appropriate integer type to cast to.
 * size_t should work too, since on most (or all?) platforms it has the same
 * width as ptrdiff_t.
 */
#define U_POINTER_MASK_LSB(ptr, mask) (((ptrdiff_t)(char *)(ptr)) & (mask))

/**
 * Get the amount of bytes that a pointer is off by from
 * the previous UAlignedMemory-aligned pointer.
 */
#define U_ALIGNMENT_OFFSET(ptr) U_POINTER_MASK_LSB(ptr, sizeof(UAlignedMemory) - 1)

/**
 * Get the amount of bytes to add to a pointer
 * in order to get the next UAlignedMemory-aligned address.
 */
#define U_ALIGNMENT_OFFSET_UP(ptr) (sizeof(UAlignedMemory) - U_ALIGNMENT_OFFSET(ptr))

/**
  *  Indicate whether the ICU allocation functions have been used.
  *  This is used to determine whether ICU is in an initial, unused state.
  */
U_CFUNC UBool 
cmemory_inUse(void);

/**
  *  Heap clean up function, called from u_cleanup()
  *    Clears any user heap functions from u_setMemoryFunctions()
  *    Does NOT deallocate any remaining allocated memory.
  */
U_CFUNC UBool 
cmemory_cleanup(void);

#ifdef XP_CPLUSPLUS

U_NAMESPACE_BEGIN

/**
 * "Smart pointer" class, deletes memory via uprv_free().
 * For most methods see the LocalPointerBase base class.
 * Adds operator[] for array item access.
 *
 * @see LocalPointerBase
 */
template<typename T>
class LocalMemory : public LocalPointerBase<T> {
public:
    /**
     * Constructor takes ownership.
     * @param p simple pointer to an array of T items that is adopted
     */
    explicit LocalMemory(T *p=NULL) : LocalPointerBase<T>(p) {}
    /**
     * Destructor deletes the memory it owns.
     */
    ~LocalMemory() {
        uprv_free(LocalPointerBase<T>::ptr);
    }
    /**
     * Deletes the array it owns,
     * and adopts (takes ownership of) the one passed in.
     * @param p simple pointer to an array of T items that is adopted
     */
    void adoptInstead(T *p) {
        uprv_free(LocalPointerBase<T>::ptr);
        LocalPointerBase<T>::ptr=p;
    }
    /**
     * Deletes the array it owns, allocates a new one and reset its bytes to 0.
     * Returns the new array pointer.
     * If the allocation fails, then the current array is unchanged and
     * this method returns NULL.
     * @param newCapacity must be >0
     * @return the allocated array pointer, or NULL if the allocation failed
     */
    inline T *allocateInsteadAndReset(int32_t newCapacity=1);
    /**
     * Deletes the array it owns and allocates a new one, copying length T items.
     * Returns the new array pointer.
     * If the allocation fails, then the current array is unchanged and
     * this method returns NULL.
     * @param newCapacity must be >0
     * @param length number of T items to be copied from the old array to the new one;
     *               must be no more than the capacity of the old array,
     *               which the caller must track because the LocalMemory does not track it
     * @return the allocated array pointer, or NULL if the allocation failed
     */
    inline T *allocateInsteadAndCopy(int32_t newCapacity=1, int32_t length=0);
    /**
     * Array item access (writable).
     * No index bounds check.
     * @param i array index
     * @return reference to the array item
     */
    T &operator[](ptrdiff_t i) const { return LocalPointerBase<T>::ptr[i]; }
};

template<typename T>
inline T *LocalMemory<T>::allocateInsteadAndReset(int32_t newCapacity) {
    if(newCapacity>0) {
        T *p=(T *)uprv_malloc(newCapacity*sizeof(T));
        if(p!=NULL) {
            uprv_memset(p, 0, newCapacity*sizeof(T));
            uprv_free(LocalPointerBase<T>::ptr);
            LocalPointerBase<T>::ptr=p;
        }
        return p;
    } else {
        return NULL;
    }
}


template<typename T>
inline T *LocalMemory<T>::allocateInsteadAndCopy(int32_t newCapacity, int32_t length) {
    if(newCapacity>0) {
        T *p=(T *)uprv_malloc(newCapacity*sizeof(T));
        if(p!=NULL) {
            if(length>0) {
                if(length>newCapacity) {
                    length=newCapacity;
                }
                uprv_memcpy(p, LocalPointerBase<T>::ptr, length*sizeof(T));
            }
            uprv_free(LocalPointerBase<T>::ptr);
            LocalPointerBase<T>::ptr=p;
        }
        return p;
    } else {
        return NULL;
    }
}

/**
 * Simple array/buffer management class using uprv_malloc() and uprv_free().
 * Provides an internal array with fixed capacity. Can alias another array
 * or allocate one.
 *
 * The array address is properly aligned for type T. It might not be properly
 * aligned for types larger than T (or larger than the largest subtype of T).
 *
 * Unlike LocalMemory and LocalArray, this class never adopts
 * (takes ownership of) another array.
 */
template<typename T, int32_t stackCapacity>
class MaybeStackArray {
public:
    /**
     * Default constructor initializes with internal T[stackCapacity] buffer.
     */
    MaybeStackArray() : ptr(stackArray), capacity(stackCapacity), needToRelease(FALSE) {}
    /**
     * Destructor deletes the array (if owned).
     */
    ~MaybeStackArray() { releaseArray(); }
    /**
     * Returns the array capacity (number of T items).
     * @return array capacity
     */
    int32_t getCapacity() const { return capacity; }
    /**
     * Access without ownership change.
     * @return the array pointer
     */
    T *getAlias() const { return ptr; }
    /**
     * Returns the array limit. Simple convenience method.
     * @return getAlias()+getCapacity()
     */
    T *getArrayLimit() const { return getAlias()+capacity; }
    /**
     * Access without ownership change. Same as getAlias().
     * A class instance can be used directly in expressions that take a T *.
     * @return the array pointer
     */
    operator T *() const { return ptr; }
    /**
     * Array item access (writable).
     * No index bounds check.
     * @param i array index
     * @return reference to the array item
     */
    T &operator[](ptrdiff_t i) { return ptr[i]; }
    /**
     * Deletes the array (if owned) and aliases another one, no transfer of ownership.
     * If the arguments are illegal, then the current array is unchanged.
     * @param otherArray must not be NULL
     * @param otherCapacity must be >0
     */
    void aliasInstead(T *otherArray, int32_t otherCapacity) {
        if(otherArray!=NULL && otherCapacity>0) {
            releaseArray();
            ptr=otherArray;
            capacity=otherCapacity;
            needToRelease=FALSE;
        }
    };
    /**
     * Deletes the array (if owned) and allocates a new one, copying length T items.
     * Returns the new array pointer.
     * If the allocation fails, then the current array is unchanged and
     * this method returns NULL.
     * @param newCapacity can be less than or greater than the current capacity;
     *                    must be >0
     * @param length number of T items to be copied from the old array to the new one
     * @return the allocated array pointer, or NULL if the allocation failed
     */
    inline T *resize(int32_t newCapacity, int32_t length=0);
    /**
     * Gives up ownership of the array if owned, or else clones it,
     * copying length T items; resets itself to the internal stack array.
     * Returns NULL if the allocation failed.
     * @param length number of T items to copy when cloning,
     *        and capacity of the clone when cloning
     * @param resultCapacity will be set to the returned array's capacity (output-only)
     * @return the array pointer;
     *         caller becomes responsible for deleting the array
     * @draft ICU 4.4
     */
    inline T *orphanOrClone(int32_t length, int32_t &resultCapacity);
private:
    T *ptr;
    int32_t capacity;
    UBool needToRelease;
    T stackArray[stackCapacity];
    void releaseArray() {
        if(needToRelease) {
            uprv_free(ptr);
        }
    }
    /* No comparison operators with other MaybeStackArray's. */
    bool operator==(const MaybeStackArray & /*other*/) {return FALSE;};
    bool operator!=(const MaybeStackArray & /*other*/) {return TRUE;};
    /* No ownership transfer: No copy constructor, no assignment operator. */
    MaybeStackArray(const MaybeStackArray & /*other*/) {};
    void operator=(const MaybeStackArray & /*other*/) {};

    // No heap allocation. Use only on the stack.
    //   (Declaring these functions private triggers a cascade of problems:
    //      MSVC insists on exporting an instantiation of MaybeStackArray, which
    //      requires that all functions be defined.
    //      An empty implementation of new() is rejected, it must return a value.
    //      Returning NULL is rejected by gcc for operator new.
    //      The expedient thing is just not to override operator new.
    //      While relatively pointless, heap allocated instances will function.
    // static void * U_EXPORT2 operator new(size_t size); 
    // static void * U_EXPORT2 operator new[](size_t size);
#if U_HAVE_PLACEMENT_NEW
    // static void * U_EXPORT2 operator new(size_t, void *ptr);
#endif
};

template<typename T, int32_t stackCapacity>
inline T *MaybeStackArray<T, stackCapacity>::resize(int32_t newCapacity, int32_t length) {
    if(newCapacity>0) {
        T *p=(T *)uprv_malloc(newCapacity*sizeof(T));
        if(p!=NULL) {
            if(length>0) {
                if(length>capacity) {
                    length=capacity;
                }
                if(length>newCapacity) {
                    length=newCapacity;
                }
                uprv_memcpy(p, ptr, length*sizeof(T));
            }
            releaseArray();
            ptr=p;
            capacity=newCapacity;
            needToRelease=TRUE;
        }
        return p;
    } else {
        return NULL;
    }
}

template<typename T, int32_t stackCapacity>
inline T *MaybeStackArray<T, stackCapacity>::orphanOrClone(int32_t length, int32_t &resultCapacity) {
    T *p;
    if(needToRelease) {
        p=ptr;
    } else if(length<=0) {
        return NULL;
    } else {
        if(length>capacity) {
            length=capacity;
        }
        p=(T *)uprv_malloc(length*sizeof(T));
        if(p==NULL) {
            return NULL;
        }
        uprv_memcpy(p, ptr, length*sizeof(T));
    }
    resultCapacity=length;
    ptr=stackArray;
    capacity=stackCapacity;
    needToRelease=FALSE;
    return p;
}

/**
 * Variant of MaybeStackArray that allocates a header struct and an array
 * in one contiguous memory block, using uprv_malloc() and uprv_free().
 * Provides internal memory with fixed array capacity. Can alias another memory
 * block or allocate one.
 * The stackCapacity is the number of T items in the internal memory,
 * not counting the H header.
 * Unlike LocalMemory and LocalArray, this class never adopts
 * (takes ownership of) another memory block.
 */
template<typename H, typename T, int32_t stackCapacity>
class MaybeStackHeaderAndArray {
public:
    /**
     * Default constructor initializes with internal H+T[stackCapacity] buffer.
     */
    MaybeStackHeaderAndArray() : ptr(&stackHeader), capacity(stackCapacity), needToRelease(FALSE) {}
    /**
     * Destructor deletes the memory (if owned).
     */
    ~MaybeStackHeaderAndArray() { releaseMemory(); }
    /**
     * Returns the array capacity (number of T items).
     * @return array capacity
     */
    int32_t getCapacity() const { return capacity; }
    /**
     * Access without ownership change.
     * @return the header pointer
     */
    H *getAlias() const { return ptr; }
    /**
     * Returns the array start.
     * @return array start, same address as getAlias()+1
     */
    T *getArrayStart() const { return reinterpret_cast<T *>(getAlias()+1); }
    /**
     * Returns the array limit.
     * @return array limit
     */
    T *getArrayLimit() const { return getArrayStart()+capacity; }
    /**
     * Access without ownership change. Same as getAlias().
     * A class instance can be used directly in expressions that take a T *.
     * @return the header pointer
     */
    operator H *() const { return ptr; }
    /**
     * Array item access (writable).
     * No index bounds check.
     * @param i array index
     * @return reference to the array item
     */
    T &operator[](ptrdiff_t i) { return getArrayStart()[i]; }
    /**
     * Deletes the memory block (if owned) and aliases another one, no transfer of ownership.
     * If the arguments are illegal, then the current memory is unchanged.
     * @param otherArray must not be NULL
     * @param otherCapacity must be >0
     */
    void aliasInstead(H *otherMemory, int32_t otherCapacity) {
        if(otherMemory!=NULL && otherCapacity>0) {
            releaseMemory();
            ptr=otherMemory;
            capacity=otherCapacity;
            needToRelease=FALSE;
        }
    };
    /**
     * Deletes the memory block (if owned) and allocates a new one,
     * copying the header and length T array items.
     * Returns the new header pointer.
     * If the allocation fails, then the current memory is unchanged and
     * this method returns NULL.
     * @param newCapacity can be less than or greater than the current capacity;
     *                    must be >0
     * @param length number of T items to be copied from the old array to the new one
     * @return the allocated pointer, or NULL if the allocation failed
     */
    inline H *resize(int32_t newCapacity, int32_t length=0);
    /**
     * Gives up ownership of the memory if owned, or else clones it,
     * copying the header and length T array items; resets itself to the internal memory.
     * Returns NULL if the allocation failed.
     * @param length number of T items to copy when cloning,
     *        and array capacity of the clone when cloning
     * @param resultCapacity will be set to the returned array's capacity (output-only)
     * @return the header pointer;
     *         caller becomes responsible for deleting the array
     * @draft ICU 4.4
     */
    inline H *orphanOrClone(int32_t length, int32_t &resultCapacity);
private:
    H *ptr;
    int32_t capacity;
    UBool needToRelease;
    // stackHeader must precede stackArray immediately.
    H stackHeader;
    T stackArray[stackCapacity];
    void releaseMemory() {
        if(needToRelease) {
            uprv_free(ptr);
        }
    }
    /* No comparison operators with other MaybeStackHeaderAndArray's. */
    bool operator==(const MaybeStackHeaderAndArray & /*other*/) {return FALSE;};
    bool operator!=(const MaybeStackHeaderAndArray & /*other*/) {return TRUE;};
    /* No ownership transfer: No copy constructor, no assignment operator. */
    MaybeStackHeaderAndArray(const MaybeStackHeaderAndArray & /*other*/) {};
    void operator=(const MaybeStackHeaderAndArray & /*other*/) {};

    // No heap allocation. Use only on the stack.
    //   (Declaring these functions private triggers a cascade of problems;
    //    see the MaybeStackArray class for details.)
    // static void * U_EXPORT2 operator new(size_t size); 
    // static void * U_EXPORT2 operator new[](size_t size);
#if U_HAVE_PLACEMENT_NEW
    // static void * U_EXPORT2 operator new(size_t, void *ptr);
#endif
};

template<typename H, typename T, int32_t stackCapacity>
inline H *MaybeStackHeaderAndArray<H, T, stackCapacity>::resize(int32_t newCapacity,
                                                                int32_t length) {
    if(newCapacity>=0) {
        H *p=(H *)uprv_malloc(sizeof(H)+newCapacity*sizeof(T));
        if(p!=NULL) {
            if(length<0) {
                length=0;
            } else if(length>0) {
                if(length>capacity) {
                    length=capacity;
                }
                if(length>newCapacity) {
                    length=newCapacity;
                }
            }
            uprv_memcpy(p, ptr, sizeof(H)+length*sizeof(T));
            releaseMemory();
            ptr=p;
            capacity=newCapacity;
            needToRelease=TRUE;
        }
        return p;
    } else {
        return NULL;
    }
}

template<typename H, typename T, int32_t stackCapacity>
inline H *MaybeStackHeaderAndArray<H, T, stackCapacity>::orphanOrClone(int32_t length,
                                                                       int32_t &resultCapacity) {
    H *p;
    if(needToRelease) {
        p=ptr;
    } else {
        if(length<0) {
            length=0;
        } else if(length>capacity) {
            length=capacity;
        }
        p=(H *)uprv_malloc(sizeof(H)+length*sizeof(T));
        if(p==NULL) {
            return NULL;
        }
        uprv_memcpy(p, ptr, sizeof(H)+length*sizeof(T));
    }
    resultCapacity=length;
    ptr=&stackHeader;
    capacity=stackCapacity;
    needToRelease=FALSE;
    return p;
}

U_NAMESPACE_END

#endif  /* XP_CPLUSPLUS */
#endif  /* CMEMORY_H */
