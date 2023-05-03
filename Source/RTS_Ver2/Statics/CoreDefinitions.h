// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


/** 
 *	---------------------------------------------------------------------------
 *	TRawPtr:
 *	---------------------------------------------------------------------------
 *	TODO think of a better name
 *	TODO would be cool if users could type TRawPtr<AActor> instead of TRawPtr(AActor)
 *	Don't think that can be achieved with macros. May have to write own class that in 
 *	shipping is just a raw pointer. Will likely have to define all the operators
 *	- in shipping builds it is a raw pointer 
 *	- in non-shipping builds it is a TWeakObjectPtr 
 *	
 *	Only use this if you never do validity checks on the pointer 
 *	
 *	The advantage of using this is you can easily detect if you're deferencing null during 
 *	development. 
 *	When you package for shipping you get a raw pointer. Now I know deferencing a TWeakObjectPtr 
 *	is just as fast as a raw pointer, but there's also likely some slight overhead with the 
 *	creation of the TWeakObjectPointer which is what I want to avoid. 
 */
#if UE_BUILD_SHIPPING
#define TRawPtr(T) T *
#else
#define TRawPtr(T) TWeakObjectPtr<T> 
#endif

