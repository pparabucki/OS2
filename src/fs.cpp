// Project:	Operativni Sistemi 2
// File: 	fs.cpp
// Date: 	March 2014
// Author: 	Petar Parabucki 115/06
// Contents:
//			
//			Class:	fs

#include "fs.h"
#include "KernelFS.h"
#include "semafori.h"


KernelFS *FS::myImpl = new KernelFS();

FS::~FS() {
	delete FS::myImpl;
}

char FS::mount(Partition* partition) {
	return FS::myImpl->mount(partition);
}

char FS::unmount(char part) {
	return FS::myImpl->unmount(part);
}

char FS::format(char part) {
	return FS::myImpl->format(part);
}

char FS::readRootDir(char part, EntryNum n, Directory &d) {
	return FS::myImpl->readRootDir(part, n, d);
}

char FS::doesExist(char* fname) {
	return FS::myImpl->doesExist(fname);
}

File* FS::open(char* fname, char mode) {

	 
	 File* f = &FS::myImpl->open(fname, mode);
	
	 if(f != NULL)
	 return f;
	 else return NULL;
}

char FS::deleteFile(char* fname) {
	return FS::myImpl->deleteFile(fname);
}