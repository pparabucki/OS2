// Project:	Operativni Sistemi 2
// File: 	KernelFfile.cpp
// Date: 	July 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	KernelFile
#include <iostream>
#include "KernelFile.h"
#include "DataCluster.h"
#include "KernelFS.h"
#include "semafori.h"
#include "bitVector.h"

using namespace std;

CRITICAL_SECTION criticalKFS;

DataCluster* KernelFile::dataBuff[] = {
										NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
										NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
										NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
										NULL,NULL
									  };

KernelFile::KernelFile(ClusterNo clNo, BytesCnt size , char mod , char* fname , DataCluster& dc){
	this->firstDC		= clNo;
	this->size			= size;
	this->mod			= mod;
	this->fname = new char[16];
	this->fname			= fname;
	this->dataCl		= &dc;
	this->pozicija		= 0;
	mutexFile = CreateSemaphore(NULL,1,1,NULL);
	InitializeCriticalSection(&criticalKFS);
	
}

KernelFile::~KernelFile(){
	
	sem_wait(mutexFile);

	EnterCriticalSection(&criticalKFS);
	
	delOpenedFromList();
	LeaveCriticalSection(&criticalKFS);

	KernelFS::drives[fname[0]-'A']->numOfOpenedFiles--;
	if(KernelFS::drives[fname[0]-'A']->numOfOpenedFiles == 0 ) {
		sem_signal(KernelFS::drives[fname[0]-'A']->freeToFormat);	// ovde signalizira da je moguce formatirati particiju posto nema otvorenih vise na njoj
	
	}
	ListOfFiles::delItem(this->fname,GetCurrentThreadId());
	sem_signal(mutexFile);

	// ZATVARANjE FAJLA

}

void KernelFile::delOpenedFromList(){

	EnterCriticalSection(&criticalKFS);

	list<RW*>::iterator it;
	if(this)
	for(it=KernelFS::files->begin(); it!=KernelFS::files->end(); it++) {
			if (memcmp((*it)->fpath,fname ,strlen(fname)) == 0 /*&& (*it)->part>='A'*/) {	// nadjem koji fajl zelim da zatvorim, delete
				if (this->mod == 'r'){  
					(*it)->endRead(); 
					if((*it)->numOfThreads == 0 ){
						sem_signal((*it)->freeToDel);
						(*it)->part=-1;						
					}
					
				}			
				else{
					(*it)->endWrite();
					if((*it)->numOfThreads == 0 ){
						sem_signal((*it)->freeToDel);
						
						(*it)->part=-1;
						
					}
					
				}
				
			}
		}
	

	LeaveCriticalSection(&criticalKFS);
	sem_signal(mutexFile);
}

char KernelFile::write(BytesCnt n, char* buffer){

	sem_wait(mutexFile);

	if( n<= 0 || mod == 'r' ){ 
		
		//cout<<"Nije dozvoljen upis jer ili je mod R ili je n<0."<<endl; 
	
		sem_signal(mutexFile);
		return 0;	}

	if( this->size + n > MAX_SIZE){	
		
		//cout<<"Prekoracena velicina fajla."<<endl; 
		
		sem_signal(mutexFile);
		return 0; }

	dataCl->readFieldFD(firstDC);	// ovime se ucitava indeksni kaslter

	dataCl->CurrField = 0;			// od pocetka
	dataCl->SecondLevelField = 0;	// od pocetka

	
	BytesCnt preostalo=n;
	BytesCnt izBuff=0;
	BytesCnt naDisk=0;

	dataCl->getSLF(pozicija);
	
	while(preostalo){
		
		if(dataCl->CurrField < 256){
			
			if(dataCl->fields[dataCl->CurrField] == 0 ){	// ukoliko jeste potrebno je zauzeti novi klaster
			
				dataCl->fields[dataCl->CurrField] = dataCl->drive->getFreeCluster();	// dodaje slobodni klaster
				if(dataCl->fields[dataCl->CurrField] != 0)
				dataCl->writeFieldTD(firstDC);
				else {
					sem_signal(mutexFile);
					return 0;
				}	// nema slobodnih klastera
			
			}

			dataCl->readFieldFD(firstDC);		
			
			BytesCnt zaUpis = 0;
			if(preostalo > 2048 ) zaUpis = 2048;
			else zaUpis = preostalo;
			if(zaUpis==0) zaUpis=1;

			naDisk = pozicija % 2048;
			if(zaUpis==2048 && naDisk>0)
				zaUpis=2048-naDisk;

			size += zaUpis;

			


			if(zaUpis<2048) 
				this->dataCl->readDataFD(dataCl->fields[dataCl->CurrField]);
			
			memcpy(dataCl->cluster+naDisk ,buffer+izBuff,zaUpis);
			dataCl->writeDataTD(dataCl->fields[dataCl->CurrField]);
			
			izBuff = izBuff + zaUpis;
			preostalo-=zaUpis;
			pozicija+=zaUpis;

			if( pozicija % 2048 == 0 ) dataCl->CurrField++;


		}
		else if(dataCl->CurrField < 512){
						
			dataCl->readFieldFD(firstDC);	

			if(dataCl->SecondLevelField >= 511 || dataCl->SecondLevelField ==0 )
			if(dataCl->fields[dataCl->CurrField] == 0 ){	// ukoliko je potrebno je zauzeti novi klaster
				dataCl->readFieldFD(firstDC);	
				dataCl->fields[dataCl->CurrField] = dataCl->drive->getFreeCluster();	// dodaje slobodni klaster
				if(dataCl->fields[dataCl->CurrField] != 0)
				dataCl->writeFieldTD(firstDC);
				else {
					sem_signal(mutexFile);
					return 0;}	// nema slobodnih klastera
											
			}
			
			dataCl->readField2FD(dataCl->fields[dataCl->CurrField]);		// ucitavaju se fields2
					
			if(dataCl->fields2[dataCl->SecondLevelField] == 0){		//	ukoliko je potrebno zauzeti SLC
				
				dataCl->fields2[dataCl->SecondLevelField] = dataCl->drive->getFreeCluster();				
				if(dataCl->fields2[dataCl->SecondLevelField] != 0)
				dataCl->writeField2TD(dataCl->fields[dataCl->CurrField]);
				else {
					sem_signal(mutexFile);
					return 0;}	// nema slobodnih klastera
				
			}
			
						
			BytesCnt zaUpis = 0;
			if(preostalo > 2048 ) zaUpis = 2048;
			else zaUpis = preostalo;
			if(zaUpis==0) zaUpis=1;

			size += zaUpis;
			if(zaUpis<2048)
			naDisk = (pozicija-256*512)%2048;
			else naDisk=0;
			
			if(zaUpis<2048) this->dataCl->readDataFD(dataCl->fields2[dataCl->SecondLevelField]);

			memcpy(dataCl->cluster+naDisk ,buffer+izBuff,zaUpis);
			dataCl->writeDataTD(dataCl->fields2[dataCl->SecondLevelField]);
			
			izBuff = izBuff + zaUpis;
			preostalo-=zaUpis;
			pozicija+=zaUpis;

			if( naDisk >= 2048 || zaUpis==2048) {
				naDisk = 0;
				dataCl->SecondLevelField++;
				}
				
			
	}
		
		if( dataCl->SecondLevelField > 511){ dataCl->SecondLevelField=0; dataCl->CurrField++;}
	}

	if(dataCl->drive->partition)
	this->dataCl->drive->updateEntrySize(fname,size);
	
	sem_signal(mutexFile);

}

BytesCnt KernelFile::read(BytesCnt n, char* buffer){
	
	sem_wait(mutexFile);

	if( n<= 0 || pozicija > size){ 
		cout<<"Nije dozvoljeno citanje jer ili je mod W ili je n<0."<<endl;
		sem_signal(mutexFile);
		return 0; }
	if( size < n ) { 
		
		//cout<<"Nije moguce procitati "<<n<<" bajtova zato sto je velicina fajla "<<size<<" manja";
		//cout<<"sledi citanje do maksimalne velicine fajla."<<endl; 
		
		n = size; 
	}
	
	this->dataCl->readFieldFD(firstDC);	 
	dataCl->CurrField = 0;
	dataCl->SecondLevelField = 0;
	BytesCnt preostalo = n;
	BytesCnt buffOffset = 0;
	BytesCnt pozicijaSaDiska = 0;
	dataCl->getSLF(pozicija);

	while(preostalo){

		if( dataCl->CurrField < 256 ){
			if( dataCl->fields[dataCl->CurrField] != 0 ){	
						
				BytesCnt zaPrepis = 0;
				if(preostalo >= 2048 ) zaPrepis = 2048;
				else zaPrepis = preostalo;
				if(zaPrepis==0) zaPrepis=1;

				pozicijaSaDiska = pozicija % 2048;

				dataCl->readDataFD(dataCl->fields[dataCl->CurrField]);
				memcpy(buffer+buffOffset,dataCl->cluster+pozicijaSaDiska ,zaPrepis);
								
				buffOffset += zaPrepis;
				preostalo  -= zaPrepis;
				pozicija   += zaPrepis;

				if(pozicijaSaDiska % 2048 == 0) dataCl->CurrField++;

			}else{ sem_signal(mutexFile); return pozicija;}

		}else if(dataCl->CurrField<512){
			
			dataCl->readField2FD(dataCl->fields[dataCl->CurrField]);
			if( dataCl->fields2[dataCl->SecondLevelField] !=0 ){
				
				BytesCnt zaPrepis = 0;
				if(preostalo >= 2048 ) zaPrepis = 2048;
				else zaPrepis = preostalo;
				if(zaPrepis==0) zaPrepis=1;

				if(zaPrepis<2048)
				pozicijaSaDiska = (pozicija-256*512)%2048;
				else pozicijaSaDiska=0;

				dataCl->readDataFD(dataCl->fields2[dataCl->SecondLevelField]);
				memcpy(buffer+buffOffset,dataCl->cluster+pozicijaSaDiska ,zaPrepis);
								
				buffOffset += zaPrepis;
				preostalo  -= zaPrepis;
				pozicija   += zaPrepis;

				if(pozicijaSaDiska%2048 == 0) dataCl->SecondLevelField++;
				if(dataCl->SecondLevelField > 511) {
					dataCl->CurrField ++;
					dataCl->SecondLevelField = 0;

				}
			
			}else{ sem_signal(mutexFile); return pozicija;}
		}
			
		}	
	sem_signal(mutexFile);
	return pozicija;
}

char KernelFile::truncate(){

	sem_wait(mutexFile);

	if( mod == 'r' ){	// samo onaj koji upisuje moze da radi truncate 
		sem_signal(mutexFile);
		return 0;	
	}

	dataCl->readFieldFD(firstDC);	// ovime se ucitava indeksni kaslter

	dataCl->CurrField = 0;			// od pocetka
	dataCl->SecondLevelField = 0;	// od pocetka

	
	BytesCnt preostalo=size-pozicija;

	if(preostalo < 0){
		sem_signal(mutexFile);
		return 0;	
	}

	BytesCnt naDisk=0;
	
	char* buffer = new char[2048];	// buffer popunjen nulama
	memset(buffer,NULL,2048);

	bool prvi = true;

	dataCl->getSLF(pozicija);

	bool	imaJosZaBrisanje = true;
	ClusterNo sledeciZaBrisanje;
	
	while(imaJosZaBrisanje){
		
		if(dataCl->CurrField < 256){
			
			dataCl->readFieldFD(firstDC);		
			if(dataCl->fields[dataCl->CurrField+1]==0){ imaJosZaBrisanje = false;}
			else sledeciZaBrisanje = dataCl->fields[dataCl->CurrField+1];

			BytesCnt zaUpis=0; 
				
			if(prvi){	// uzme od prvog klastera koliko mu je ostalo do kraja
				zaUpis = 2048 - (pozicija%2048);
				prvi=false;
			}else zaUpis = 2048;
			
			naDisk = pozicija % 2048;
			
			size -= zaUpis;

			if(zaUpis<2048) 
				this->dataCl->readDataFD(dataCl->fields[dataCl->CurrField]);
			memcpy(dataCl->cluster+naDisk ,buffer,zaUpis);
			dataCl->writeDataTD(dataCl->fields[dataCl->CurrField]);	// upisao nule
			
			pozicija+=zaUpis;	// za brisanje

			if(zaUpis==2048){
			dataCl->drive->bitV->freeCluster(dataCl->fields[dataCl->CurrField]); // oslobadja 
			dataCl->fields[dataCl->CurrField] = 0;
			}
			if( imaJosZaBrisanje ) dataCl->CurrField++;


		}
		else if(dataCl->CurrField < 512){

			BytesCnt zaUpis=0; 
				
			if(prvi){	// uzme od prvog klastera koliko mu je ostalo do kraja
				zaUpis = 2048 - (pozicija%2048);
				prvi=false;
			}else zaUpis = 2048;

						
			dataCl->readField2FD(dataCl->fields[dataCl->CurrField]);
			if( dataCl->fields2[dataCl->SecondLevelField] !=0 ){
				
				naDisk = pozicija % 2048;
			
				size -= zaUpis;

				if(zaUpis<2048) 
					this->dataCl->readDataFD(dataCl->fields2[dataCl->SecondLevelField]);
				memcpy(dataCl->cluster+naDisk ,buffer,zaUpis);
				dataCl->writeDataTD(dataCl->fields2[dataCl->SecondLevelField]);	// upisao nule
			
				pozicija+=zaUpis;	// za brisanje

				if(zaUpis==2048){
				dataCl->drive->bitV->freeCluster(dataCl->fields2[dataCl->SecondLevelField]); // oslobadja 
				dataCl->fields2[dataCl->SecondLevelField] = 0;
				}

				if(dataCl->fields2[dataCl->SecondLevelField+1]==0 && dataCl->SecondLevelField+1 <= 511){ 
					imaJosZaBrisanje = false;
				}

				if( imaJosZaBrisanje ){
				
					dataCl->SecondLevelField++;
					if(dataCl->SecondLevelField > 511) {
						dataCl->CurrField ++;
						dataCl->SecondLevelField = 0;

					}
				}
			
			}
		}
		if( dataCl->SecondLevelField > 511){ dataCl->SecondLevelField=0; dataCl->CurrField++;}
	}

	if(dataCl->drive->partition){
		pozicija = size;	// postavlja poziciju na poslednji
		this->dataCl->drive->updateEntrySize(fname,size);
	
	}
	
	sem_signal(mutexFile);
}

char KernelFile::seek(BytesCnt n){
	sem_wait(mutexFile);
	if( n < 0 || n > size ){ sem_signal(mutexFile); return 0; }	//neuspeh
	pozicija = n;
	sem_signal(mutexFile);
	return 1;							// uspeh
}


char KernelFile::eof(){
	sem_wait(mutexFile);
	if(pozicija == size){	sem_signal(mutexFile);	return 2;}	// na kraju fajla
	if(pozicija < 0 || size < 0 ){ sem_signal(mutexFile);	return 1;}	// greska
	if(pozicija <  size){	sem_signal(mutexFile);	return 0;}	// nije na kraju fajla	
}

