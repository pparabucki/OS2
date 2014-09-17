// Project:	Operativni Sistemi 2
// File: 	KernelFS.cpp
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	KernelFS
#include <iostream>
#include "KernelFS.h"
#include "Drive.h"
#include "bitVector.h"
#include "rootCluster.h"
#include "KernelFile.h"
#include "DataCluster.h"
#include "semafori.h"

using namespace std;


Drive* KernelFS::drives[26] =	{
								NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
								NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
								NULL,NULL,NULL,NULL,NULL,NULL
								};

short int KernelFS::currEntryInBuffer = 0;

bool KernelFS::pronadjen = false;

list<RW*>* KernelFS::files = new list<RW*>;
HANDLE KernelFS::KFSmutex = CreateSemaphore(NULL,1,1,NULL); 
HANDLE KernelFS::mountMutex = CreateSemaphore(NULL,1,1,NULL);
HANDLE KernelFS::KernelDEL = CreateSemaphore(NULL,1,1,NULL);

CRITICAL_SECTION critKFS;

KernelFS::KernelFS(){

	sem_wait(KFSmutex);

	// Upis nula u buffer velicine 64
		char nulaa[8];
		memset(nulaa,NULL,sizeof(nulaa));

		unsigned int nula = 0 ;
		
		for(int i=0; i < 64 ;i++){
			/* name */	memcpy(buffer[i].name, &nulaa  ,sizeof(char)*9);
			/* ext  */	memcpy(buffer[i].ext , &nulaa  ,sizeof(char)*4);
			/* res  */	buffer[i].reserved		= 0;
			/* first*/	buffer[i].firstCluster	= 0;
			/* size */	buffer[i].size			= 0;
			
			}
		openedFiles = new ListOfFiles();
		f	= NULL;
		InitializeCriticalSection(&critKFS);
		sem_signal(KFSmutex);
}

KernelFS::~KernelFS(){
	DeleteCriticalSection(&critKFS);
}

char KernelFS::mount(Partition* partition){
	
	sem_wait(mountMutex);
	
	int i=0;
	while(drives[i] != NULL && i<26) i++;

	if(i==26){
		//cout<<"Nije moguce montirati particiju , zato sto nema slobodnih mesta!"<<endl; 
		sem_signal(mountMutex);
		
		return '0';
	}	// nemoguce je dodeliti particiju

	drives[i] = new Drive(partition,'A'+i);

	//cout<<"Montirana je particija "<<char('A'+i)<<" sadrzi "<<drives[i]->partition->getNumOfClusters()<<" klastera."<<endl;
	
	sem_signal(mountMutex);
	
	return 'A'+i;
}

char KernelFS::unmount(char partition){
	//SEMAFORI
	
	sem_wait(mountMutex);
	
	if(partition < 'A' || partition > 'Z') { 
	
		//cout<<"Pogresna labela particije!"<<endl;
		
		sem_signal(mountMutex);

		return 0; }
	// UBACITI PROVERU ZA OTVORENE FAJLOVE
	
	if(drives[partition-'A']){
		int brojKl = drives[partition-'A']->partition->getNumOfClusters();
		free(drives[partition-'A']);
		drives[partition-'A'] = NULL;
		
		//cout<<"Particija "<<char(partition)<<" je uspesno demontirana, sadrzi "<<brojKl<<" klastera."<<endl;
		
		sem_signal(mountMutex);

		return 1;
	}
	else{
		//cout<<"Particija "<<partition<<" nije bila montirana, tako da je nemoguce demontirati je."<<endl;
	}
	
	sem_signal(mountMutex);

	return 0;
}

char KernelFS::format(char partition){
	//SEMAFORI

	sem_wait(mountMutex);

	// PROVERITI DA LI NEKO NIJE ZATVORIO SVE FAJLOVE

	if(partition < 'A' || partition > 'Z') { 
		
		//cout<<"Pogresna labela particije!"<<endl;
		
		sem_signal(mountMutex);
		return 0; 
	}
	if(drives[partition-'A'] == NULL )	{ 
		
		//cout<<"Data particija nije montirana!"<<endl; 
		
		sem_signal(mountMutex);
		return 0;	
	}

	if (drives[partition-'A']->numOfOpenedFiles > 0) {
		//cout<<"***Nije moguc format "<<partition<<" particije, postoje otvoreni fajlovi nad particijom!***"<<endl;
		sem_wait(drives[partition-'A']->freeToFormat);        
	}

	drives[partition-'A']->bitV->resetBV();

	drives[partition-'A']->rootCluster->resetRC();		
	
	sem_signal(mountMutex);

	return 1;
}

char KernelFS::readRootDir(char part, EntryNum n, Directory &d){

	
	if(part<'A' || part >'Z')		{
		
		//cout<<"Nije uneta ispravana labela particije."<<endl; 
		return 0;
	}
	if(drives[part-'A'] == NULL )	{
		//cout<<"Data particija nije montirana."<<endl;
		return 0;
	}

	char ret = drives[part-'A']->readRootDir(n,d);

	return ret;

}	

char KernelFS::doesExist(char* fname){

	EnterCriticalSection(&critKFS);

	char part;
	char* fnam = new char[8]();	//	setuje fnam sve na null
	char* fext = new char[3]();	//	setuje fext sve na null
		
	decodeName(fname,part,fnam,fext);
	
	if(part<'A' || part >'Z')		{
		//cout<<"Nije uneta ispravana labela particije."<<endl; 
		LeaveCriticalSection(&critKFS);
		return 0;
	}
	if(drives[part-'A'] == NULL )	{
		//cout<<"Data particija nije montirana."<<endl;
		LeaveCriticalSection(&critKFS);
		return 0;
	}

	
	int continueToSearch=65;	// ako je vrednost 65 nastavlja pretragu, ako je manje - prekida
	ClusterNo	currEntry=0; 
	
	while(continueToSearch==65){
		continueToSearch = this->readRootDir(part,currEntry,buffer);
		if(continueToSearch==0) break;
		int granica=0;
		if(continueToSearch == 65) granica =  continueToSearch - 1;
		else granica = continueToSearch;
		for(int i = 0; i < granica; i++){
			int j = 0;
			while(j<8){
				if(buffer[i].name[j] == fnam[j]) j++;
				else { break;}
				if(j==8){
					j=0;
					while(j<3){
						if(buffer[i].ext[j] == fext[j]) j++;
						else { j=8; break;}
						if(j==3){
							currEntryInBuffer = i;
							LeaveCriticalSection(&critKFS);
							return 1;
						}
					}
				}
			}
			//*/
		}
		if( continueToSearch == 65 )
		currEntry = currEntry + continueToSearch -1 ;	// dohvata sledecih 64  
	
	}
	//cout<<"ERROR: Fajl "<<fname<<" nije pronadjen!"<<endl;
	LeaveCriticalSection(&critKFS);
	return 0;
}	

void KernelFS::updateSizeOfFile(char* fname , BytesCnt size){

	drives[fname[0]-'A']->updateEntrySize(fname,size);
	
}

File& KernelFS::open(char* fname, char mode){
	
	//SEMAFORI

	sem_wait(KFSmutex);


	if(fname[0]<'A' || fname[0] >'Z')		{
		//cout<<"Nije uneta ispravana labela particije."<<endl; 
		f=NULL;
		sem_signal(KFSmutex);
		return *f;
	}
	if(drives[fname[0]-'A'] == NULL )	{
		//cout<<"Data particija nije montirana."<<endl;
		f=NULL;
		sem_signal(KFSmutex);
		return *f;
	}
	
	char part;
	char* fnam = new char[9]();	//	setuje fnam sve na null
	char* fext = new char[4]();	//	setuje fext sve na null
	
	decodeName(fname,part,fnam,fext);

	bool found = false;			// da li je ovaj fajl u listi otvorenih
	list<RW*>::iterator it;
	for(it = files->begin(); it != files->end(); it++)
		if (strcmp((*it)->fpath,fname) == 0 ){
			if((*it)->numOfThreads > 0 && (*it)->numR <= 0 && (*it)->part>='A'){	// ako je u toku upis , uspavaj se na odredjeno vreme
				Sleep(1000*(*it)->numOfThreads);
				it=files->begin();

			}else{
				found = true;
				break;    
			}                                                                                                               
	}
	if (!found) {                                           // ako nije kreira se nov objekat u otvorenim fajlovima <------
		files->push_back(new RW(fname, fname[0]));          // ubacuje el na kraj liste
		it = (--(files->end()));
	}       
	else (*it)->part = fname[0];
	

	
	if( (this->doesExist(fname))){ // da li postoji na disku

		Entry* e;	
		e = openedFiles->isItOpen(fname,mode,GetCurrentThreadId()); // da li se nalazi u listi otvorenih fajlova

		if( e->name[0] == 0 ){
			//cout<<"Nije otvoren sledi pravljenje tog fajla pomocu onoga sa diska"<<endl; 
			e = getEntry(fname);

			Entry ubaci;

			memcpy(&ubaci,e,20);
			f = new File();
			DataCluster* dc = new DataCluster(drives[fname[0]-'A'],ubaci.firstCluster,fname);
			f->myImpl = new KernelFile(ubaci.firstCluster, ubaci.size , mode , fname , *dc);
			
			openedFiles->addItem(fname,ubaci,GetCurrentThreadId(),mode);
		}
		pronadjen = true;
		if ( mode == 'r' ) {		
			
			(*it)->startRead();
			sem_signal(KFSmutex);
			return *f;

		}else if( mode == 'a'  ){
			(*it)->startWrite();

			f->seek(f->getFileSize());
			
			if(doesExist(fname)== 0){
				f=NULL;
				sem_signal(KFSmutex);
				return *f;
			} 
			sem_signal(KFSmutex);
			return *f;
		
		}else if(  mode == 'w'  ){	// ako postoji i treba novi upis
		
			sem_signal(KFSmutex);
			
			deleteFile(fname);
			
			(*it)->startWrite();

			Entry ubaci;
			strcpy(ubaci.name,fnam);
			strcpy(ubaci.ext,fext);
			ubaci.firstCluster = drives[part-'A']->getFreeCluster();	
			
			ubaci.reserved = 0;
			ubaci.size = 0;

			f = new File();
			DataCluster* dc = new DataCluster((drives[fname[0]-'A']),ubaci.firstCluster,fname);
			f->myImpl = new KernelFile(ubaci.firstCluster, ubaci.size , mode , fname , *dc);
			f->myImpl->dataCl->setDrive(*drives[fname[0]-'A']);

			drives[fname[0]-'A']->insertEntry(ubaci);
			openedFiles->addItem(fname,ubaci,GetCurrentThreadId(),mode);
			drives[fname[0]-'A']->numOfOpenedFiles++;
			
			sem_signal(KernelDEL);
			sem_signal(KFSmutex);
			return *f;
			
		}
		
	} else if( mode == 'w'  ){	// ako ne postoji i upis je
	
		// make new , ima prava da napravi novi samo onaj koji upisuje
		(*it)->startWrite();

		Entry ubaci;
		strcpy(ubaci.name,fnam);
		strcpy(ubaci.ext,fext);
		ubaci.firstCluster = drives[part-'A']->getFreeCluster();	
		ubaci.reserved = 0;
		ubaci.size = 0;

		f = new File();
		DataCluster* dc = new DataCluster(drives[fname[0]-'A'],ubaci.firstCluster,fname);
		f->myImpl = new KernelFile(ubaci.firstCluster, ubaci.size , mode , fname , *dc);
		
		drives[fname[0]-'A']->insertEntry(ubaci);
		openedFiles->addItem(fname,ubaci,GetCurrentThreadId(),mode);
		drives[fname[0]-'A']->numOfOpenedFiles++;
		sem_signal(KFSmutex);
		return *f;

	}	else {
		// u suprotnom vrati 0
		if(mode == 'r'){
			//cout<<"Nije moguce otvoriti fajl za citanje u modu 'r' zato sto ne postoji."<<endl;
		}else if(mode == 'a'){
			//cout<<"Nije moguce otvoriti fajl za citanje i upis u modu 'a' zato sto ne postoji."<<endl;
		}else{ 
			//cout<<"Pogresan mod je unet."<<endl;
		}
		pronadjen = false;
		f=NULL;
		sem_signal(KFSmutex);
		return *f;
	}
}

char KernelFS::deleteFile(char* fname){
	sem_wait(KFSmutex);
	sem_wait(KernelDEL);

	list<RW*>::iterator it;

	short maxTimes=0;

	for(it = files->begin(); it != files->end(); it++){
		if (strcmp((*it)->fpath,fname) == 0 && (*it)->part>='A'){
		if( (*it)->numOfThreads > 0 ){	// ako je otvoren potrebno je sacekati da se zatvori

		sem_signal(KFSmutex);	

		sem_wait((*it)->freeToDel);

		}
	}
	}
	
	
	if( this->doesExist(fname) ) {	// ako postoji na disku , obrisi ga
		
		drives[fname[0]-'A']->deleteEntry(fname);
		
				

	}else{
		//cout<<"ERROR: dati fajl ne postoji ili je otvoren, pa ga je nemoguce obrisati."<<endl;
		sem_signal(KFSmutex);
		return 0;
	}

	sem_signal(KFSmutex);
	return 0;
	
}

Drive* KernelFS::getDrive(char driveLetter) const{
	return 0;
}

Entry* KernelFS::makeEntry(char *fname){
	return 0;
}



Entry* KernelFS::getEntry(char* fname){

	EnterCriticalSection(&critKFS);
	
	char part;
	char* fnam = new char[8]();	//	setuje fnam sve na null
	char* fext = new char[3]();	//	setuje fext sve na null

	
	decodeName(fname,part,fnam,fext);

	if(part<'A' || part >'Z')		{
		//cout<<"Nije uneta ispravana labela particije."<<endl; 
		LeaveCriticalSection(&critKFS);
		return 0;
	}
	if(drives[part-'A'] == NULL )	{
		//cout<<"Data particija nije montirana."<<endl;
		LeaveCriticalSection(&critKFS);
		return 0;
	}
	int continueToSearch=65;	// ako je vrednost 65 nastavlja pretragu, ako je manje - prekida
	ClusterNo	currEntry=0; 
	
	while(continueToSearch==65){
		continueToSearch = this->readRootDir(part,currEntry,buffer);
		if(continueToSearch==0) break;
		int granica=0;
		if(continueToSearch == 65) granica =  continueToSearch - 1;
		else granica = continueToSearch;
		for(int i = 0; i < granica; i++){
			int j = 0;
			while(j<8){
				if(buffer[i].name[j] == fnam[j]) j++;
				else { break;}
				if(j==8){
					j=0;
					while(j<3){
						if(buffer[i].ext[j] == fext[j]) j++;
						else { j=8; break;}
						if(j==3){
							currEntryInBuffer = i;
							LeaveCriticalSection(&critKFS);
							return &buffer[i];
						}
					}
				}
			}
			//*/
		}
		if( continueToSearch == 65 )
		currEntry = currEntry + continueToSearch -1 ;	// dohvata sledecih 64  
	
	}
	LeaveCriticalSection(&critKFS);
	return 0;

}