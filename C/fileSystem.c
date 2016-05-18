/* PROJECT DESCRIPTION (ADDED 5/16/2016): 

	Manipulation (reading and editing) of FAT32 images
	Creation of interactive working filesystem from image. 
	
	Used to demonstrate C skills
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "list.h"

#define MAX_PATH_SIZE 80
#define END_OF_CHAIN 268435455
#define LONGNAME 0xF
#define DIR 0x10
#define FIRST_FOUR_BITS_CLEARED 0b00001111111111111111111111111111
#define FIRST_3_BYTES_CLEARED 0b00000000000000000000000011111111
#define DELETE 1
#define KEEP 0
#define NUM_FILES_PER_LINE 10
#define BYTES_PER_DIR_ENTRY 32
#define FREE_CLUSTER 0
#define _FILE 1
#define DIRECTORY 0
#define DOT_DIRECTORY -1
#define SPACE_2_DOT 1
#define DOT_2_SPACE 0
#define NEITHER 2

typedef struct DirectoryEntryStruct {
	unsigned char *DE_byteArray;
	char mode[3];
	char name[13];
	struct list_head list;
	unsigned int offsetOfSelf;
} DirectoryEntry;

typedef struct NumberStruct {
	unsigned int num;
	struct list_head list;
} Number;

/* file image */
FILE *imagefd;

/* DIR entry lists */
struct list_head cwdDirEntries;
struct list_head openFileTable;

/* boot sector info */
unsigned int
	BPB_BytesPerSec,
	BPB_SecPerClus,
	BPB_RsvdSecCnt,
	BPB_NumFATS,
	BPB_FATSz32,
	BPB_RootClus, 
	BPB_RootEntCnt;	
	
unsigned int startOfFAT1, startOfFAT2;

/* input: offset and number of bytes to read 
   output: byte array containing bytes from offset to offset + numBytes */
unsigned char *readBytes(const unsigned int offset, const unsigned int numBytes) {
	unsigned char *byteArray = malloc(sizeof(unsigned char) * numBytes);
	
	fseek(imagefd, offset, SEEK_SET);
	
	fread(byteArray, 1, numBytes, imagefd);
	
	return byteArray;
	
}

/* takes a little endian byte array up to size 4 *
 * returns an unsigned int                       */
unsigned int byteArrayToInt(unsigned char *byteArray, const unsigned int size, const int delete) {
	unsigned int i;
	unsigned int uintArray[4] = { 0, 0, 0, 0 };
	
	if (size > 4 || size < 1) {
		printf("byteArrayToInt() error!\n");
		exit(1);
	}
	
	for (i = 0; i < size; i++)
		uintArray[i] = (unsigned int)byteArray[i];
	
	if (delete)
		free(byteArray);
	
	return (uintArray[3] << 24) | (uintArray[2] << 16) | (uintArray[1] << 8) | uintArray[0];
}

/* takes an unsigned int, and returns a little endian byte array of size 4 */
/* source: http://stackoverflow.com/questions/2350099/how-to-convert-an-int-to-a-little-endian-byte-array */
unsigned char *intToByteArray(const unsigned int integer) {
	unsigned char *byteArray = malloc(sizeof(unsigned char) * 4);
	
	byteArray[0] = (unsigned char)integer;
	byteArray[1] = (unsigned char)(integer >> 8 & 0xFF);
	byteArray[2] = (unsigned char)(integer >> 16 & 0xFF);
	byteArray[3] = (unsigned char)(integer >> 24 & 0xFF);
	
	return byteArray;
}

/* copies a directory entry */
void copyDirEntry(DirectoryEntry *copy, const DirectoryEntry *original) {
	int i = 0;
	if (original == NULL)
		return;
	
	if (original->DE_byteArray != NULL) {
		copy->DE_byteArray = malloc(sizeof(unsigned char) * BYTES_PER_DIR_ENTRY);
		
		for (i = 0; i < BYTES_PER_DIR_ENTRY; i++)
			copy->DE_byteArray[i] = original->DE_byteArray[i];
	}
	strncpy(copy->name, original->name, 12);
	copy->name[12] = '\0';
	
	copy->offsetOfSelf = original->offsetOfSelf;
}

/* given a path name, returns the last folder/file name 
   e.g. input: /usr/src/include/ output: include */
char *getCWDName(const char *cwd) {
	int i = strlen(cwd) - 1;
	char *name = malloc(sizeof(char) * (i + 2));
	
	/* decrement i until it points to the second to last '/' */
	while(cwd[--i] != '/');
	
	strcpy(name, &cwd[i+1]);
	name[strlen(name) - 1] = '\0';
	
	if (strlen(name) < 2)
		name[0] = '/';
	
	return name;
}

/* source: http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
   Function that trims leading and trailing whitespace */
void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}

/* trims consecutive dots
   e.g. input: file.....txt output: file.txt */
void trimDots(char *filename){
	int i = 0, numSequential = 0;
	
	while(filename[i] != '\0') {
		if (filename[i] == '.')
			numSequential++;
		
		else 
			numSequential = 0;
		
		if (numSequential > 1)
			filename[i] = ' ';
		
		i++;
	}
	
	for (i = 0; i < 11; i++){
		if (filename[i] == ' ') {
			trim(&filename[i++]);
			break;
		}
	}
}

/* pads with spaces, converts a given string into a legal short entry DIR_NAME */
void expand(char *s) {
	int i = 0, j = 0, end;
	char *new = malloc(sizeof(char) * 12);
	
	strncpy(new, s, 11);
	new[11] = '\0';
	
	while (new[i] != '\0' && new[i] != ' ' && i < 8)
		i++;
	
	if (new[i] == ' ')
		j = i;
	
	while (i < 8) {
		new[i] = ' ';
		i++;
	}
	
	if (j != 0) {
		j++;
		end = j + 3;
		while (s[j] != ' ' && s[j] != '\0' && j < end)
			new[i++] = s[j++];
		
		
	}
	
	for (; i < 11; i++)
			new[i] = ' ';
		
	strncpy(s, new, 12);
	
	free(new);
}

/* boolean function 
   returns true if there is text character after a space */
int textAfterSpace(char *s) {
	int i = 0;
	
	while ((s[i] == ' ' || s[i] == '.') && s[i] != '\0')
		i++;
	
	if (s[i] == '\0')
		return 0;
	
	return 1;
	
}

/* multipurpose function
   converts alpha characters to uppercase
   and depending on space2Dot, converts dots to spaces, or spaces to dots, or neither */
void convertFileName(char *filename, const int conversionType) {
	int i = 0, spaceFound = 0;

	while (filename[i] != '\0') {
		if (filename[0] == '.') {
			filename[2] = '\0';
			return;
		}
		
		if (filename[i] == ' ' && conversionType == SPACE_2_DOT && textAfterSpace(&filename[i + 1])) {
			filename[i] = '.';
			spaceFound = 1;
		}
		else if (filename[i] == ' ' && conversionType == SPACE_2_DOT) {
			filename[i] = '\0';
			return;
		}
		else if (filename[i] == '.' && conversionType == DOT_2_SPACE && textAfterSpace(&filename[i + 1]))
			filename[i] = ' ';
		
		else
			filename[i] = toupper(filename[i]);
		
		i++;
	}
	
	if (!spaceFound && conversionType == SPACE_2_DOT && strlen(filename) > 8) {	
		filename[11] = filename[10];
		filename[10] = filename[9];
		filename[9] = filename[8];
		filename[8] = '.';
		
	}
	
	filename[12] = '\0';	
	
}

/* input: a cluster number with a value pointing to END_OF_CHAIN
   output: the address of the start of the data of the new cluster */
unsigned int allocateNewCluster(const unsigned int prevClusterNum) {
	unsigned char *byteArray, zero = 0;
	unsigned int offset = startOfFAT1, currentClusterNum, i;
	unsigned int prevClusterNumOffset = startOfFAT1 + (prevClusterNum * 4);
	unsigned int firstDataSector, firstSectorofCluster, ptrToNewClusNum;
	unsigned int EOC = END_OF_CHAIN;
	
	/* search FAT table for any free clusters */
	while (offset < startOfFAT2) {
		byteArray = readBytes(offset, 4);
		currentClusterNum = byteArrayToInt(byteArray, 4, DELETE);
		currentClusterNum &= FIRST_FOUR_BITS_CLEARED;
		
		if (currentClusterNum == FREE_CLUSTER) {
			/* set the previous cluster to point to the new cluster number */
			fseek(imagefd, prevClusterNumOffset, SEEK_SET);
			ptrToNewClusNum = (offset - startOfFAT1) / 4;
			fwrite(&ptrToNewClusNum, sizeof(ptrToNewClusNum), 1, imagefd);
			
			/* set the current cluster in FAT to EoC */
			fseek(imagefd, offset, SEEK_SET);
			fwrite(&EOC, sizeof(EOC), 1, imagefd); 
			
			/* calculate the address of the start of the data section of the new cluster */
			firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
			firstSectorofCluster = ((ptrToNewClusNum - 2) * BPB_SecPerClus) + firstDataSector;
			offset = firstSectorofCluster * BPB_BytesPerSec;
			
			/* initialize all bytes of new cluster to 0 */
			for (i = 0; i < BPB_BytesPerSec; i++) {
				fseek(imagefd, offset + i, SEEK_SET);
				fwrite(&zero, 1, 1, imagefd);
			}
			fflush(imagefd);
			
			return offset;
		}
		offset += 4;
	}
	
	/* if FAT is full, print error msg and return with error code 0 */
	printf("Error: out of space!\n");
	return 0;
}

/* same as allocateNewCluster() except it doesn't write to FAT[prevClusterNum] and doesn't return offset of data section
   it returns the cluster number that was found to be free */
unsigned int findNewClusterNumber() {
	unsigned char *byteArray, zero = 0;
	unsigned int offset = startOfFAT1, currentClusterNum, i;
	unsigned int firstDataSector, firstSectorofCluster, newClusterNum;
	unsigned int EOC = END_OF_CHAIN;
	
	/* search FAT table for any free clusters */
	while (offset < startOfFAT2) {
		byteArray = readBytes(offset, 4);
		currentClusterNum = byteArrayToInt(byteArray, 4, DELETE);
		currentClusterNum &= FIRST_FOUR_BITS_CLEARED;
		
		if (currentClusterNum == FREE_CLUSTER) {
			/* set the current cluster in FAT to EoC */
			fseek(imagefd, offset, SEEK_SET);
			fwrite(&EOC, sizeof(EOC), 1, imagefd); 
			
			newClusterNum = (offset - startOfFAT1) / 4;
			
			/* calculate the address of the start of the data section of the new cluster */
			firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
			firstSectorofCluster = ((newClusterNum - 2) * BPB_SecPerClus) + firstDataSector;
			offset = firstSectorofCluster * BPB_BytesPerSec;
			
			/* initialize all bytes of new cluster to 0 */
			for (i = 0; i < BPB_BytesPerSec; i++) {
				fseek(imagefd, offset + i, SEEK_SET);
				fwrite(&zero, 1, 1, imagefd);
			}
			fflush(imagefd);
			
			return newClusterNum;
		}
		offset += 4;
	}
	
	/* if FAT is full, print error msg and return with error code 0 */
	printf("Error: out of space!\n");
	return 0;
}

/* given a byte array, return a cluster number */
unsigned int getClusterNumFromByteArray(unsigned char *byteArray) {
	unsigned int DIR_FstClusHI, DIR_FstClusLO, clusterNum;
	
	DIR_FstClusLO = byteArrayToInt(byteArray + 26, 2, KEEP);
	DIR_FstClusHI = byteArrayToInt(byteArray + 20, 2, KEEP);
		
	clusterNum = (DIR_FstClusHI << 24) | DIR_FstClusLO;
	
	if (clusterNum == 0)
		clusterNum = BPB_RootClus;
	
	return clusterNum;
}

/* given an offset and a cluster number, loops through the directory and adds them to a linked list */
void add_cwd_entries_to_list(unsigned int dirEntryOffset, unsigned int currentClusterNum) {
	unsigned int i = 0, firstDataSector, firstSectorofCluster;
	unsigned char *byteArray;
	DirectoryEntry *newDirEntry;
	
	/* list through directory, saving valid directory entries into a list */
	while (1) { 
		byteArray = readBytes(dirEntryOffset, BYTES_PER_DIR_ENTRY);
		
		/* if directory from here on out is empty, break */
		if (byteArray[0] == 0) {
			free(byteArray);
			return;
		}
		
		/* if directory entry has been deleted, or if the directory entry is a long name directory entry, skip */
		else if (byteArray[0] == 0xE5 || byteArray[11] & 0xF == LONGNAME) {
			free(byteArray);
			dirEntryOffset += BYTES_PER_DIR_ENTRY;
			i++;
			continue;
		}
		
		newDirEntry = malloc(sizeof(DirectoryEntry));
		
		newDirEntry->DE_byteArray = byteArray;
		strncpy(newDirEntry->name, byteArray, 11);
		newDirEntry->name[11] = '\0';
		newDirEntry->name[12] = '\0';
		trim(newDirEntry->name);
		
		if (newDirEntry->name[0] != '.') {
			convertFileName(newDirEntry->name, SPACE_2_DOT);
			trimDots(newDirEntry->name);
		}
		
		newDirEntry->offsetOfSelf = dirEntryOffset;
		
		list_add_tail(&newDirEntry->list, &cwdDirEntries);
		
		dirEntryOffset += BYTES_PER_DIR_ENTRY;
		i++;
		
		/* check if at end of cluster, if so, find the next cluster */
		if (i == (BPB_BytesPerSec / BYTES_PER_DIR_ENTRY)) {
			i = 0;
			
			/* get next cluster number */
			byteArray = readBytes(startOfFAT1 + (currentClusterNum * 4), 4);
			currentClusterNum = byteArrayToInt(byteArray, 4, DELETE);
			currentClusterNum &= FIRST_FOUR_BITS_CLEARED;
			
			if (currentClusterNum == END_OF_CHAIN) 
				return;
			
			/* change offset to the first sector of currentClusterNum */ 
			else {
				firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
				firstSectorofCluster = ((currentClusterNum - 2) * BPB_SecPerClus) + firstDataSector;
	
				dirEntryOffset = firstSectorofCluster * BPB_BytesPerSec;
			}
		} 
	}
	
}

/* initializes the global data, locates the root directory, 
   and adds all the directory entries inside root to a linked list */
void init(int argc, char **argv) {
	unsigned char *byteArray;
	unsigned int firstDataSector, firstSectorofCluster;
	
	imagefd = fopen(argv[1], "r+");
	if (imagefd == NULL) {
		printf("error: fopen(%s, r+) failed\n", argv[1]);
		exit(1);
	}
	
	INIT_LIST_HEAD(&cwdDirEntries);
	INIT_LIST_HEAD(&openFileTable);
	
	/* read BytesPerSec */
	byteArray = readBytes(11, 2);
	BPB_BytesPerSec = byteArrayToInt(byteArray, 2, DELETE);
	
	/* read SecPerClus */
	byteArray = readBytes(13, 1);
	BPB_SecPerClus = byteArrayToInt(byteArray, 1, DELETE);
	
	/* read BPB_RsvdSecCnt */
	byteArray = readBytes(14, 2);
	BPB_RsvdSecCnt = byteArrayToInt(byteArray, 2, DELETE);
	
	/* read BPB_NumFATS */
	byteArray = readBytes(16, 1);
	BPB_NumFATS = byteArrayToInt(byteArray, 1, DELETE);
	
	/* read BPB_FATSz32 */
	byteArray = readBytes(36, 4);
	BPB_FATSz32 = byteArrayToInt(byteArray, 4, DELETE);
	
	/* read BPB_RootClus */
	byteArray = readBytes(44, 4);
	BPB_RootClus = byteArrayToInt(byteArray, 4, DELETE);
	
	/* read BPB_RootEntCnt */
	byteArray = readBytes(17, 2);
	BPB_RootEntCnt = byteArrayToInt(byteArray, 2, DELETE);
	
	/* find start of fat tables */
	startOfFAT1 = BPB_RsvdSecCnt * BPB_BytesPerSec;
	startOfFAT2 = startOfFAT1 + (BPB_FATSz32 * BPB_BytesPerSec);
	
	/* find start of root directory */
	firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
	firstSectorofCluster = ((BPB_RootClus - 2) * BPB_SecPerClus) + firstDataSector;
	
	/* add all files inside root directory to cwd list */
	add_cwd_entries_to_list(firstSectorofCluster * BPB_BytesPerSec, BPB_RootClus);
	
}

/* returns 0 if file does not exist, 1 if it does */
int fileExists(const char *filename) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;

	list_for_each(ptr, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		
		if (strcmp(dirEntry->name, filename) == 0) {
			return 1;
		}
	}
	return 0;
}

/* returns 0 if file is closed, 1 if it is open */
int fileIsOpen(const char *filename) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;
	
	list_for_each(ptr, &openFileTable) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			return 1;
		}
	}
	return 0;
}

/* returns 0 if file is not a directory, 1 if it is */
int fileIsDir(const char *filename) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;
	unsigned char result;
	
	list_for_each(ptr, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			
			result = dirEntry->DE_byteArray[11] & DIR;
			
			if (result == DIR)
				return 1;
			
			else
				return 0;
		}
	}
}

void myOpen(const char *filename, const char *mode) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry, *dirEntryCopy;
	
	if (strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0 && strcmp(mode, "rw") != 0 && strcmp(mode, "wr") != 0) {
		printf("Error: incorrect parameter\n");
		return;
	}
	
	if (!fileExists(filename)) {
		printf("Error: file does not exist\n");
		return;
	}
	
	if (fileIsOpen(filename)) {
		printf("Error: file already open!\n");
		return;
	}
	
	if (fileIsDir(filename)) {
		printf("Error: cannot open a directory\n");
		return;
	}
	
	list_for_each(ptr, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			dirEntryCopy = malloc(sizeof(DirectoryEntry));
			copyDirEntry(dirEntryCopy, dirEntry);
			strcpy(dirEntryCopy->mode, mode);
			list_add_tail(&dirEntryCopy->list, &openFileTable);
		}
	}
}

void myClose(const char *filename) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;
	int found = 0;
	
	list_for_each(ptr, &openFileTable) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			found = 1;
			list_del_init(&dirEntry->list);
			break;
		}
	}
	
	if (!found) 
		printf("Error: file not open\n");
		
}

void changeDirectory(const char *dir) {
	struct list_head *ptr, *temp;
	DirectoryEntry *dirEntry;
	unsigned int currentClusterNum;
	unsigned int firstDataSector, firstSectorofCluster, offset;
	
	/* get firstClusterNum and delete everything from cwdDirEntries list */
	list_for_each_safe(ptr, temp, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, dir) == 0) 
			currentClusterNum = getClusterNumFromByteArray(dirEntry->DE_byteArray);
		
		list_del_init(ptr);
		free(dirEntry->DE_byteArray);
		free(dirEntry);
	}
	
	/* find the address of the first sector of the currentClusterNum */
	firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
	firstSectorofCluster = ((currentClusterNum - 2) * BPB_SecPerClus) + firstDataSector;
	offset = firstSectorofCluster * BPB_BytesPerSec;
	
	/* add all files and directories inside inputted directory to cwdDirEntries */
	add_cwd_entries_to_list(offset, currentClusterNum);
}

/* returns a success or failure code, 0 and 1 respectively */
int myCD(const char *dir, char *cwd) {
	int i;
	
	if (!fileExists(dir)) {
		printf("Error: does not exist\n");
		return 1;
	}
	
	if (!fileIsDir(dir)) {
		printf("Error: not a directory\n");
		return 1;
	}
	
	if (strcmp(dir, "..") == 0) {
		changeDirectory(dir);
		
		/* change prompt */
		i = strlen(cwd);
		cwd[--i] = '\0';
		
		while(cwd[i] != '/') 
			cwd[i--] = '\0';	
	}
	
	else if (strcmp(dir, ".") != 0) {
		changeDirectory(dir);
		
		/* change prompt */
		strcat(cwd, dir);
		strcat(cwd, "/");
	}
	
	return 0;
}

DirectoryEntry *myCreate(char *filename, const int type) {
	unsigned int clusterNum, prevClusterNum, foundDot = 0;
	struct list_head *ptr;
	unsigned int i = 0, firstDataSector, firstSectorofCluster, offset, offsetOfDirEntry;
	unsigned char *byteArray;
	DirectoryEntry *firstDirEntry, *newDirEntry;
	
	
	if (fileExists(filename)) {
		printf("Error: file already exists\n");
		return NULL;
	}
	
	if (filename[0] == '.' && type != DOT_DIRECTORY) {
		printf("Error: name not valid\n");
		return NULL;
	}
	
	/* get . directory entry */
	list_for_each(ptr, &cwdDirEntries) {
		firstDirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(firstDirEntry->name, ".") == 0) {
			foundDot = 1;
			break;
		}
	}
	
	/* check if in root directory */
	if (!foundDot) {
		firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
		firstSectorofCluster = ((BPB_RootClus - 2) * BPB_SecPerClus) + firstDataSector;
	
		offset = firstSectorofCluster * BPB_BytesPerSec;
		
		clusterNum = BPB_RootClus;
	}
	else {
		clusterNum = getClusterNumFromByteArray(firstDirEntry->DE_byteArray);
		offset = firstDirEntry->offsetOfSelf;
	}
	
	/* find empty spot */
	while (1) {
		if (type == DOT_DIRECTORY) {
			if (strcmp(filename, "..") == 0)
				offset += BYTES_PER_DIR_ENTRY;
			
			break;
		}
		
		byteArray = readBytes(offset, BYTES_PER_DIR_ENTRY);
		
		i++;
		
		/* if directory entry is empty, has been deleted, or is a long name directory entry, break */
		if (byteArray[0] == 0 || byteArray[0] == 0xE5 || byteArray[11] & 0xF == LONGNAME) {
			free(byteArray);
			
			if (i == (BPB_BytesPerSec / BYTES_PER_DIR_ENTRY)) {
				allocateNewCluster(clusterNum);
				i = 0;
			}
			
			break;
		}
		
		offset += BYTES_PER_DIR_ENTRY;
		
		/* check if at end of cluster, if so, find the next cluster */
		if (i == (BPB_BytesPerSec / BYTES_PER_DIR_ENTRY)) {
			i = 0;
			
			/* save clusterNum  */
			prevClusterNum = clusterNum;
			
			/* get next cluster number */
			byteArray = readBytes(startOfFAT1 + (clusterNum * 4), 4);
			clusterNum = byteArrayToInt(byteArray, 4, DELETE);
			clusterNum &= FIRST_FOUR_BITS_CLEARED;
		
			if (clusterNum == END_OF_CHAIN) {
				offset = allocateNewCluster(prevClusterNum);
				break;
			}
			/* change offset to the first sector of clusterNum */
			else {
				firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
				firstSectorofCluster = ((clusterNum - 2) * BPB_SecPerClus) + firstDataSector;
	
				offset = firstSectorofCluster * BPB_BytesPerSec;
			}		
		}
	}

	offsetOfDirEntry = offset;	

	/* create directory entry */
	newDirEntry = malloc(sizeof(DirectoryEntry));
	newDirEntry->DE_byteArray = malloc(BYTES_PER_DIR_ENTRY);
	
	/* DIR_Name */
	if (type == DOT_DIRECTORY) {
		newDirEntry->DE_byteArray[0] = '.';
		if (filename[1] == '.') 
			newDirEntry->DE_byteArray[1] = '.';
		else
			newDirEntry->DE_byteArray[1] = ' ';
		
		for (i = 2; i < 11; i++) 
			newDirEntry->DE_byteArray[i] = ' ';
	}
	else {
		convertFileName(filename, DOT_2_SPACE);	
		expand(filename);
	
		for (i = 0; i < 11; i++) 
			newDirEntry->DE_byteArray[i] = filename[i];
	}
	
	/* DIR_Attr */
	if (type == DIRECTORY || type == DOT_DIRECTORY) 
		newDirEntry->DE_byteArray[11] = DIR;
	
	else
		newDirEntry->DE_byteArray[11] = 0;
	
	/* DIR_NTRes */
	newDirEntry->DE_byteArray[12] = 0;
	
	/* DIR_Optionals */
	for (i = 13; i < 20; i++)
		newDirEntry->DE_byteArray[i] = 0;
		
	/* find first cluster number */
	clusterNum = findNewClusterNumber();
	
	/* DIR_FstClusHI */
	newDirEntry->DE_byteArray[21] = clusterNum >> 24;
	newDirEntry->DE_byteArray[20] = (clusterNum >> 16) & FIRST_3_BYTES_CLEARED;
	
	/* DIR_WrtTime and DIR_WrtDate */
	newDirEntry->DE_byteArray[22] = 0;
	newDirEntry->DE_byteArray[23] = 0;
	
	newDirEntry->DE_byteArray[25] = 0b100001; 
	newDirEntry->DE_byteArray[24] = 0b10;
	
	/* DIR_FstClusLO */
	newDirEntry->DE_byteArray[27] = (clusterNum >> 8) & FIRST_3_BYTES_CLEARED;
	newDirEntry->DE_byteArray[26] = clusterNum & FIRST_3_BYTES_CLEARED;
	
	/* DIR_FileSize */
	for (i = 28; i < 32; i++)
		newDirEntry->DE_byteArray[i] = 0;
	
	
	/* copy name */
	strncpy(newDirEntry->name, newDirEntry->DE_byteArray, 11);
	newDirEntry->name[11] = '\0';
	newDirEntry->name[12] = '\0';
	trim(newDirEntry->name);
	if (type != DOT_DIRECTORY) {
		convertFileName(newDirEntry->name, SPACE_2_DOT);
		trimDots(newDirEntry->name);
	}
	
	newDirEntry->offsetOfSelf = offsetOfDirEntry;
	
	/* add to list */
	list_add_tail(&newDirEntry->list, &cwdDirEntries);
	
	/* write to img file */
	if (type != DOT_DIRECTORY) {
		fseek(imagefd, offsetOfDirEntry, SEEK_SET);
		fwrite(newDirEntry->DE_byteArray, sizeof(unsigned char), 32, imagefd); 
		fflush(imagefd);
	}
	
	return newDirEntry;
}

void myMkdir(char *dir) {
	struct list_head *ptr, *temp;
	DirectoryEntry *parent = NULL, *newDirEntry, *dotDirEntry;
	unsigned int clusterNum, tempClusterNum, offset;
	unsigned int foundDot = 0, firstDataSector, firstSectorofCluster, zero = 0;
	unsigned char *byteArray;
	
	if (dir[0] == '.') {
		printf("Error: name not valid\n");
		return;
	}
	
	/* get . directory entry; save way to get back */
	list_for_each(ptr, &cwdDirEntries) {
		dotDirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dotDirEntry->name, ".") == 0) {
			foundDot = 1;
			tempClusterNum = getClusterNumFromByteArray(dotDirEntry->DE_byteArray);
			break;
		}
	}
	
	/* create folder */
	parent = myCreate(dir, DIRECTORY); 
	if (parent == NULL) return;
	clusterNum = getClusterNumFromByteArray(parent->DE_byteArray);
	
	/* create the new . directory entry inside new folder */	
	changeDirectory(dir);
	newDirEntry = myCreate(".", DOT_DIRECTORY);	
	
	/* change . directory entry's cluster number */
	/* DIR_FstClusHI */
	newDirEntry->DE_byteArray[21] = clusterNum >> 24;
	newDirEntry->DE_byteArray[20] = (clusterNum >> 16) & FIRST_3_BYTES_CLEARED;
	
	/* DIR_FstClusLO */
	newDirEntry->DE_byteArray[27] = (clusterNum >> 8) & FIRST_3_BYTES_CLEARED;
	newDirEntry->DE_byteArray[26] = clusterNum & FIRST_3_BYTES_CLEARED;
	
	firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
	firstSectorofCluster = ((clusterNum - 2) * BPB_SecPerClus) + firstDataSector;
	offset = firstSectorofCluster * BPB_BytesPerSec;
	
	/* propagate changes to image */
	fseek(imagefd, offset, SEEK_SET);
	fwrite(newDirEntry->DE_byteArray, 1, 32, imagefd);
	fflush(imagefd);
	
	/* create the .. directory entry */
	newDirEntry = myCreate("..", DOT_DIRECTORY);
	
	/* calculate cluster number and offset  */
	if (!foundDot) 
		clusterNum = BPB_RootClus;
	
	else  
		clusterNum = tempClusterNum;
		
	
	/* change new directory entry's cluster number to parent's cluster number*/
	/* DIR_FstClusHI */
	newDirEntry->DE_byteArray[21] = clusterNum >> 24;
	newDirEntry->DE_byteArray[20] = (clusterNum >> 16) & FIRST_3_BYTES_CLEARED;
	
	/* DIR_FstClusLO */
	newDirEntry->DE_byteArray[27] = (clusterNum >> 8) & FIRST_3_BYTES_CLEARED;
	newDirEntry->DE_byteArray[26] = clusterNum & FIRST_3_BYTES_CLEARED;
	
	
	/* propagate changes to image */
	fseek(imagefd, offset + BYTES_PER_DIR_ENTRY, SEEK_SET);
	fwrite(newDirEntry->DE_byteArray, 1, 32, imagefd);
	fflush(imagefd);
	
	/* go back to parent directory */
	changeDirectory("..");
}

void myRemove(char *filename) {
	struct list_head *dirEntryPtr, clusterList, *clusterListPtr, *temp;
	unsigned int clusterNum, savedClusterNum = 0, offset, firstClusterNum, zero;
	unsigned int firstDataSector, firstSectorofCluster;
	unsigned char *byteArray, E5;
	DirectoryEntry *dirEntry;
	Number *clusterNumInList;
	
	zero = 0;
	E5 = 0xE5;
	
	/* find requested file */
	list_for_each(dirEntryPtr, &cwdDirEntries) {
		dirEntry = list_entry(dirEntryPtr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) 
			break;
	}
	
	/* find the first cluster number of the file */
	clusterNum = getClusterNumFromByteArray(dirEntry->DE_byteArray);
	firstClusterNum = clusterNum;
	
	INIT_LIST_HEAD(&clusterList);
		
	/* seek to last non-deleted cluster entry of the chain in the FAT */
	while(clusterNum != END_OF_CHAIN && clusterNum != 0) {
		savedClusterNum = clusterNum;
		
		/* get next cluster number */
		byteArray = readBytes(startOfFAT1 + (clusterNum * 4), 4);
		clusterNum = byteArrayToInt(byteArray, 4, DELETE);
		clusterNum &= FIRST_FOUR_BITS_CLEARED;
			
		
		clusterNumInList = malloc(sizeof(Number));
		clusterNumInList->num = savedClusterNum;
		
		/* add it to list */
		list_add(&clusterNumInList->list, &clusterList);
	}
	
	/* go through list and zero out cluster numbers */
	list_for_each_safe(clusterListPtr, temp, &clusterList) {
		clusterNumInList = list_entry(clusterListPtr, struct NumberStruct, list);
		
		offset = startOfFAT1 + (clusterNumInList->num * 4);
		fseek(imagefd, offset, SEEK_SET);
		
		/* set cluster entry to 0 */
		fwrite(&zero, sizeof(unsigned int), 1, imagefd);
		
		list_del_init(clusterListPtr);
		free(clusterNumInList);
	}
	
	/* delete file's directory entry */
	fseek(imagefd, dirEntry->offsetOfSelf, SEEK_SET);
	fwrite(&E5, sizeof(unsigned char), 1, imagefd);
	
	/* delete from cwd list */
	list_del_init(dirEntryPtr);
	free(dirEntry->DE_byteArray);
	free(dirEntry);
	
	fflush(imagefd);
}

void myRM(char *filename) {
	if (!fileExists(filename)) {
		printf("Error: file does not exist\n");
		return;
	}
	
	if (fileIsDir(filename)) {
		printf("Error: cannot rm a directory\n");
		return;
	}
	
	myRemove(filename);
}

unsigned int findSize(const char *filename)  {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;
	
	list_for_each(ptr, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			return byteArrayToInt(dirEntry->DE_byteArray + 28, 4, KEEP);
		}
	}
}

void mySize(const char *filename) {	
	if (!fileExists(filename)) {
		printf("Error: file does not exist\n");
		return;
	}
	
	printf("%u\n", findSize(filename));
}

/* recursive function               
 * base case: dir == ".", print cwd 
 * recursive case: cd to dir, then ls "."      
 * and cd back to dir */
void myLS(const char *dir, char *cwd) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;
	int i = 1, error, printNewLine = 0;
	char *temp;
	
	if (strcmp(dir, ".") == 0) {
		list_for_each(ptr, &cwdDirEntries) {
			
			if (printNewLine) {
				printf("\n");
				printNewLine = 0;
			}
			
			dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
			printf("%s ", dirEntry->name);
			
			/* print a newline every 10 files/folders */
			if (i++ % NUM_FILES_PER_LINE == 0)
				printNewLine = 1;
		}
		printf("\n");
	}
	
	else if (strcmp(dir, "..") == 0) {
		/* find the second to last '/'
		   so I can save the cwd's name in order to return back */
		temp = getCWDName(cwd);
		
		error = myCD(dir, cwd);  /* change cwd to parent directory */
		if (!error) {
			myLS(".", cwd);      /* print cwd */
			myCD(temp, cwd);     /* change cwd back to the child */
		}
		free(temp);
	}
	
	else {
		error = myCD(dir, cwd); /* change cwd to child directory */
		if (!error) {
			myLS(".", cwd);     /* print cwd */
			myCD("..", cwd);    /* change cwd back to the parent */
		}
	}
}

/* returns 1 if given dir is empty, 0 if not */
int dirIsEmpty(char *dir) {
	struct list_head *ptr;
	DirectoryEntry *dirEntry;
	
	changeDirectory(dir);
	list_for_each(ptr, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		
		if (dirEntry->name[0] != '.') {
			changeDirectory("..");
			return 0;
		}
	}

	changeDirectory("..");
	return 1;
}

void myRmdir(char *dir) {
	if (!fileExists(dir)) {
		printf("Error: directory does not exist\n");
		return;
	}
	
	if (!fileIsDir(dir)) {
		printf("Error: not a directory\n");
		return;
	}
	
	if (dir[0] == '.') {
		printf("Error: cannot remove . or ..\n");
		return;
	}
	
	if (!dirIsEmpty(dir)) {
		printf("Error: directory not empty\n");
		return;
	}
	
	myRemove(dir);
}

void myRead(const char *filename, int position, int numBytesToRead) {
	struct list_head *ptr;
	unsigned int currentClusterNum, offset;
	unsigned int firstDataSector, firstSectorofCluster, i;
	unsigned char *byteArray;
	DirectoryEntry *dirEntry;
	
	if (!fileExists(filename)) {
		printf("Error: file does not exist\n");
		return;
	}
	
	if (fileIsDir(filename)) {
		printf("Error: cannot read a directory\n");
		return;
	}
	
	if (!fileIsOpen(filename)) {
		printf("Error: file must be open\n");
		return;
	}
	
	/* find requested file, check if it is marked for reading */
	list_for_each(ptr, &openFileTable) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			if (strcmp(dirEntry->mode, "w") == 0) {
				printf("Error: file not opened for reading\n");
				return;
			}
			
			break;
		}
	}
	
	if (position < 0) {
		printf("Error: invalid position; position can't be negative\n");
		return;
	}
	
	if (numBytesToRead < 0) {
		printf("Error: invalid numBytesToRead; numBytesToRead can't be negative\n");
		return;
	}
	
	if ((position + numBytesToRead) > findSize(filename)) {
		printf("Error: attempt to read beyond EoF\n");
		return;
	}
	
	/* find the first cluster number of the file */
	currentClusterNum = getClusterNumFromByteArray(dirEntry->DE_byteArray);
	
	/* if position is over cluster boundary, get to the correct cluster */
	while (position > BPB_BytesPerSec - 1) {
		byteArray = readBytes(startOfFAT1 + (currentClusterNum * 4), 4);
		currentClusterNum = byteArrayToInt(byteArray, 4, DELETE);
		currentClusterNum &= FIRST_FOUR_BITS_CLEARED;
		
		position -= BPB_BytesPerSec;
	}
	
	/* find the address of the file's first relevant cluster of data  */
	firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
	firstSectorofCluster = ((currentClusterNum - 2) * BPB_SecPerClus) + firstDataSector;
	offset = firstSectorofCluster * BPB_BytesPerSec;

	/* seek to position */
	offset += position;
	
	while (1) {
		/* read bytes requested */
		byteArray = readBytes(offset, numBytesToRead);
	
		/*  print until numBytesToRead reached, or cluster boundary reached */
		i = 0;
		while (i < numBytesToRead && i + position < BPB_BytesPerSec) {
			printf("%c", byteArray[i++]);
		}
		free(byteArray);
		
		numBytesToRead -= i;

		/* if while reading, it goes over cluster boundary, find the next cluster, and continue reading */
		if (numBytesToRead > 0) {
			byteArray = readBytes(startOfFAT1 + (currentClusterNum * 4), 4);
			currentClusterNum = byteArrayToInt(byteArray, 4, DELETE);
			currentClusterNum &= FIRST_FOUR_BITS_CLEARED;
			
			offset = (((currentClusterNum - 2) * BPB_SecPerClus) + firstDataSector) * BPB_BytesPerSec;
			position = 0;
		}
		else
			break;
	}
	printf("\n");
}

void myWrite(const char *filename, int position, int numBytesToWrite, char *inputString) {
	struct list_head *ptr;
	unsigned int nextClusterNum, offset, currentClusterNum, numBytesWritten = 0;
	unsigned int boundaryClusOffsetReset = 0, newSize, oldPos = position, i, oldSize;
	unsigned int firstDataSector, firstSectorofCluster, numBytesWrittenThisLoop;
	unsigned char *byteArray, *string, *endQuotePtr;
	DirectoryEntry *dirEntry;
	
	oldSize = findSize(filename);
	
	if (!fileExists(filename)) {
		printf("Error: file does not exist\n");
		return;
	}
	
	if (fileIsDir(filename)) {
		printf("Error: cannot write to a directory\n");
		return;
	}
	
	if (!fileIsOpen(filename)) {
		printf("Error: file must be open\n");
		return;
	}
	
	/* find requested file, check if it is marked for writing */
	list_for_each(ptr, &openFileTable) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {
			if (strcmp(dirEntry->mode, "r") == 0) {
				printf("Error: file not opened for writing\n");
				return;
			}
			
			break;
		}
	}
	
	if (position < 0) {
		printf("Error: invalid position; position can't be negative\n");
		return;
	}
	
	if (numBytesToWrite < 0) {
		printf("Error: invalid numBytesToWrite; numBytesToWrite can't be negative\n");
		return;
	}
	
	/* trim quotes from string */
	string = &inputString[1];
	endQuotePtr = strchr(string, '"');
	
	if (endQuotePtr != NULL)
		*endQuotePtr = '\0';
	
	else {
		printf("Error: no end quote at end of string\n");
		return;
	}
	
	if (numBytesToWrite != strlen(string)){
		printf("Error: string length and numBytesToWrite don't match\n");
		return; 
	}
	
	/* find the first cluster number of the file */
	nextClusterNum = getClusterNumFromByteArray(dirEntry->DE_byteArray);
	currentClusterNum = nextClusterNum;
	
	/* if position is over cluster boundary, get to the correct cluster */
	while (position > BPB_BytesPerSec - 1) {
		byteArray = readBytes(startOfFAT1 + (nextClusterNum * 4), 4);
		nextClusterNum = byteArrayToInt(byteArray, 4, DELETE);
		nextClusterNum &= FIRST_FOUR_BITS_CLEARED;
		
		position -= BPB_BytesPerSec;
		
		if (nextClusterNum == END_OF_CHAIN) {
			position = 0;
			break;
		}
		
		currentClusterNum = nextClusterNum;
	}
	
	/* find the address of the file's first relevant cluster of data  */
	if (nextClusterNum != END_OF_CHAIN) {
		firstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * BPB_FATSz32);
		firstSectorofCluster = ((nextClusterNum - 2) * BPB_SecPerClus) + firstDataSector;
		offset = firstSectorofCluster * BPB_BytesPerSec;
	}
	else
		offset = allocateNewCluster(currentClusterNum);
	
	
	/* allocateNewCluster returns 0 if FAT is full */
	if (offset == 0)
		return;
	
	/* seek to position */
	offset += position;
	
	while (1) {
		numBytesWrittenThisLoop = 0;
		
		/*  write until numBytesTowrite reached, or cluster boundary reached */
		while (numBytesWrittenThisLoop < numBytesToWrite && numBytesWrittenThisLoop + position < BPB_BytesPerSec) {
			fseek(imagefd, offset + numBytesWritten - boundaryClusOffsetReset, SEEK_SET);
			fwrite(&string[numBytesWritten], sizeof(unsigned char), 1, imagefd);
			numBytesWrittenThisLoop++;
			numBytesWritten++;
		}
		
		numBytesToWrite -= numBytesWrittenThisLoop;
		
		/* if while writing, it goes over cluster boundary, find the next cluster, and continue writing */
		if (numBytesToWrite > 0) {
			byteArray = readBytes(startOfFAT1 + (nextClusterNum * 4), 4);
			nextClusterNum = byteArrayToInt(byteArray, 4, DELETE);
			nextClusterNum &= FIRST_FOUR_BITS_CLEARED;
			
			if (nextClusterNum != END_OF_CHAIN) 
				offset = (((nextClusterNum - 2) * BPB_SecPerClus) + firstDataSector) * BPB_BytesPerSec;
				
			else 
				offset = allocateNewCluster(currentClusterNum);
				
			boundaryClusOffsetReset = numBytesWritten;
			position = 0;
			
			if (offset == 0)
				return;
		}
		else
			break;
		
		currentClusterNum = nextClusterNum;
	}
	
	/* edit the size information in the directory entry */
	newSize = numBytesWritten + oldPos;
	
	if (newSize < oldSize) 
		return;
	
	byteArray = intToByteArray(newSize);
	
	/* write to file */
	fseek(imagefd, dirEntry->offsetOfSelf + 28, SEEK_SET);
	fwrite(byteArray, 1, 4, imagefd);
	
	fflush(imagefd);
	
	/* write to dirEntry in openFileTable */
	for (i = 0; i < 4; i++) 
		dirEntry->DE_byteArray[28 + i] = byteArray[i];
	
	/* write to dirEntry in cwdDirEntries */
	list_for_each(ptr, &cwdDirEntries) {
		dirEntry = list_entry(ptr, struct DirectoryEntryStruct, list);
		if (strcmp(dirEntry->name, filename) == 0) {	
			for (i = 0; i < 4; i++) 
				dirEntry->DE_byteArray[28 + i] = byteArray[i];
			
			break;
		}
	}
	
	free(byteArray);
}

/* given a line, return a space delimited arg array and return the num args given */
int tokenize(char *line, char *args[5]) {
	int i = 0, j = 0, k = 0;
	char *buf = malloc(sizeof(char) * strlen(line) + 1);
	memset(buf, '\0', sizeof(char) * strlen(line));
	
	while (line[i] != '\0' && line[i] != '\n' && j != 4) {
		if (line[i] == ' ') {
			if (strlen(buf) > 0) {
				args[j] = malloc (sizeof(char) * strlen(buf) + 1);
				strcpy(args[j], buf);
				j++;
			}
			k = ++i;
			memset(buf, '\0', sizeof(char) * strlen(line));
			continue;
		}
		buf[i-k] = line[i];
		i++;
	}
	
	/* if there are 4 args, handle the read and write cases, that need to handle inputting the entire string into a single arg index */
	if (j == 4) {
		while (line[i] != '\0' && line[i] != '\n' && i < strlen(line)) {
			buf[i-k] = line[i];
			i++;
		}
		if (strlen(buf) > 0) {
			args[j] = malloc (sizeof(char) * strlen(buf) + 1);
			strcpy(args[j], buf);
		}
		free(buf);
		return 5;
	}
	
	if (strlen(buf) > 0) {
		args[j] = malloc (sizeof(char) * strlen(buf) + 1);
		strcpy(args[j], buf);
	}
	
	free(buf);
	return ++j;
}

int main(int argc, char **argv) {
	char *line = NULL, *filename = NULL;
	size_t len = 0;
	ssize_t read;
	char cwd[MAX_PATH_SIZE];
	char *args[5];
	int numArgs;
	int i;
	
	strcpy(cwd, "/");
	
	if (argc != 2) {
		printf("Usage: p3.x <fat32 img filename>\n");
		exit(1);
	}
	
	/* get boot sector info and find root directory */
	init(argc, argv);
	
	for (i = 0; i < 5; i++)
		args[i] = NULL;
	
	/* input command loop */
	while(1) {
		/* reset args */
		numArgs = 0;
		for (i = 0; i < 5; i++) {
			if (args[i] != NULL) {
				free(args[i]);
				args[i] = NULL;
			}
			else 
				args[i] = NULL;
		}
		
		/* reset line */
		if (line != NULL) free(line);
		len = 0;
		line = NULL;
		
		/* reset filename */
		if (filename != NULL) free(filename);
		filename = malloc(sizeof(char) * 13);
	
		/* print prompt, get input */
		printf("%s] ", cwd);
		read = getline(&line, &len, stdin);
		
		if (read < 2) continue; 
		
		/* safely split args by delimiter token "space" */
		numArgs = tokenize(line, args);
		
		if (args[1] != NULL) {
			strncpy(filename, args[1], 12);
			convertFileName(filename, NEITHER);
		}
		else {
			printf("Error: not enough arguments\n");
			continue;
		}
		
		if (strcmp(args[0], "open") == 0) {
			if (numArgs == 3)
				myOpen(filename, args[2]);
			
			else 
				printf("Usage: open <filename> <mode>\n");
		}
		else if (strcmp(args[0], "close") == 0) {
			if (numArgs == 2)
				myClose(filename);
			
			else 
				printf("Usage: close <filename>\n");
		}
		else if (strcmp(args[0], "create") == 0) {
			if (numArgs == 2) {
				if (strlen(args[1]) > 12) {
					printf("Error: name too long\n");
					continue;
				}
				myCreate(filename, _FILE);
			}
			else 
				printf("Usage: create <filename>\n");
		}
		else if (strcmp(args[0], "rm") == 0) {
			if (numArgs == 2)
				myRM(filename);

			else 
				printf("Usage: rm <filename>\n");	
		}			
		else if (strcmp(args[0], "size") == 0) {
			if (numArgs == 2)
				mySize(filename);

			else 
				printf("Usage: size <filename>\n");	
		}			
		else if (strcmp(args[0], "cd") == 0) {
			if (numArgs == 2)
				myCD(filename, cwd);

			else 
				printf("Usage: cd <dirname>\n");	
		}			
		else if (strcmp(args[0], "ls") == 0) {
			if (numArgs == 2)
				myLS(filename, cwd);

			else 
				printf("Usage: ls <dirname>\n");	
		}			
		else if (strcmp(args[0], "mkdir") == 0) {
			if (numArgs == 2) {
				if (strlen(args[1]) > 12) {
					printf("Error: name too long\n");
					continue;
				}
				myMkdir(filename);
			}
			else 
				printf("Usage: mkdir <dirname>\n");	
		}			
		else if (strcmp(args[0], "rmdir") == 0) {
			if (numArgs == 2)
				myRmdir(filename);

			else 
				printf("Usage: rmdir <dirname>\n");	
		}			
		else if (strcmp(args[0], "read") == 0) {
			if (numArgs == 4)
				myRead(filename, atoi(args[2]), atoi(args[3]));

			else 
				printf("Usage: read <filename> <position> <num bytes>\n");	
		}			
		else if (strcmp(args[0], "write") == 0) {
			if (numArgs == 5)
				myWrite(filename, atoi(args[2]), atoi(args[3]), args[4]);

			else 
				printf("Usage: write <filename> <position> <num bytes> <\"string\">\n");	
		}			
		else 
			printf("Error: Unknown command.\n");
	}
	
	return 0;
}
