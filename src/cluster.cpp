// Project:	Operativni Sistemi 2
// File: 	cluster.cpp
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	Cluster	

#include "cluster.h"

Cluster::Cluster(Drive *drive, ClusterNo clusterNo){
	this->drive		= drive;
	this->clusterNo = clusterNo; 
	this->partition = drive->getPartition();
	this->partition->readCluster(clusterNo,this->cluster);
}

void Cluster::setClusterNo(ClusterNo cn){
	this->clusterNo = cn;
}

ClusterNo Cluster::getClusterNo()const{
	return this->clusterNo;
}
Cluster::~Cluster(){
	delete [] cluster;
}