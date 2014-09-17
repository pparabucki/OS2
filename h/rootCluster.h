// Project:	Operativni Sistemi 2
// File: 	RootCluster.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	RootCluster	

#pragma once

#include "cluster.h"
#include <Windows.h>

class Driver;

///broj field-a kod RC drugog nivoa
const ClusterNo numberOfFields	= 512;

///broj entry-a kod RootCluster-a
const unsigned int numberOfEntries  = 102;	//	2048 / 20 = 102 + 8bita
const unsigned short one = 1;
const unsigned int numberOfEntriesInFirstLevel = 26111;

class RootCluster : public Cluster{
	
	
	friend void TestRC();
	friend class KernelFS;


private: 

	Entry		entries[numberOfEntries];
	ClusterNo	fields[numberOfFields];
	ClusterNo	fields2[numberOfFields];
	


	///trenutna pozicija polja u IC
	volatile unsigned int CurrField;

	///pozicija u klasteru sa entry-ima
	volatile unsigned int FirstLevelEntry;
	
	///trenutna pozicija polja u drugom nivou
	volatile unsigned int SecondLevelField;
	///pozicija u kalasteru sa entry-ima
	volatile unsigned int SecondLevelEntry;

public:

	RootCluster(Drive *);
	~RootCluster();

	HANDLE rootMutex;

	void resetRC();

	/// dohvata Entry na osnovu pozicije
	Entry* getEntry(unsigned);

	/// vraca 1 - uspesno , 0 - neuspesno 
	int putEntry(Entry);

	/// vraca 1 - uspesno , 0 - neuspesno 
	int deleteEntry(char*);

	/// vraca 1 - uspesno , 0 - neuspesno 
	void updateEntrySize(char* , BytesCnt);

	char readRootDir(EntryNum,Directory&);



	//-----------------------------------------
	
	int getFilePosition(char*);




private:
	///upisi field-ove prvog nivoa na disk
	void writeFieldTD(ClusterNo);
	///procitaj field-ove prvog nivoa sa diska
	void readFieldFD(ClusterNo);

	///upisi field-ove drugog nivoa na disk
	void writeField2TD(ClusterNo);
	///procitaj field-ove drugog nivoa sa diska
	void readField2FD(ClusterNo);

	///upisi entry-e na disk
	void writeEntryTD(ClusterNo);
	///procitaj entry-e sa diska
	void readEntryFD(ClusterNo);
	///restartuje entry ne na disku
	void resetEntry();
	///postavlja vrednosti u promenljive
	int getClusters(ClusterNo&,ClusterNo&,EntryNum&,EntryNum);
	/// oslobadjanje Cluster-a koje je zauzeo fajl
	void deleteFileClusters(ClusterNo);
};