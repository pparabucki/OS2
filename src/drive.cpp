// Project:	Operativni Sistemi 2
// File: 	drive.cpp
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	Drive	

#include "drive.h"
#include "bitVector.h"
#include "rootCluster.h"

using namespace std;

Drive::Drive(Partition* part, char driveLetter){
	
	this->partition		= part;
	this->driveLetter	= driveLetter;
	numOfOpenedFiles = 0;
	numOfFiles = 0;

	this->bitV = new BitVector(this);
	this->rootCluster = new RootCluster(this);

	freeToFormat = CreateSemaphore(NULL,1,1,NULL);
	
}

char Drive::getDriveLetter()const {
	return this->driveLetter;
}

Partition* Drive::getPartition() const{
	return this->partition;
}

ClusterNo Drive::getFreeCluster(){
	
	return this->bitV->getFreeCluster();
		
}
void Drive::insertEntry(Entry e){
	if (!this->rootCluster->putEntry(e)){ 
		//cout<<"Nije moguce uneti enty "<<e.name<<" zato sto nema slobodnih klastera"<<endl;
	}
}
void Drive::deleteEntry(char* fname){
	this->rootCluster->deleteEntry(fname);
}

char Drive::readRootDir(EntryNum n , Directory &d){
	return this->rootCluster->readRootDir(n,d);	
}

void Drive::updateEntrySize(char* fname , BytesCnt cnt){
	this->rootCluster->updateEntrySize(fname,cnt);
}
Drive::~Drive(){
	delete this->bitV;
	delete this->rootCluster;
}