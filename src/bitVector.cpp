// Project:	Operativni Sistemi 2
// File: 	bitVector.cpp
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	BitVector	

#include "bitVector.h"
#include <iostream>
using namespace std;


char BitVector::zero=0x00;
char BitVector::one =0x01;

CRITICAL_SECTION criticalBitV;


BitVector::BitVector(Drive* drive) : Cluster(drive,0){
		
	this->numberOfClusters = 0;
	InitializeCriticalSection(&criticalBitV);
	
}

ClusterNo BitVector::getFreeCluster(){

	EnterCriticalSection(&criticalBitV);
	
	this->partition->readCluster(0,this->cluster);

	for(ClusterNo i=0;i<ClusterSize;i++){	// pretrazuje polja klastera 0..2047
	
		if(this->cluster[i] != 0)
			for (int j=0; j <= 7; j ++)
					{	int pom;
							pom = 0;
							pom = (1 << j);
						if ( (this->cluster[i] & pom) ){
							
							this->cluster[i] &= ~pom;
							nextFreeCluster = i*8 + j;
							if( numberOfClusters > this->partition->getNumOfClusters()){ LeaveCriticalSection(&criticalBitV); return 0;}
							
							this->partition->writeCluster(0,this->cluster);
							this->numberOfClusters++;
							LeaveCriticalSection(&criticalBitV);
							return nextFreeCluster;	// vraca slobodan klaster
						}
					}
	}
	
	LeaveCriticalSection(&criticalBitV);
	return 0;	// vraca 0 ukoliko nema slobodnih klastera na particiji
				// maksimalan broj klastera je 2048*8 = 16384 , ali kada
				// se oduzmu nulti i prvi klaster max = 16382 -> max 32 MB
}

void BitVector::resetBV(){
	EnterCriticalSection(&criticalBitV);
	this->partition->readCluster(0,this->cluster);
	int i=0;
	while(i<ClusterSize){					// postavlja sve na slobodne klastere
		 this->cluster[i] = (char)0xff;
		i++;
	}
	this->cluster[0] = (char)0xfc;			// postavlja nulti i prvi klaster na zauzete
	this->partition->writeCluster(0,this->cluster);
	LeaveCriticalSection(&criticalBitV);
}

void BitVector::print(){
	EnterCriticalSection(&criticalBitV);
	// print
	for(int k=0;k<2048/60;k++){
		
		cout<<" "<<k<<" ";
		
		for (int j = 7; j >= 0; j --)
			{
				if(j==3) printf(" ");
				if ( (this->cluster[k] & (1 << j)) )
				cout<<"1";
				else
				cout<<"0";
			}
		if (k%4==0)cout<<endl;
	}
	LeaveCriticalSection(&criticalBitV);
}

ClusterNo BitVector::getnextFreeCluster(){
	EnterCriticalSection(&criticalBitV);
	ClusterNo ret = this->nextFreeCluster;
	LeaveCriticalSection(&criticalBitV);
	return ret;
}

void BitVector::freeCluster(ClusterNo clusterNo){
	EnterCriticalSection(&criticalBitV);
	this->partition->readCluster(0,this->cluster);

	ClusterNo bytePos , bitePos;

	bytePos = clusterNo / 8;
			
	bitePos = clusterNo % 8;
	
	this->cluster[bytePos] |= (1<<bitePos); 

	this->partition->writeCluster(0,this->cluster);
	LeaveCriticalSection(&criticalBitV);
}

BitVector::~BitVector(){
	DeleteCriticalSection(&criticalBitV); 
}