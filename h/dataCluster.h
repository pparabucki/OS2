// Project:	Operativni Sistemi 2
// File: 	DataCluster.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	DataCluster	

#pragma once

#include "cluster.h"
#include "RootCluster.h"

const unsigned int sizeOfDC = 2048;

class DataCluster : public Cluster{
	
	friend class KernelFile;
	friend class KernelFS;

private: 
	
	//	podaci koji se upisuju na disk se nalaze u custer[2048]
	char*		putanjaFajla;
	ClusterNo	fields[numberOfFields];
	ClusterNo	fields2[numberOfFields];
	///sadrzi vrednost indeksnog klastera koji u svojim poljima ima pokazivace na druge klastere
	ClusterNo	indexClusterNo;
	ClusterNo	numOfDataClusters;

	///trenutna pozicija polja u IC
	volatile unsigned int CurrField;

	///pozicija u klasteru sa podacima
	volatile unsigned int FirstLevelData;
	
	///trenutna pozicija polja u drugom nivou
	volatile unsigned int SecondLevelField;
	///pozicija u kalasteru sa podacima
	volatile unsigned int SecondLevelData;

public:

	DataCluster(Drive *, ClusterNo,char*);
	~DataCluster();


private:
	///upisi field-ove prvog nivoa na disk
	void writeFieldTD(ClusterNo);
	///procitaj field-ove prvog nivoa sa diska
	void readFieldFD(ClusterNo);

	///upisi field-ove drugog nivoa na disk
	void writeField2TD(ClusterNo);
	///procitaj field-ove drugog nivoa sa diska
	void readField2FD(ClusterNo);

	///upisi data na disk u Cluster->cluster
	void writeDataTD(ClusterNo);
	///procitaj data sa diska u Cluster->cluster
	void readDataFD(ClusterNo);
	///restartuje entry ne na disku
	void resetData();

	void resetFields();
	void resetFields2();

	int getSLF(BytesCnt);
	void setDrive(Drive&);
};