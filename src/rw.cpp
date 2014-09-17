// Project:	Operativni Sistemi 2
// File: 	rw.cpp
// Date: 	September 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	RW

#include "rw.h"

#include "rw.h"
#include "iostream"

RW::RW(char* put, char part) {  // put = fajl1, falj2...
        fpath = new char[strlen(put)+1];
        memcpy(fpath,put,strlen(put)+1);
        rw = CreateSemaphore(NULL,1,10,NULL); 
		freeToDel = CreateSemaphore(NULL,0,1,NULL);
		numR = 0;
		this->part = part;
		numOfThreads=0;
}

RW::~RW() {
        CloseHandle(rw);
		CloseHandle(freeToDel);
}

void RW::startRead() {
		
        if (++numR == 1) 
			sem_wait(rw);
		numOfThreads++;
}

void RW::endRead() {

		numOfThreads--;
		if (--numR == 0) 
		sem_signal(rw);  
		
		
}

void RW::startWrite() {
		
	    sem_wait(rw); 
		numOfThreads++;
}

void RW::endWrite() {

		numOfThreads--;
        sem_signal(rw);		
}
