// Project:	Operativni Sistemi 2
// File: 	KernelFS.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	KernelFS

#pragma once

#include "fs.h"
#include "semafori.h"
#include "file.h"
#include "listOfFiles.h"
#include "rw.h"
#include <windows.h>
#include <list>

class ListOfFiles;
class Drive;
class File;
using namespace std;

class KernelFS {
	

public: 

	friend int main();
	friend void TestMountFormatInsertGet();
	friend class dataCluster;
	friend class KernelFile;
	friend class ListOfFiles;

	// TEST
	static bool pronadjen;
	static list<RW*>* files;
	static HANDLE KFSmutex;
	static HANDLE mountMutex;
	static HANDLE KernelDEL;
	KernelFS();
	~KernelFS(); 
 
	/// montira particiju
	/// vraca dodeljeno slovo
	/// ili 0 u slucaju neuspeha
	char mount(Partition*);		

	/// demontiranje zadate particije
	/// 0 - neuspesno , 1 - uspesno	
	char unmount(char);			

	/// formatiranje zadate particije
	/// 0 - neuspesno , 1 - uspesno
	char format(char);			
	 
	///	prvim argumentom se zadaje particija, drugim redni broj
	///	validnog ulaza od kog se pocinje citanje,
	///	treci argument je adresa na kojoj se smesta procitani niz ulaza 
	char readRootDir(char, EntryNum, Directory &d);		
 
	///	argument je naziv fajla 
	///zadata psolutnom putanjom
	char doesExist(char*);		
								

	File& open(char*, char); 
	char deleteFile(char*); //  vraca 0 ukoliko je neuspesno , 1 uspesno
	static void updateSizeOfFile(char*,BytesCnt);
	
	ListOfFiles *openedFiles;
	

protected:

	

	Drive* getDrive(char driveLetter) const;
	Entry* makeEntry(char *fname);
	Entry* getEntry(char *fname);
	Directory buffer;
	static Drive* drives[26];
	static short int currEntryInBuffer;
	File* f;
		
};