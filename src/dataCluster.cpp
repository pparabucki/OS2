// Project:	Operativni Sistemi 2
// File: 	DataCluster.cpp
// Date: 	July 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	DataCluster	
#include "drive.h"
#include "KernelFS.h"
#include "DataCluster.h"

DataCluster::DataCluster(Drive* d , ClusterNo no,char* fname):Cluster(d,no){

	CurrField			=		0;
	FirstLevelData		=		0;
	SecondLevelField	=		0;
	SecondLevelData		=		0;
	indexClusterNo		=	   no;
	numOfDataClusters	=		0;
	putanjaFajla	=	fname;	// zbog buffer-a
	
	readFieldFD(no);	// upisuje vrednosti pocetnog indeksnog kastera u fields
	resetFields();

}


void DataCluster::writeDataTD(ClusterNo clNo){
	this->partition->writeCluster(clNo,this->cluster);
}

void DataCluster::readDataFD(ClusterNo clNo){
	this->partition->readCluster(clNo,this->cluster);
}

void DataCluster::writeFieldTD(ClusterNo clNo){
	memcpy(this->cluster,fields, sizeof(fields));
	this->partition->writeCluster(clNo,this->cluster);
}

void DataCluster::readFieldFD(ClusterNo clNo){
	this->partition->readCluster(clNo,this->cluster);
	memcpy(this->fields,this->cluster,sizeof(fields));
}

void DataCluster::writeField2TD(ClusterNo clNo){
	memcpy(this->cluster,fields2, sizeof(fields2));
	this->partition->writeCluster(clNo,this->cluster);
}

void DataCluster::readField2FD(ClusterNo clNo){
	this->partition->readCluster(clNo,this->cluster);
	memcpy(this->fields2,this->cluster,sizeof(fields2));
}

void DataCluster::resetData(){
	memset(cluster,NULL,sizeof(cluster));
}

void DataCluster::resetFields(){
	memset(fields,NULL,sizeof(cluster));
	
}
void DataCluster::resetFields2(){	
	memset(fields2,NULL,sizeof(cluster));
}



int DataCluster::getSLF(BytesCnt pozicija){
	
	long const int L1 = 256 * 2048;	// u prvi nivo	field	 524288 B
	long const int L2 = 512 * 2048;	// u drugi nivo	field2	1048576	B

	if(pozicija <  L1){ 
		this->CurrField			= pozicija/2048;
		this->FirstLevelData	= pozicija%2048;		
		this->SecondLevelField	= 0; 
		this->SecondLevelData	= 0; 
	}
	else{
		
		long int pom = 0 , kolikoPuta = 0 , ostatak = 0 , ostatak2 = 0;

		pom = pozicija - L1;
		kolikoPuta	= pom / L2;
		ostatak		= pom % L2;

		this->CurrField	 = 256 + kolikoPuta;
		this->FirstLevelData	= 0;	
		this->SecondLevelField = ( ostatak / 2048 ) % 512;
		this->SecondLevelData = ostatak % 2048;
	}
	
	return 1;
}

void DataCluster::setDrive(Drive &drive){
	*this->drive = drive;
}