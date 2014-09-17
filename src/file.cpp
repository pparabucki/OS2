#include "File.h"
#include "KernelFS.h"
#include <iostream>
#include "semafori.h"
using namespace std;

File::~File() {
	delete myImpl;
};

File::File() {
	this->myImpl = NULL;
};

char File::write(BytesCnt bc, char* buffer) {

	return myImpl->write(bc,buffer);
};

BytesCnt File::read(BytesCnt bc, char* buffer) {
	
	return myImpl->read(bc,buffer);

};

char File::seek(BytesCnt bc) {
	return myImpl->seek(bc);
};

BytesCnt File::filePos() {
	return myImpl->filePos();
};

char File::eof() {
	if(!myImpl) return 1; //greska
	return myImpl->eof();
};

BytesCnt File::getFileSize() {
	return myImpl->getFileSize();
};

char File::truncate() {
	return myImpl->truncate();
};
