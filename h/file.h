#pragma once

#include "fs.h"
#include "KernelFile.h"

class KernelFile;

class File {
public:
	char write (BytesCnt, char* buffer);		//upis niza bajtova od tekuce pozicije
	BytesCnt read (BytesCnt, char* buffer);		// citanje niza bajtova od tekuce pozicije
	char seek (BytesCnt);						// pomeranje tekuce pozicije u fajlu
	BytesCnt filePos();							// dohvatanje tekuce pozicije u fajlu
	char eof ();								// provera da li je tekuca pozicija u fajlu kraj tog fajla
	BytesCnt getFileSize();						// dohvatanje trenutne velicine u bajtovima
	char truncate ();							// **opciono -- brisanje dela fajla od tekuce pozicije do kraja
	~File();									//zatvaranje fajla
private:
	friend class FS;
	friend class KernelFS;
	friend class ListOfFiles;
	friend void TestMountFormatInsertGet();
	friend class Elem;
	friend int TestSaSlikom();
	friend int TestFajlovi();

	File ();									//objekat fajla se moze kreirati samo otvaranjem
	KernelFile *myImpl;
};