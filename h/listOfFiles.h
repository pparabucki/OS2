// Project:	Operativni Sistemi 2
// File: 	listOfFiles.h
// Date: 	August 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	listOfFiles

#pragma once

#include <iostream>
#include <windows.h>
#include "fs.h"
#include "file.h"
#include "KernelFS.h"

using namespace std;

class KernelFS;
class Elem {   
	
	public:
													// ELEMENT LISTE:
   	char* fname;									// - ime fajla
    Elem* next;                		 				// - pokazivac na sledeci element,
	Elem* prev;										// - pokazivac na prethodni element
	int   threadID;									// - ID thread-a koji se ubacuje
	char  mode;										// - modalitet otvaranja fajla
	int	  numOfReaders;								// - broj citalaca
	int	  numOfWriters;								// - broj pisaca
	ClusterNo firstCluster;							// - pocetni index klaster
	BytesCnt size;									// - velicina
	HANDLE freeToDel;								// - semafor kada svi zavrse da moze da brise 
	Elem(char* fnam,Entry e,int tID,char mod){								// - konstruktor.
		fname = new char[17]();
		mode = mod;
		threadID = tID;
		strcpy(fname,fnam);
		next=NULL;
		prev=NULL;
		numOfReaders = 0;
		numOfWriters = 0;
		firstCluster = e.firstCluster;
		size		 = e.size;
		freeToDel = CreateSemaphore(NULL,0,1,NULL);
	}
	
	ClusterNo getFC()		{return firstCluster;}
	void setFC(ClusterNo fc){firstCluster=fc;}

	BytesCnt getSize()			 {return size;}
	void	 setSize(BytesCnt s) {size = s;}
};

class ListOfFiles{

	friend class KernelFS;
	friend class KernelFile;
	friend int main();
	friend void TestMountFormatInsertGet();
	friend int TestFajlovi();
	

public:
	ListOfFiles();		// konstruktor
	static HANDLE mutexList;
private:    

	  static Elem* head;         // Pokazivac na pocetak liste.
	  static Elem* curr;         // Pokazivac na tekuci element liste.
	  static Elem* tail;         // Pokazivac na kraj liste
	  static int cnt;
  
	  
	  ~ListOfFiles();			// Destruktor.
	  int count();           	// Broj elemenata liste.
	  static void printList();  		// Prolazak kroz celu listu i njeno ispisivanje.
 
	  void addItem(char*,Entry,int,char);   	// Dodavanje na kraj.
	 
	  Entry* isItOpen(char*,char,int);		// Trazi u listi otvorenih , otvoren - 1 , ne postoji - 0
	  bool isItOpenBool(char*);
  
	  static bool delItem(char*, int); 		// Brisanje Elementa iz liste.
	  void delAll();            // Brisanje svih File-ova iz liste.
	  void delAllFin();         // Brisanje svih File-ova iz liste koje su zavrseni.


	  Elem* getHead(){ return head; } // Dohvatanje Head pokazivaca.
	  Elem* getTail(){ return tail; } // Dohvatanje Tail pokazivaca.

	  Entry e;
};