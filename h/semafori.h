#pragma once
#include<iostream>
#include<fstream>
#include<cstdlib>
#include<windows.h>

#define sem_signal(x) ReleaseSemaphore(x, 1, NULL)
#define sem_wait(x)   WaitForSingleObject(x, INFINITE)

extern HANDLE mutexList;

static int decodeName(char* fname , char &part , char* &fnam, char* &fext){	//  cuva labelu particije u part
																			//	cuva ime fajla u fnam
																			//	cuva ekstenziju fajla u fext
	
	
	
	if( strlen(fname)>15 ) {
			
		return 0;
	
	}

	int i=0 , j=0;

	part = toupper(fname[0]);

	i=3; 
	for(; fname[i] != '.' && j<8; i++ , j++){			//kopira ime od treceg znaka do tacke 
														//i dok je ime manje od 8 karaktera
		fnam[j] = fname[i];
	}
	// u while ulazi samo ako je j<8 da doda razmake
	while(j<8){
		fnam[j] = ' ';
		j++;
	}
	fnam[j]='\0';
	
	i++;
	
	for(j=0;j<3;i++,j++) 
		fext[j] = fname[i];
	fext[j]='\0';
	return 1;
}