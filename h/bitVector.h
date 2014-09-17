// Project:	Operativni Sistemi 2
// File: 	bitVector.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	BitVector	

#pragma once

#include "cluster.h"

class Drive;

#define MAX_NUMBER_OF_CLUSTERS 16384	// 2048 * 8	, 2048 bajta = 16 384 bita

class Cluster;

class BitVector : public Cluster {
	friend class RootCluster;
	friend class KernelFS;
	friend class KernelFile;
private: 
	
	static char zero; 
	static char one;
		
	ClusterNo nextFreeCluster;
	ClusterNo numberOfClusters;
	
public:

	BitVector(Drive* drive);
	~BitVector();

	///zauzima prvi slobodan klaster i vraca njegov broj
	ClusterNo getFreeCluster();
	///vraca vrednost nextFreeCluster
	ClusterNo getnextFreeCluster();

	///postavlja dati klaster kao slobodan
	void freeCluster(ClusterNo);

	///print bitski
	void print();
	void resetBV();
};