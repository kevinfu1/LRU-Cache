#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

FILE *file;

struct frame {
    unsigned int startAddress;
    int tag;
    int valid;
    char blockData[64];
    int size;
    int lastUsed;
};

void hexToString(char *source, int startIdx, char *hex, int size) {
    for (int i=startIdx; i<startIdx+size; i++) {
        sscanf(hex, "%2hhx", &source[i]);
        hex+=2; 
    } 
}

void printData(char *data, int startIdx, int size) {
    for (int i=startIdx; i<startIdx+size; i++) {
        printf("%02hhx", data[i]);
    }
}

/*num frames = capacity/block size */

int main (int argc, char *argv[]) {
    char line[100];
    file = fopen (argv[1], "r");

    int cachSize = (atoi(argv[2]))*1024;
    int way = atoi(argv[3]);
    char writeType[2];
    strcpy(writeType, argv[4]);
    int blockSize = atoi(argv[5]);
    char type[5];
    unsigned int address;
    int accessSize;
    char value[128];
    char* valuePointer = value;
    bool wt = false;

    if (strcmp(writeType, "wt") ==0) {
        wt = true;
    }

    int numFrames = cachSize/blockSize;
    int sets = numFrames/way;

    char memory[(int) pow(2.0, 16.0)];
    char* memoryPointer = memory;

    struct frame** cache = (struct frame**)malloc(sizeof(struct frame*)*numFrames);

    for (int i=0; i<numFrames; i++) {
        struct frame* row = (struct frame*)malloc(sizeof(struct frame)*sets);
        cache[i] = row;
    }
    for (int i = 0; i<numFrames; i++) {
        for (int j=0; j<sets; j++) {
            cache[i][j].valid = 0;
            cache[i][j].lastUsed = 0;
        }
    }

    while (fgets(line, 100, file) != NULL) {
        sscanf(line, "%s %04x %d %s", type, &address, &accessSize, value);
        int blockOffset = address % blockSize;
        int index = (address / blockSize) % sets;
        int tag = address/(sets*blockSize);
        int max = 0;
        int LRUpos;
        for (int i=0; i<numFrames; i++) {  
            cache[index][i].startAddress = address - blockOffset;
            
            if (cache[index][i].valid == 1 && cache[index][i].tag == tag) { 
                    /*hit*/
                if (strcmp(type, "store") == 0) {
                    hexToString(cache[index][i].blockData, blockOffset, value, accessSize);
                    for (int k = 0; k < numFrames; k++){
                        cache[index][k].lastUsed++;
                    }
                    cache[index][i].lastUsed = 0;
                    if (wt) {
                        hexToString(memoryPointer, address, value, accessSize);    
                    }
                    printf("%s %04x %s\n", "store", address, "hit");
                } else {
                    printf("%s %04x %s ", "load", address, "hit");
                    printData(cache[index][i].blockData, blockOffset, accessSize);
                    printf("\n");
                    for (int k = 0; k < numFrames; k++){
                        cache[index][k].lastUsed++;
                    }
                    cache[index][i].lastUsed = 0;
                }
                break;
            
            } else if(cache[index][i].valid == 0) {
                int full = 0;
                if (strcmp(type, "store") == 0) {  
                    if (!wt) {
                        cache[index][i].size = accessSize;
                        cache[index][i].valid = 1;
                        cache[index][i].tag = tag;
                        cache[index][i].lastUsed = 0;
                        for (int k = 0; k < blockSize; k++){
                            cache[index][i].blockData[k] = memory[address-blockOffset+k];
                        }
                        hexToString(cache[index][i].blockData, blockOffset, value, accessSize);
                    } else {
                        hexToString(memoryPointer, address, value, accessSize);
                    } 
                    printf("%s %04x %s\n", "store", address, "miss");
                    break;
                } else {
                    cache[index][i].size = accessSize;
                    cache[index][i].valid = 1;
                    cache[index][i].tag = tag;
                    cache[index][i].lastUsed = 0;
                    for (int k = 0; k < blockSize; k++){
                        cache[index][i].blockData[k] = memory[address-blockOffset+k];
                    }
                    printf("%s %04x %s ", "load", address, "miss");
                    printData(memoryPointer, address, accessSize);
                    printf("\n");
                    break;
                }
                
                break;
            }
            if (i == numFrames-1 && cache[index][i].valid == 1) {
                for (int j = 0; j < numFrames; j++) {
                    if (cache[index][j].lastUsed > max) {
                        max = cache[index][j].lastUsed;
                        LRUpos = j;
                    }
                }
                if (strcmp(type, "store")==0) {
                    if (!wt) {
                        for (int k = 0; k < blockSize; k++){
                            memory[cache[index][LRUpos].startAddress+k] = cache[index][LRUpos].blockData[k];
                            cache[index][LRUpos].blockData[k] = memory[address-blockOffset+k];
                        }
                        hexToString(cache[index][LRUpos].blockData, blockOffset, value, accessSize);
                        cache[index][LRUpos].size = accessSize;
                        cache[index][LRUpos].tag = tag;
                        cache[index][LRUpos].lastUsed = 0; 
                    } else {
                        hexToString(memoryPointer, address, value, accessSize);  
                    }    
                    printf("%s %04x %s\n", "store", address, "miss");
                } else {
                    if (!wt) {
                        for (int k = 0; k < blockSize; k++){
                            memory[cache[index][LRUpos].startAddress+k] = cache[index][LRUpos].blockData[k];
                        }
                    }
                    for (int k = 0; k < blockSize; k++){
                        cache[index][LRUpos].blockData[k] = memory[address-blockOffset+k];
                    }
                    cache[index][LRUpos].size = accessSize;
                    cache[index][LRUpos].tag = tag;
                    cache[index][LRUpos].lastUsed = 0;
                    printf("%s %04x %s ", "load", address, "miss");
                    printData(memoryPointer, address, accessSize);
                    printf("\n");
                }    
                
                max = 0;
            }
            
        }

    }

    fclose(file);
}
