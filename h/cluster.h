// Project:	Operativni Sistemi 2
// File: 	cluster.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	Cluster	

#pragma once

#include <iostream>

#include "part.h"
#include "drive.h"
#include "fs.h"

class Drive;
class Partition;

class Cluster {
	// ubaceno radi testova
	friend int main();
	friend void print(Cluster* c);
	friend class BitVector;
	friend class RootCluster;
	friend void TestKlasteri();
	friend void TestCitanjeParticije(Partition*, Cluster*);
	
	//-----------------------

protected:
	///Redni broj klastera
	ClusterNo clusterNo;
	char cluster[ClusterSize];

	Drive *drive;
	Partition *partition;

public:
	
	Cluster(Drive *, ClusterNo);
	~Cluster();

	void setClusterNo(ClusterNo cn); 

	///dohvata redni broj klastera
	ClusterNo getClusterNo() const;
};