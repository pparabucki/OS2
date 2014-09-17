// Project:	Operativni Sistemi 2
// File: 	drive.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	Drive	

#pragma once
#include <Windows.h>
#include "cluster.h"
#include "fs.h"

class RootCluster;

class Drive{
	// ubaceno radi testova
	friend int main();
	friend class BitVector;
	friend class RootCluster;
	friend void TestBV();
	friend void TestRC();
	friend void TestKlasteri();
	friend void TestMountFormatInsertGet();
	friend class KernelFS;
	friend class dataCluster;
	friend class KernelFile;
	
	//-----------------------

	Partition *partition;
	char driveLetter;
	HANDLE freeToFormat;	// ovaj ce da signalizira da je moguce da se formatira particija
	BitVector *bitV;
	ClusterNo freeClusterNo;
	RootCluster *rootCluster;

	int numOfFiles;
	int numOfOpenedFiles;

public:

	Drive(Partition*, char);
	~Drive();

	char getDriveLetter() const;

	Partition* getPartition() const;

	ClusterNo getFreeCluster();
	void insertEntry(Entry);
	void deleteEntry(char*);
	char readRootDir(EntryNum,Directory&);
	void updateEntrySize(char*,BytesCnt);

};