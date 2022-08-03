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
    int lastUsed;
};

void hexToString(unsigned char *source, int startIdx, char *hex, int size) {
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
    char writeType[3];
    strcpy(writeType, argv[4]);
    int blockSize = atoi(argv[5]);
    char type[5];
    unsigned int address;
    int accessSize;
    char value[128];
    bool wt = false;

    if (strcmp(writeType, "wt") ==0) {
        wt = true;
    }

    int numFrames = cachSize/blockSize;
    int sets = numFrames/way;

    unsigned char memory[16384];
    unsigned char* memoryPointer = memory;

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
        sscanf(line, "%s %x %d %s", type, &address, &accessSize, value);
        int blockOffset = address % blockSize;
        int index = (address / blockSize) % sets;
        int tag = address/(sets*blockSize);
        int max = cache[index][0].lastUsed;
        int LRUpos = 0;
        char hit_miss[5];
        int hitIdx;

        strcpy(hit_miss, "miss");

        for (int k = 0; k < numFrames; k++){
            if (cache[index][k].lastUsed > max) {
                max = cache[index][k].lastUsed;
                LRUpos = k;
            }
            if (cache[index][k].valid == 1 && cache[index][k].tag == tag){
                strcpy(hit_miss, "hit");
                hitIdx = k;
            }
        }
        if (strcmp(hit_miss, "hit")==0) {
            if (strcmp(type, "load") == 0) {
                printf("%s %04x %s ", "load", address, hit_miss);
                for (int k=blockOffset; k<blockOffset+accessSize; k++) {
                    printf("%02hhx", cache[index][hitIdx].blockData[k]);
                }
                printf("\n");
                for (int k = 0; k < numFrames; k++){
                    cache[index][k].lastUsed++;
                }
                cache[index][hitIdx].lastUsed = 0;
            } else {
                char *valuePointer = value;
                for (int i = blockOffset; i < blockOffset + accessSize; i++) {
                    sscanf(valuePointer, "%02hhx", &cache[index][hitIdx].blockData[i]);
                    valuePointer+=2;
                }
                for (int k = 0; k < numFrames; k++){
                    cache[index][k].lastUsed++;
                }
                cache[index][hitIdx].lastUsed = 0;
                if (wt) {
                    char *valuePointer = value;
                    hexToString(memoryPointer, address, valuePointer, accessSize);    
                }
                printf("%s %04x %s\n", "store", address, hit_miss);
            }
        } else if (strcmp(hit_miss, "miss") == 0) {
            int full = 0;
            if (strcmp(type, "load") == 0) { 
                for (int i=0; i<numFrames; i++) {
                    if (cache[index][i].valid == 0) {
                        for (int k = 0; k < blockSize; k++){
                            cache[index][i].blockData[k] = memory[address-blockOffset+k];
                        }
                        full = 1;
                        cache[index][i].valid = 1;
                        cache[index][i].tag = tag;
                        cache[index][i].startAddress = address - blockOffset;
                        cache[index][i].lastUsed = 0;
                        break;
                    }
                    if (full==1) break;
                }
                if (full == 0) {
                    if (!wt) {
                        for (int k = 0; k < blockSize; k++){
                            memory[cache[index][LRUpos].startAddress+k] = cache[index][LRUpos].blockData[k];
                        }
                    }
                    for (int k = 0; k < blockSize; k++){
                        cache[index][LRUpos].blockData[k] = memory[address-blockOffset+k];
                    }
                    cache[index][LRUpos].tag = tag;
                    cache[index][LRUpos].lastUsed = 0;
                    cache[index][LRUpos].startAddress = address - blockOffset;
                }
                for (int k = 0; k < numFrames; k++){
                    cache[index][k].lastUsed++;
                }
                printf("%s %04x %s ", "load", address, hit_miss);
                for (int j = address; j<address+accessSize; j++){
                    printf("%02hhx", memory[j]);
                }
                printf("\n");
            } else {
               if (wt) {
                    char *valuePointer = value;
                    hexToString(memoryPointer, address, valuePointer, accessSize);
                } else { 
                    for (int i=0; i<numFrames; i++) {
                        if (cache[index][i].valid == 0) {
                            for (int k = 0; k < blockSize; k++){
                                cache[index][i].blockData[k] = memory[address-blockOffset+k];
                            }
                            char *valuePointer = value;
                            for (int k = blockOffset; k < blockOffset + accessSize; k++){
                                sscanf(valuePointer, "%02hhx", &cache[index][i].blockData[k]);
                                valuePointer+=2;
                            }
                            cache[index][i].startAddress = address - blockOffset;
                            cache[index][i].valid = 1;
                            cache[index][i].tag = tag;
                            cache[index][i].lastUsed = 0;
                            full = 1;
                            break;
                        }
                        if (full == 1) break;
                    } 
                    if (full==0) {
                        for (int k = 0; k < blockSize; k++){
                            memory[cache[index][LRUpos].startAddress+k] = cache[index][LRUpos].blockData[k];
                            cache[index][LRUpos].blockData[k] = memory[address-blockOffset+k];
                        }
                        char *valuePointer = value;
                        for (int k = blockOffset; k < blockOffset + accessSize; k++){
                            sscanf(valuePointer, "%02hhx", &cache[index][LRUpos].blockData[k]);
                            valuePointer+=2;
                        }
                        cache[index][LRUpos].tag = tag;
                        cache[index][LRUpos].lastUsed = 0; 
                        cache[index][LRUpos].startAddress = address - blockOffset;
                    }
                    for (int k = 0; k < numFrames; k++){
                        cache[index][k].lastUsed++;
                    }
                } 
                printf("%s %04x %s\n", "store", address, hit_miss); 
            }
            
        }

    }

    fclose(file);

    return 0;
}
