// Project:	Operativni Sistemi 2
// File: 	RootCluster.h
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	RootCluster	

#include "rootCluster.h"
#include "KernelFS.h"
#include "bitVector.h"


using namespace std;
CRITICAL_SECTION criticalRC;

// RC nasledjuje od Cluster i zauzima prvi cluster na disku
RootCluster::RootCluster(Drive* d):Cluster(d,one){
	CurrField		  =	0;
	FirstLevelEntry   =	0;
	SecondLevelField  =	0;
	SecondLevelEntry  =	0;
	rootMutex = CreateSemaphore(NULL,1,10,NULL);
	InitializeCriticalSection(&criticalRC);
}

void RootCluster::resetRC(){
	EnterCriticalSection(&criticalRC);
	
	int  numOfClusters = partition->getNumOfClusters();
	char emptyCluster [2048];
	memset(emptyCluster, 0, sizeof emptyCluster);
	for(int i=1;i<numOfClusters;i++) partition->writeCluster(i,emptyCluster);	// poicinje od 1 zbog bitV

	LeaveCriticalSection(&criticalRC);

}

RootCluster::~RootCluster(){
	CloseHandle(rootMutex);
	delete [] fields;
	delete [] fields2;
	DeleteCriticalSection(&criticalRC); 
}

int RootCluster::putEntry(Entry entry){

	EnterCriticalSection(&criticalRC);

	ClusterNo provera;

	readFieldFD(one);	// citanje RootCluster-a

	while( fields[CurrField]){
		if((CurrField+1<512) && fields[CurrField+1]) CurrField++;
		else break;
	}

	readEntryFD(fields[CurrField]);
	
	if(fields[CurrField]){
	FirstLevelEntry=0;
	while( entries[FirstLevelEntry].name[0]){
				FirstLevelEntry++;
	}
	}
	
	if(CurrField < 256 ){
		if( fields[CurrField] == 0 )	// treba zauzeti jedan klaster
		{
			provera = drive->getFreeCluster();	// ucita u field novi klaster
			if(provera)	fields[CurrField] = provera;
			else{ LeaveCriticalSection(&criticalRC); return 0; }
			
			resetEntry();
			
			FirstLevelEntry = 0;

			entries[FirstLevelEntry++] = entry;

			writeEntryTD(fields[CurrField]);		// upisuje sve nule u novi kaster za Entry-e
													// s tim sto se na prvom entry-u nalazi novi
													// entry

			
		}
		else if( FirstLevelEntry < 102){
			
			readEntryFD(fields[CurrField]);
			entries[FirstLevelEntry++] = entry;		
			writeEntryTD(fields[CurrField]);

		}else {
			CurrField++;

			if(CurrField==256){
				
				SecondLevelField = 0;
				SecondLevelEntry = 0;

				provera = drive->getFreeCluster();						// uzima novi klaster za field2
				if(provera) fields[CurrField] = provera;
				else{ LeaveCriticalSection(&criticalRC); return 0; }
									
				readField2FD(fields[CurrField]);						// ucitan novi klaster u field2
				
				provera = drive->getFreeCluster();	
				if(provera) fields2[SecondLevelField] = provera;		// za entry
				else { LeaveCriticalSection(&criticalRC); return 0;}
				
				resetEntry();

				entries[SecondLevelEntry++] = entry;

				writeEntryTD(fields2[SecondLevelField]);

				writeFieldTD(one);
				writeField2TD(fields[CurrField]);
				LeaveCriticalSection(&criticalRC);
				return 1;

				

			}

			FirstLevelEntry = 0;

			provera = drive->getFreeCluster();
			if(provera) fields[CurrField] = provera;	// ucita u field novi klaster
			else { LeaveCriticalSection(&criticalRC); return 0;}

			resetEntry();
			
			FirstLevelEntry = 0;

			entries[FirstLevelEntry++] = entry;

			writeEntryTD(fields[CurrField]);
		}

	}
	else if( CurrField >255 && CurrField <512 ){
		SecondLevelField=0;
		readField2FD(fields[CurrField]);
		while( fields2[SecondLevelField]){
		if( (SecondLevelField+1<512) && fields2[SecondLevelField+1]) {
			SecondLevelField++;
		}
		else break;
		}

		readEntryFD(fields2[SecondLevelField]);
		while( entries[SecondLevelEntry].name[0]){
					SecondLevelEntry++;
		}
		if ( SecondLevelEntry < 102 ){
			entries[SecondLevelEntry++] = entry;
			writeEntryTD(fields2[SecondLevelField]);
		}
		else{
		
			SecondLevelField++;

			if(SecondLevelField==512){
				CurrField++;

				SecondLevelField = 0;
				SecondLevelEntry = 0;

				provera = drive->getFreeCluster();						// uzima novi klaster za field2
				if(provera) fields[CurrField] = provera;
				else { LeaveCriticalSection(&criticalRC); return 0;}
				
				readField2FD(fields[CurrField]);						// ucitan novi klaster u field2
				
				
				provera = drive->getFreeCluster();	
				if(provera) fields2[SecondLevelField] = provera;		// za entry
				else { LeaveCriticalSection(&criticalRC); return 0;}
				
				resetEntry();

				entries[SecondLevelEntry++] = entry;

				writeEntryTD(fields2[SecondLevelField]);

				writeFieldTD(one);
				writeField2TD(fields[CurrField]);
				LeaveCriticalSection(&criticalRC);
				return 1;

			}

			SecondLevelEntry = 0;

			provera = drive->getFreeCluster();	
			if(provera) fields2[SecondLevelField] = provera;		
			else{ LeaveCriticalSection(&criticalRC); return 0;}
			
			resetEntry();

			entries[SecondLevelEntry++] = entry;

			writeEntryTD(fields2[SecondLevelField]);
			writeField2TD(fields[CurrField]);
		}
	}
	else{ LeaveCriticalSection(&criticalRC); return 0;}
	this->writeFieldTD(one);
	LeaveCriticalSection(&criticalRC);
	return 1;
}

int RootCluster::deleteEntry(char* fname){

	EnterCriticalSection(&criticalRC);
	
	char part;
	char* fnam = new char[8]();	//	setuje fnam sve na null
	char* fext = new char[3]();	//	setuje fext sve na null

	decodeName(fname,part,fnam,fext);

	ClusterNo clusterFirst=0 , clusterSecond=0 , entry = 0;	// polja u klasterima prvog nivoa , i drugog nivoa
	EntryNum  poslednjiFLField = 0;
	EntryNum  poslednjiSLField = 0;
	EntryNum  poslednjiEntry = 0;
	unsigned int num=0;
	Entry poslednji;
	ClusterNo pocetniKlaster = 0;


	readFieldFD(one);	// citanje RootCluster-a

	while(true){
	
		if( fields[clusterFirst] != 0 && clusterFirst <=255){
			readEntryFD(fields[clusterFirst]);
			for(entry=0;entry<102;entry++){
				if( memcmp(&entries[entry].name,fnam,8*sizeof(char)) == 0 ){
					if( memcmp(&entries[entry].ext,fext,3*sizeof(char)) == 0 ){
						pocetniKlaster = entries[entry].firstCluster;
												
						poslednjiFLField = clusterFirst;
						while(fields[poslednjiFLField]){		// postavlja nas na poslednji zauzet field
							if(fields[poslednjiFLField + 1] != 0)
								poslednjiFLField++;
							else break;
						}

						
						if(poslednjiFLField<256){
							this->readEntryFD(fields[poslednjiFLField]);
							while(entries[poslednjiEntry].name[0] !=0 ){
								if(entries[poslednjiEntry+1].name[0] != 0 && poslednjiEntry<101)
								poslednjiEntry++;
								else 
									break;
							}
						}else if(poslednjiFLField < 512 ){
							
							this->readField2FD(fields[poslednjiFLField]);
							while(fields2[poslednjiSLField]){
								if(fields2[poslednjiSLField + 1] != 0)
									poslednjiSLField++;
								else break;
							}
							this->readEntryFD(fields2[poslednjiSLField]);
							while(entries[poslednjiEntry].name[0] !=0 ){
								if(entries[poslednjiEntry+1].name[0] != 0)
								poslednjiEntry++;
								else break;
							}
						}

						if( poslednjiFLField < 256){							

							if(poslednjiEntry){	// Entry > 0

								this->readEntryFD(fields[poslednjiFLField]);
								

								if(entry!=poslednjiEntry || poslednjiFLField != clusterFirst )
								memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
								else 
								memset(&poslednji,NULL,20);
								
								memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

								writeEntryTD(fields[poslednjiFLField]);		// upisuje oslobodjenu poslednju lokaciju
								
								readFieldFD(one);							// cita fields sa diska
								readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin

								
								memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
								writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

								deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo
								LeaveCriticalSection(&criticalRC);
								return 1;

							}else{	// Entry = 0
								
								pocetniKlaster = entries[poslednjiEntry].firstCluster;
								if(poslednjiFLField == 0 ){							// znaci da je to ujedno i poslednji entry uopste
									deleteFileClusters(entries[0].firstCluster);	// brise iz RC poslednji klaster koji je za entry
									
									readFieldFD(one);
									this->resetEntry();
									this->drive->bitV->freeCluster(fields[0]);
									writeEntryTD(fields[0]);

									fields[poslednjiFLField] = 0;
									writeFieldTD(one);								// brise vrednost u RC field[0]
									LeaveCriticalSection(&criticalRC);
									return 1;
								}
								this->readEntryFD(fields[poslednjiFLField]);
								
								pocetniKlaster = entries[poslednjiEntry].firstCluster;
								if(entry!=poslednjiEntry || poslednjiFLField != clusterFirst ){
								memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
								
								}else memset(&poslednji,NULL,20);
								
								memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

								writeEntryTD(fields[poslednjiFLField]);		// upisuje oslobodjenu poslednju lokaciju
								
								readFieldFD(one);							// cita fields sa diska
								readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin

								
								memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
								writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

								deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo

								readFieldFD(one);	
								drive->bitV->freeCluster(fields[poslednjiFLField]);
								fields[poslednjiFLField] = 0;
								writeFieldTD(one);
								LeaveCriticalSection(&criticalRC);
								return 1;
							}

						}else if (poslednjiFLField == 256){	// GRANICNI SLUCAJ

							if(poslednjiEntry){	// Entry Poslednjeg > 0

								pocetniKlaster = entries[entry].firstCluster;
								this->readField2FD(fields[poslednjiFLField]);	// dohvata field2 na koji pokazuje field 
								this->readEntryFD(fields2[poslednjiSLField]);	// dohvata entry-e na drugog nivoa

								// nije potreban uslov, zato sto je prvi klaster sigurno manji od 256
								memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
																
								memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

								writeEntryTD(fields2[poslednjiSLField]);	// upisuje oslobodjenu poslednju lokaciju
								
								readFieldFD(one);							// cita fields sa diska
								readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin ovde je clusterFirst<256

								
								memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
								writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

								deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo
								LeaveCriticalSection(&criticalRC);
								return 1;

							}else{	// Entry = 0
								
								pocetniKlaster = entries[poslednjiEntry].firstCluster;
								if(poslednjiSLField == 0 ){							// znaci da je to ujedno i poslednji entry u SLField
									
									this->readFieldFD(one);							// ucitava field prvog nivoa
									this->readField2FD(fields[poslednjiFLField]);	// dohvata field2 na koji pokazuje field poslednjiFLField = 256
									this->readEntryFD(fields2[poslednjiSLField]);	// dohvata entry-e na drugog nivoa poslednjiSLField = 0

									// nije potreban uslov, zato sto je prvi klaster sigurno manji od 256
									memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
									
									memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

									writeEntryTD(fields2[poslednjiSLField]);	// upisuje oslobodjenu poslednju lokaciju
								
									readFieldFD(one);							// cita fields sa diska
									readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin ovde je clusterFirst<256

								
									memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
									writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

									deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo

									this->drive->bitV->freeCluster(fields2[poslednjiSLField]);	// oslobadja klaster entry 
									memset(&fields2,NULL,sizeof(fields2));			// upisuje null u field2
									this->writeField2TD(fields[poslednjiFLField]);	// prepisuje ga na particiju
									this->drive->bitV->freeCluster(fields[poslednjiFLField]);	// oslobadja ga u bit vector-u
									fields[poslednjiFLField] = 0;					// upisuje 0 u poslednjiFLField
									this->writeFieldTD(one);						// to upisuje na disk
									LeaveCriticalSection(&criticalRC);
									return 1;

								}
								// ovde dolazi ako je SLF >0 , a entry je 0 , znaci treba brisati poslednji entry klaster
								this->readEntryFD(fields2[poslednjiSLField]);
								
								memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
								
								memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

								writeEntryTD(fields2[poslednjiSLField]);	// upisuje oslobodjenu poslednju lokaciju
								
								readFieldFD(one);							// cita fields sa diska
								readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin

								
								memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
								writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

								deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo

								this->drive->bitV->freeCluster(fields2[poslednjiSLField]);	// oslobadja klaster entry 
								fields2[poslednjiSLField] = 0;				// upisuje 0 u field2 tj. oslobadja taj field drugog nivoa
								this->writeField2TD(fields[poslednjiFLField]);	// prepisuje ga na particiju
								
								LeaveCriticalSection(&criticalRC);
								return 1;
							}
						}else if (poslednjiFLField < 512){

								if(poslednjiEntry){	// Entry Poslednjeg > 0

								pocetniKlaster = entries[entry].firstCluster;
								this->readField2FD(fields[poslednjiFLField]);	// dohvata field2 na koji pokazuje field 
								this->readEntryFD(fields2[poslednjiSLField]);	// dohvata entry-e na drugog nivoa

								// nije potreban uslov, zato sto je prvi klaster sigurno manji od 256
								memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
																
								memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

								writeEntryTD(fields2[poslednjiSLField]);	// upisuje oslobodjenu poslednju lokaciju
								
								readFieldFD(one);							// cita fields sa diska
								readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin ovde je clusterFirst<256

								
								memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
								writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

								deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo
								LeaveCriticalSection(&criticalRC);
								return 1;

							}else{	// Entry = 0
								
								pocetniKlaster = entries[poslednjiEntry].firstCluster;
								if(poslednjiSLField == 0 ){							// znaci da je to ujedno i poslednji entry u SLField
									
									this->readFieldFD(one);							// ucitava field prvog nivoa
									this->readField2FD(fields[poslednjiFLField]);	// dohvata field2 na koji pokazuje field poslednjiFLField > 256
									this->readEntryFD(fields2[poslednjiSLField]);	// dohvata entry-e na drugog nivoa poslednjiSLField = 0

									// nije potreban uslov, zato sto je prvi klaster sigurno manji od 256
									memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
									
									memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

									writeEntryTD(fields2[poslednjiSLField]);	// upisuje oslobodjenu poslednju lokaciju
								
									readFieldFD(one);							// cita fields sa diska
									readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin ovde je clusterFirst<256

								
									memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
									writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

									deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo

									this->drive->bitV->freeCluster(fields2[poslednjiSLField]);	// oslobadja klaster entry 
									memset(&fields2,NULL,sizeof(fields2));			// upisuje null u field2
									this->writeField2TD(fields[poslednjiFLField]);	// prepisuje ga na particiju
									this->drive->bitV->freeCluster(fields[poslednjiFLField]);	// oslobadja ga u bit vector-u
									fields[poslednjiFLField] = 0;					// upisuje 0 u poslednjiFLField
									this->writeFieldTD(one);						// to upisuje na disk
									LeaveCriticalSection(&criticalRC);
									return 1;

								}
								// ovde dolazi ako je SLF >0 , a entry je 0 , znaci treba brisati poslednji entry klaster
								this->readEntryFD(fields2[poslednjiSLField]);
								
								memcpy(&poslednji , &entries[poslednjiEntry] , 20);	// kopira poslednji zauzeti Entry
								
								memset(&entries[poslednjiEntry],NULL,20);	//	brise poslednji zauzeti Entry

								writeEntryTD(fields2[poslednjiSLField]);	// upisuje oslobodjenu poslednju lokaciju
								
								readFieldFD(one);							// cita fields sa diska
								readEntryFD(fields[clusterFirst]);			// cita Entry-e sa diska od onog koji treba da se promenin

								
								memcpy(&entries[entry] , &poslednji , 20);	// prepisuje poslednji u onaj koji se brise
								writeEntryTD(fields[clusterFirst]);			// upisuje Entry-e na disk

								deleteFileClusters(pocetniKlaster);			// Ova funkcija oslobadja klastere koje je fajl zauzeo

								this->drive->bitV->freeCluster(fields2[poslednjiSLField]);	// oslobadja klaster entry 
								fields2[poslednjiSLField] = 0;				// upisuje 0 u field2 tj. oslobadja taj field drugog nivoa
								this->writeField2TD(fields[poslednjiFLField]);	// prepisuje ga na particiju
								
								LeaveCriticalSection(&criticalRC);
								return 1;
							}
						}

						LeaveCriticalSection(&criticalRC);
						return 1;
					}
			
				}
			}
			
			clusterFirst++;
					
		}else if( fields[clusterFirst] && clusterFirst<512 && clusterSecond<512){
			
			this->readField2FD(fields[clusterFirst]);
			for(clusterSecond =0;clusterSecond<512;clusterSecond++){
				this->readEntryFD(fields2[clusterSecond]);
				for(entry =0;entry<102;entry++){
					if( memcmp(&entries[entry] .name,fnam,8*sizeof(char)) == 0 ){
						if( memcmp(&entries[entry].ext,fext,3*sizeof(char)) == 0 ){
							cout<<"Fajl "<<fname<<" je uspesno pronadjen! SLEDI BRISANjE"<<endl;		
							LeaveCriticalSection(&criticalRC);
							return 1;
						}
			
					}
				}
			}
			clusterFirst++;
		}else {
			break;
		}
	}
}

void RootCluster::updateEntrySize(char* fname, BytesCnt size){

	EnterCriticalSection(&criticalRC);
	
	char part;
	char* fnam = new char[8]();	//	setuje fnam sve na null
	char* fext = new char[3]();	//	setuje fext sve na null

	decodeName(fname,part,fnam,fext);

	ClusterNo clusterFirst=0 , clusterSecond=0;	// polja u klasterima prvog nivoa , i drugog nivoa
	EntryNum  entry;
	unsigned int num=0;

	readFieldFD(one);	// citanje RootCluster-a

	while(true){
	
		if( fields[clusterFirst] != 0 && clusterFirst <=255){
			readEntryFD(fields[clusterFirst]);
			for(int i =0;i<102;i++){
				if( memcmp(&entries[i].name,fnam,8*sizeof(char)) == 0 ){
					if( memcmp(&entries[i].ext,fext,3*sizeof(char)) == 0 ){
						// vrsi se promena velicine fajla 
						readEntryFD(fields[clusterFirst]);// ovo je dodato zbog sinhronizacije
						memcpy(&entries[i].size,&size,sizeof(size));
						writeEntryTD(fields[clusterFirst]);
						//writeEntryTD(fields[clusterFirst]);// ovo je dodato zbog sinhronizacije
						break;
					}
			
				}
			}
			
			clusterFirst++;
					
		}else if( fields[clusterFirst] && clusterFirst<512 && clusterSecond<512){
			
			this->readField2FD(fields[clusterFirst]);
			for(int j =0;j<512;j++){
				this->readEntryFD(fields2[j]);
				for(int i =0;i<102;i++){
					if( memcmp(&entries[i] .name,fnam,8*sizeof(char)) == 0 ){
						if( memcmp(&entries[i].ext,fext,3*sizeof(char)) == 0 ){
							// vrsi se promena velicine fajla 
							this->readField2FD(fields[clusterFirst]); // ovo je dodato zbog sinhronizacije
							memcpy(&entries[i].size,&size,sizeof(size));
							this->writeEntryTD(fields2[j]);
							//this->writeEntryTD(fields2[j]);	// ovo je dodato zbog sinhronizacije
							break;
						}
			
					}
				}
			}
			clusterFirst++;
		}else {
			break;
		}

}
	LeaveCriticalSection(&criticalRC);
}

char RootCluster::readRootDir(EntryNum n,Directory &d){
	EnterCriticalSection(&criticalRC);

	ClusterNo clusterFirst , clusterSecond;
	EntryNum  entry;
	unsigned int num=0;
	bool stampaj = false;

	readFieldFD(one);	// citanje RootCluster-a

	if (! this->getClusters(clusterFirst,clusterSecond,entry,n)){ LeaveCriticalSection(&criticalRC); return 0; }

	if ( clusterFirst <= 255){
		
			if ( clusterFirst == 255 ){
				readEntryFD(fields[clusterFirst]);
				
				while(num<64){
					if ( entries[entry].name[0] != 0){
						
						memcpy(&d[num] , &entries[entry] , 20 * sizeof(char) );
						if(stampaj) cout<<"entries["<<num<<"] = "<<d[num].name<<endl;
						num++; entry++;
					}else break;
					if ( entry == 102 ){
					
						readField2FD(fields[clusterFirst+1]);
						readEntryFD(fields2[clusterSecond]);
						entry = 0;						
					}	
						
				}
			}
			else if (fields[clusterFirst]!= 0){
			
				readEntryFD(fields[clusterFirst]);
				
				while(num<64){
					if(entries[entry].name[0]!=0){
						
						
						memcpy(&d[num] , &entries[entry] , 20 * sizeof(char) );
						if(stampaj) cout<<"entries["<<num<<"] = "<<entries[entry].name<<endl;
						num++;	entry++;
						}else break;
					if(entry==102){
						readEntryFD(fields[clusterFirst+1]);
						entry=0;
					}

				}			

		}
			if (num == 64) {
				if(entries[entry].name[0]==0){LeaveCriticalSection(&criticalRC); return 64;}
				else{ LeaveCriticalSection(&criticalRC); return 65;}
			}
			else{LeaveCriticalSection(&criticalRC); return num;}
	}else if(clusterFirst <=511 && fields[clusterFirst]!= 0){
		readField2FD(fields[clusterFirst]);
		if(clusterSecond <= 511){
			
			if( clusterSecond == 511){
				readFieldFD(clusterFirst);
				readEntryFD(fields2[clusterSecond]);

				while(num<64){
					if ( entries[entry].name[0] != 0 ){
						memcpy(&d[num] , &entries[entry] , 20 * sizeof(char) );
						if(stampaj) cout<<"entries["<<num<<"] = "<<d[num].name<<endl;
						num++; entry++;
					}else break;

					if ( entry == 102){
						readFieldFD(clusterFirst+1);
						readField2FD(fields[clusterFirst+1]);
						readEntryFD(fields2[clusterSecond=0]);
						entry=0;
					}
				}
			}
			else if(fields2[clusterSecond] != 0){
				readEntryFD(fields2[clusterSecond]);
				
				while(num<64){
					if(entries[entry].name[0] !=0 ){
						
						memcpy(&d[num] , &entries[entry] , 20 * sizeof(char) );
						if(stampaj) cout<<"entries["<<num<<"] = "<<entries[entry].name<<endl;
						num++;	entry++;
					}else break;
					if ( entry == 102){
						readEntryFD(fields2[clusterSecond+1]);
						entry=0;
					}
				
			}
			
		}
		}
		
			if (num == 64) {
				if(entries[entry].name[0]==0){LeaveCriticalSection(&criticalRC); return 64;}
				else{LeaveCriticalSection(&criticalRC); return 65;}
			}
			else{LeaveCriticalSection(&criticalRC); return num;}
	}
	else{LeaveCriticalSection(&criticalRC); return 0;}
	
}


void RootCluster::writeEntryTD(ClusterNo clNo){
	EnterCriticalSection(&criticalRC);
	memcpy(&this->cluster,&entries, sizeof(entries));
	this->partition->writeCluster(clNo,this->cluster);
	LeaveCriticalSection(&criticalRC);
}

void RootCluster::readEntryFD(ClusterNo clNo){
	EnterCriticalSection(&criticalRC);
	this->partition->readCluster(clNo,this->cluster);
	memcpy(this->entries,this->cluster,sizeof(entries));
	LeaveCriticalSection(&criticalRC);
}

void RootCluster::writeFieldTD(ClusterNo clNo){
	EnterCriticalSection(&criticalRC);
	memcpy(this->cluster,fields, sizeof(fields));
	this->partition->writeCluster(clNo,this->cluster);
	LeaveCriticalSection(&criticalRC);
}

void RootCluster::readFieldFD(ClusterNo clNo){
	EnterCriticalSection(&criticalRC);
	this->partition->readCluster(clNo,this->cluster);
	memcpy(this->fields,this->cluster,sizeof(fields));
	LeaveCriticalSection(&criticalRC);
}

void RootCluster::writeField2TD(ClusterNo clNo){
	EnterCriticalSection(&criticalRC);
	memcpy(this->cluster,fields2, sizeof(fields2));
	this->partition->writeCluster(clNo,this->cluster);
	LeaveCriticalSection(&criticalRC);
}

void RootCluster::readField2FD(ClusterNo clNo){
	EnterCriticalSection(&criticalRC);
	this->partition->readCluster(clNo,this->cluster);
	memcpy(this->fields2,this->cluster,sizeof(fields2));
	LeaveCriticalSection(&criticalRC);
}

void RootCluster::resetEntry(){
	EnterCriticalSection(&criticalRC);
		char nulaa[8];
		memset(nulaa,NULL,sizeof(nulaa));

		unsigned int nula = 0 ;
		
		for(int i=0; i < numberOfEntries;i++){
			/* name */	memcpy(entries[i].name, &nulaa  ,sizeof(char)*8);
			/* ext  */	memcpy(entries[i].ext , &nulaa  ,sizeof(char)*3);
			/* res  */	entries[i].reserved		= 0;
			/* first*/	entries[i].firstCluster = 0;
			/* size */	entries[i].size			= 0;
			
			}
		LeaveCriticalSection(&criticalRC);
}

int RootCluster::getClusters(ClusterNo& clusterFirst,ClusterNo& clusterSecond,EntryNum& entry,EntryNum n){
	EnterCriticalSection(&criticalRC);
	
	if( n <=  numberOfEntriesInFirstLevel) clusterFirst = n / numberOfEntries;
	else clusterFirst = numberOfFields/2 + (n-numberOfEntriesInFirstLevel) / (numberOfFields*102);
	entry   = n % numberOfEntries;
	clusterSecond = 0;
	if  ( clusterFirst <= (numberOfFields/2-1) ){ 
		CurrField = clusterFirst; 
		LeaveCriticalSection(&criticalRC);
		return 1;
	}
	else if ( clusterFirst <= (numberOfFields-1) ){ 
		ClusterNo left,pom;
		left	= (n - numberOfEntriesInFirstLevel);
		pom     = left / (numberOfFields*numberOfEntries);
		left	-= pom*numberOfFields*numberOfEntries;
		clusterSecond = (left / numberOfEntries);
		if( left%numberOfEntries == 0) clusterSecond--;
		if( left % numberOfEntries )
		entry = left - clusterSecond*numberOfEntries - 1 ;
		else if(clusterFirst >(numberOfFields/2)){cout<<"Usao u if RootCluster::getClusters"<<endl;  entry = 101;}
		LeaveCriticalSection(&criticalRC);
		return 1;
	}
	else{ cout<<"nije unet ispravan broj entry"<<endl; LeaveCriticalSection(&criticalRC); return 0;}
}

void RootCluster::deleteFileClusters(ClusterNo pocetniKlaster){
	EnterCriticalSection(&criticalRC);

	readFieldFD(pocetniKlaster);

	ClusterNo currField = 0;
	ClusterNo currSField = 0;

	char emptyCluster [2048];
	memset(emptyCluster, 0, sizeof emptyCluster);
	
	while(fields[currField]){
	
		if(currField < 256){
			drive->bitV->freeCluster(fields[currField]);				// oslobadja bit vector
			partition->writeCluster(fields[currField],emptyCluster);	// upisuje sve nule u klaster
		}else if(currField <512){
			readField2FD(fields[currField]);
			while(fields2[currSField]!=0 && currSField<512){
				drive->bitV->freeCluster(fields2[currSField]);				// oslobadja bit vector
				partition->writeCluster(fields2[currSField],emptyCluster);	// upisuje sve nule u klaster			
				currSField++;
			}
			
		}else break;

		currField++;
	}
	
	drive->bitV->freeCluster(pocetniKlaster);				// oslobadja bit vector
	partition->writeCluster(pocetniKlaster,emptyCluster);	// upisuje sve nule u klaster
	LeaveCriticalSection(&criticalRC);
}