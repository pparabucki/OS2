// Project:	Operativni Sistemi 2
// File: 	rw.h
// Date: 	September 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	RW

#pragma once


#include <iostream>
#include <cstring>
#include "semafori.h"

class RW { 
        
public:
	HANDLE rw, freeToWrite;
    char* fpath;
	char part;
    int numR;	
	int numOfThreads;
	HANDLE freeToDel;
	
	RW(char* put, char part);
    ~RW();
	
	void startRead();
    void endRead();
    void startWrite();
    void endWrite();
};

