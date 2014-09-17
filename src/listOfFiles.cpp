// Project:	Operativni Sistemi 2
// File: 	listOfFiles.cpp
// Date: 	August 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	ListOfFiles

#include "listOfFiles.h"
#include "testprimer.h"
#include "semafori.h"

#include <iostream>

using namespace std;

Elem* ListOfFiles::head= NULL;         // Pokazivac na pocetak liste.
Elem* ListOfFiles::curr= NULL;         // Pokazivac na tekuci element liste.
Elem* ListOfFiles::tail= NULL;
int ListOfFiles::cnt = 0;
HANDLE ListOfFiles::mutexList = CreateSemaphore(NULL,1,1,NULL);	// semafor koji dozvoljava samo jedu nit u listi

CRITICAL_SECTION criticalLOF;

ListOfFiles::ListOfFiles(){
	memset(&e,NULL,20);
	InitializeCriticalSection(&criticalLOF);
	
}

ListOfFiles::~ListOfFiles(){
	delAll();
	CloseHandle(mutexList);
	DeleteCriticalSection(&criticalLOF);
}

int ListOfFiles::count(){
	return cnt;
}

void ListOfFiles::printList(){
	EnterCriticalSection(&criticalLOF);
	sem_wait(mutexList);	
	curr = head;
	while(curr){
		//cout<<" Fname: "<<curr->fname<<endl<<" FSize : "<<curr->getSize()<<endl<<" FirstDC: "<<curr->getFC()<<endl;
		curr = curr->next;
	}
	sem_signal(mutexList);
	LeaveCriticalSection(&criticalLOF);
}

void ListOfFiles::addItem(char* newFile,Entry e , int tID , char mod){
	EnterCriticalSection(&criticalLOF);
	sem_wait(mutexList);

	Elem *novi = new Elem(newFile,e,tID,mod);

	
	if( head == NULL ){
		
		head =  novi;
		head->prev = NULL;
		head->next = NULL;
		tail = head;
		
	}
	else if(tail != NULL){
		
		tail->next = novi;
		novi->prev = tail;
		tail = novi;
		tail->next = NULL;
	
	}
	
	cnt++;
	sem_signal(mutexList);
	
	LeaveCriticalSection(&criticalLOF);
 }

Entry* ListOfFiles::isItOpen(char* fname,char mod , int tID){
	EnterCriticalSection(&criticalLOF);
	sem_wait(mutexList);
	int maxTime=0;
	curr = head;
	bool prom;
	while(curr){
		prom = false;
		if( memcmp(curr->fname , fname , sizeof(fname)) == 0 ){
			
			if( curr->mode==mod && curr->threadID == tID){	// ovo je dodato 21.08.2014
			char part;
			char* fnam = new char[9]();	//	setuje fnam sve na null
			char* fext = new char[4]();	//	setuje fext sve na null
			
			
			decodeName(fname,part,fnam,fext);

			for(int i=0;i<8;i++)
			e.name[i]=fnam[i];
			for(int i=0;i<3;i++)
			e.ext[i] = fext[i];
			e.reserved = 0;
			e.firstCluster = curr->firstCluster;			
			e.size = curr->size;

			sem_signal(mutexList);
			LeaveCriticalSection(&criticalLOF);
			return &e;		// ako je pronasao entry vraca ga
			}
			else{ 
				curr = curr->next; 
				prom=true;
			}
		}
		if(!prom) 
			curr = curr->next;
			
	}
	memset(&e.name, NULL , 8);

	sem_signal(mutexList);
	LeaveCriticalSection(&criticalLOF);
	return &e;	// ne postoji
}

bool ListOfFiles::isItOpenBool(char* fname){
	EnterCriticalSection(&criticalLOF);
	sem_wait(mutexList);

	curr = head;
	
	while(curr){
	
		if( memcmp(curr->fname , fname , sizeof(fname)) == 0 ){
			
			sem_signal(mutexList);
			LeaveCriticalSection(&criticalLOF);
			return true;		// ako je pronasao entry vraca ga
			
		}
		else curr = curr->next;
		
	}
	
	
	sem_signal(mutexList);
	LeaveCriticalSection(&criticalLOF);
	return false;	// ne postoji
}

bool ListOfFiles::delItem(char* delFile, int tID){
	EnterCriticalSection(&criticalLOF);
	sem_wait(mutexList);

	Elem *zaBrisanje;
	Elem *prethodni;
	Elem *sledeci; 	
 	
	zaBrisanje=NULL;
	prethodni =NULL;
	sledeci	  =NULL;
	
	curr=head;
		
	while(curr){
		if( strcmp(curr->fname , delFile ) == 0 && curr->threadID == tID){
			if(curr->prev == NULL){					// znaci radi se o prvom
				if(curr->next != NULL){				// ima jos
				head = curr->next;					
				head->prev = NULL;
				zaBrisanje = curr;
				curr=NULL;
				free(zaBrisanje);
				}else{								// jedini je
				head = NULL;
				tail = NULL;
				zaBrisanje = curr;
				curr=NULL;
				free(zaBrisanje);
				}
			}else if(curr->prev != NULL && curr->next != NULL){	// znaci radi se o srednjem
				prethodni = curr->prev;
				sledeci = curr->next;
				prethodni->next = sledeci;
				sledeci->prev = prethodni;
				zaBrisanje = curr;
				curr=NULL;
				free(zaBrisanje);

			}else if(curr->next == NULL){			// znaci radi se o poslednjem
				tail = curr->prev;
				tail->next = NULL;
				zaBrisanje = curr;
				curr=NULL;
				free(zaBrisanje);

			}
			
			sem_signal(mutexList);
			LeaveCriticalSection(&criticalLOF);
			return true;	// uspesno je obrisan iz liste
			
		}
		else curr = curr->next;
		
	}
	//cout<<"Nuje u listi otvorenih fajlova "<<delFile<<" curr->threadID : "<<tID<<endl;
	
	sem_signal(mutexList);
	LeaveCriticalSection(&criticalLOF);
	return false;	// nije izbrisan iz liste (jer mozda ne postoji ili ga nije ta nit otvorila)
 	
}

void ListOfFiles::delAllFin(){
			
}


void ListOfFiles::delAll(){
	EnterCriticalSection(&criticalLOF);
	sem_wait(mutexList);

	curr = head;

	while(curr){

		Elem* erese;
		erese=curr;
		curr=curr->next;
		delete erese;
		head=curr;
	}
	if(curr==tail){
			delete head;
			delete curr;
			delete tail;
	}
	cnt=0;

	sem_signal(mutexList);
	LeaveCriticalSection(&criticalLOF);
}
