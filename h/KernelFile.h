#pragma once
#include<iostream>
#include<fstream>
#include<cstdlib>
#include<windows.h>
#include<ctime>
#include "fs.h"
#include "file.h"
#include "part.h"
#include "KernelFS.h"


class Cluster;
class DataCluster;

#define MAX_SIZE 256*2048 + 256*512*2048	// maksimalna velicina u bajtovima
#define BUFF_SIZE 32


#define sem_signal(x) ReleaseSemaphore(x, 1, NULL)
#define sem_wait(x)   WaitForSingleObject(x, INFINITE)

class BitVector;

class KernelFile
{
	friend class KernelFS;
	friend class ListOfFiles;
	friend void TestMountFormatInsertGet();
	friend int TestSaSlikom();
	friend int TestFajlovi();
	friend class Elem;

	
	char* fname;				//ime fajla
	BytesCnt size;				//velicina fajla
	BytesCnt pozicija;			//pozicija u fajlu
	char mod;					//write/read
	ClusterNo firstDC;			//broj prvog dataCluster-a
	DataCluster*	dataCl;
	static DataCluster* dataBuff[BUFF_SIZE];	// buffer koji ce sadrzati 32 fajla sa dataCluster-ima


public:
	
	HANDLE mutexFile;

	void setMode(char m){ this->mod = m; }		//	za promenu moda
	KernelFile(ClusterNo, BytesCnt,char,char*,DataCluster&);
	~KernelFile();

	char write(BytesCnt, char*); 
	BytesCnt read(BytesCnt, char*);
	char truncate(); // - **

	char seek(BytesCnt);  
	BytesCnt filePos()const{ return pozicija; };
	char eof();
	BytesCnt getFileSize() {return size; };
	void delOpenedFromList();
	
};