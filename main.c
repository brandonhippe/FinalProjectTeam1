#include "main.h"

/*
* TODO-LIST FOR FINAL PROJECT
* 
* read function 
* write function
* snoop functions
* clear all function
* print all function
* 
* 
*/


int main(int argc, char *argv[]) {
    char *filename;

    // Need to have filename and mode select
    if(argc != 3) {
        printf("Two arguments required!");
        exit(-1);
    }

    // Determines what mode parses filename
    while(*++argv) {
        if((*argv)[0] == '-') {
            switch((*argv)[1]) {
                
                case 's':
                    printf("Silent Mode Enabled...\n");
                    silentMode = true;
                    break;
                case 'n':
                    printf("Nornal Mode Enabled...\n");
                    break;
                default:
                    printf("Unknonw option -%c\n", (*argv)[1]);
                    exit(-1);
            }
        }
        else {
            filename = (*argv);
        }
    }

    trace = traceParser(filename);
    resetCache();
    for(int i = 0; i < trace.size; i++) {
        switch(trace.command[i]) {
            case(READREQ):
                read(trace.address[i]);
                break;
            case(WRITEREQ):
                write(trace.address[i]);
                break;
            case(READREQI):
                break;
            case(SNOOPINVALD):
                break;
            case(SNOOPREAD):
                break;
            case(SNOOPWRITE):
                break;
            case(SNOOPRWIN):
                break;
            case(CLR):
                resetCache();
                break;
            case(PRINT):
                printValid(trace.address[i]);
                break;
            default:
                exit(-1);
        }
        output.cacheRatio = output.cacheHits / (float)(output.cacheReads + output.cacheWrites);
        printOutput();
    }

    return 0;
}

void read(unsigned int address) {
    Derived values;
    int way;
    int snoopResult;
    output.cacheReads++;

    values = addressParser(address);
    way = findHit(values);
    
    if (way != -1) {
        // Hit
        output.cacheHits++;
        switch (cache[values.index].lines[way].MESI) {
            case 'M':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'E':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'S':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'I':
                busOperation(READ, address, &snoopResult);
                writeMESI(values.index, way, &snoopResult);
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            default:
                exit(-1);
        }
    }
    else {
        output.cacheMiss++;
        way = findEmpty(values.index);
        if (way != -1) {
            // Miss (Empty)
            busOperation(READ, address, &snoopResult);
            writeMESI(values.index, way, &snoopResult);
            cache[values.index].lines[way].tag = values.tag;
            updateLRU(values.index, way);
            messageToCache(SENDLINE, address);
        }
        else {
            // Miss (Needs Evict)
            way = getLRU(values.index);
            switch (cache[values.index].lines[way].MESI) {
                case 'M':
                    messageToCache(EVICTLINE, address);
                    busOperation(WRITE, address, &snoopResult);
                    writeMESI(values.index, way, &snoopResult);
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
                case 'E':
                    messageToCache(EVICTLINE, address);
                    busOperation(READ, address, &snoopResult);
                    writeMESI(values.index, way, &snoopResult);
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
                case 'S':
                    messageToCache(EVICTLINE, address);
                    busOperation(READ, address, &snoopResult);
                    cache[values.index].lines[way].MESI = 'S';
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
                case 'I':
                    messageToCache(EVICTLINE, address);
                    busOperation(READ, address, &snoopResult);
                    writeMESI(values.index, way, &snoopResult);
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
            }

        }
    }
}

void write(unsigned int address) {
    Derived values;
    int way;
    int snoopResult;
    output.cacheReads++;

    values = addressParser(address);
    way = findHit(values);

    if (way != -1) {
        // Hit
        output.cacheHits++;
        switch (cache[values.index].lines[way].MESI) {
        case 'M':
            updateLRU(values.index, way);
            messageToCache(GETLINE, address);
            break;
        case 'E':
            updateLRU(values.index, way);
            cache[values.index].lines[way].MESI = 'M';
            messageToCache(GETLINE, address);
            break;
        case 'S':
            busOperation(INVALIDATE, address, &snoopResult);
            cache[values.index].lines[way].MESI = 'M';
            updateLRU(values.index, way);
            messageToCache(SENDLINE, address);
            break;
        case 'I':
            busOperation(READ, address, &snoopResult);
            writeMESI(values.index, way, &snoopResult);
            updateLRU(values.index, way);
            messageToCache(SENDLINE, address);
            break;
        default:
            exit(-1);
        }
    }
    else {
        output.cacheMiss++;
        way = findEmpty(values.index);
        if (way != -1) {
            // Miss (Empty)
            busOperation(READ, address, &snoopResult);
            messageToCache(GETLINE, address);
            cache[values.index].lines[way].MESI = 'M';
            cache[values.index].lines[way].tag = values.tag;
            updateLRU(values.index, way);
        }
        else {
            // Miss (Needs Evict)
            way = getLRU(values.index);
            switch (cache[values.index].lines[way].MESI) {
            case 'M':
                messageToCache(EVICTLINE, address);
                busOperation(WRITE, address, &snoopResult);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            case 'E':
                messageToCache(EVICTLINE, address);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].MESI = 'M';
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            case 'S':
                messageToCache(EVICTLINE, address);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].MESI = 'M';
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            case 'I':
                messageToCache(EVICTLINE, address);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].MESI = 'M';
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            }

        }
    }
}

int snoopInvaild() {
    // Parse address
    // check cache at index for same tag
    // if hit, invalidate the line
    // report to L1 to invalidate line
    // report snoop results to bus
    // else,
    // report snoop results to bus
}

int snoopRead() {
    // Parse address
    // check cache at index for same tag
    // if hit, set mesi bit
    // I to S
    // S to to S
    // E to S 
    // M to M
    // updateLRU
    // else,
    // report snoop results to bus
}

int snoopWrite() {
    // Parse address
    // check cache at index for same tag
    // if hit, set mesi bit
    // I to M 
    // E to M
    // S to M
    // M to M
    // updateLRU
    // else,
    // report snoop results to bus
}   

int snoopReadM() {
    // Parse address
    // check cache at index for same tag
    // if hit, set mesi bit
    // I to I
    // S to I
    // E to I
    // M to I, bus write, flush back to main memory
    // updateLRU
    // else
    // report snoop results to bus
}

void updateLRU(unsigned int setIndex, int way) {
    unsigned int PLRU = cache[setIndex].PLRU;

    switch(way) {
        case 0:
            PLRU = PLRU & 0x74;
            break;
        case 1:
            PLRU = (PLRU & 0x74) | 0x8;
            break;
        case 2:
            PLRU = (PLRU & 0x6C) | 0x2;
            break;
        case 3:
            PLRU = (PLRU & 0x6C) | 0x12;
            break;
        case 4:
            PLRU = (PLRU & 0x6A) | 0x1;
            break;
        case 5:
            PLRU = (PLRU & 0x6A) | 0x21;
            break;
        case 6:
            PLRU = (PLRU & 0x3A) | 0x5;
            break;
        case 7:
            PLRU = (PLRU & 0x3A) | 0x45;
            break;
    }
    cache[setIndex].PLRU = PLRU;
}

int getLRU(unsigned int setIndex) {
    int way;
    unsigned int PLRU = cache[setIndex].PLRU;

    if( (PLRU & 0x1) == 0 ) {
        if( ((PLRU >> 2) & 0x1) == 0 ) {
            if( ((PLRU >> 3) & 0x1) == 0 ) {
                way = 6;
            }
            else {
                way = 7;
            }
        }
        else {
            if( ((PLRU >> 6) & 0x1) == 0 ) {
                way = 5;
            }
            else {
                way = 4;
            }
        }
    }
    else {
        if( ((PLRU >> 1) & 0x1) == 0 ) {
            if( ((PLRU >> 5) & 0x1) == 0 ) {
                way = 3;
            }
            else {
                way = 2;
            }
        }
        else {
            if( ((PLRU >> 4) & 0x1) == 0 ) {
                way = 0;
            }
            else {
                way = 1;
            }
        }
    }

    return way;
}

int findEmpty(unsigned int setIndex) {
    int way;

    for (int i = 0; i < NUMWAYS; i++) {
        if (cache[setIndex].lines[i].MESI == 'I') {
            return way = i;
        }
    }
    return -1;
}

int findHit(Derived values) {
    int way;
    for (int i = 0; i < NUMWAYS; i++) {
        if (cache[values.index].lines[i].tag == values.tag) {
            return way = i;
        }
    }
    return -1;
}

void messageToCache(int Message, unsigned int Address) {
    if(!silentMode) {
        printf("L2: %s %x\n", messageChar[Message], Address);
    }
}

void printValid(unsigned int address) {
    Derived values;
    values = addressParser(address);
    printf("\n===================================================================\n");
    printf("|                          Cache Set: %d                        |", values.index);
    printf("\n===================================================================\n");
    for (int i = 0; i < NUMWAYS; i++) {
        if (cache[values.index].lines[i].MESI == 'I') {
            continue;
        }
        else {
            printf("                            ------------\n");
            printf("                            Line   : %d\n",    i);
            printf("                            Tag    : 0x%x\n",  cache[values.index].lines[i].tag);
            printf("                            MESI   : %c\n",  cache[values.index].lines[i].MESI);
            printf("                            ------------\n");
        }
    }
}

void printOutput() {
    printf("\n===================================================================\n");
    printf("|                              Stats                              |");
    printf("\n===================================================================\n");
    printf("                    Total Cache Reads:    %d\n",    output.cacheReads);
    printf("                    Total Cache Writes:   %d\n",      output.cacheWrites);
    printf("                    Total Cache Hits:     %d\n",      output.cacheHits);
    printf("                    Total Cache Misses:   %d\n",      output.cacheMiss);
    printf("                    Cache Hit Ratio:      %.2f\n\n",  output.cacheRatio);
}

void resetCache() {
    for (int i = 0; i < NUMSETS; i++) {
        for (int j = 0; j < NUMWAYS; j++) {
            cache[i].lines[j].MESI = 'I';
            cache[i].lines[j].tag = 0;
        }
    }

    output.cacheHits = 0;
    output.cacheMiss = 0;
    output.cacheRatio = 0;
    output.cacheReads = 0;
    output.cacheWrites = 0.0;
}

void writeMESI(unsigned int setIndex, int way, int *snoopResult) {
    if (*snoopResult == NOHIT) {
        cache[setIndex].lines[way].MESI = 'E';
    }
    else if (*snoopResult == HIT || *snoopResult == HITM) {
        cache[setIndex].lines[way].MESI = 'S';
    }
}

void busOperation(int BusOp, unsigned int Address, int *SnoopResult) {
    *SnoopResult = getSnoopResult(Address);
    if (!silentMode) {
        printf("BusOp: %s, Address : %x, Snoop Result : %s\n", busChar[BusOp], Address, snoopChar[*SnoopResult]);
    }
}

int getSnoopResult(unsigned int Address) {
    int snoopRes;

    snoopRes = (Address & BYTEMASK);

    switch (snoopRes) {
        case NOHIT:
            return NOHIT;
        case HIT:
            return HIT;
        case HITM:
            return HITM;
        default:
            return NOHIT;
    }
}

void putSnoopResult(unsigned int Address, int SnoopResult) {
    if (!silentMode) {
        printf("SnoopResult: Address %x, SnoopResult : %s\n", Address, snoopChar[SnoopResult]);
    }
}

Derived addressParser(unsigned int address) {
    Derived values;

    values.index = (address >> BYTEOFF) & INDEXMASK;
    values.tag = (address >> (BYTEOFF + INDEXOFF)) & TAGMASK;
    return values;
}

TraceValues traceParser(char *filename) {
    FILE *fp;
    TraceValues trace;
    int i = 0;
    fp = fopen(filename, "r");

    if(fp == NULL) {
        printf("Unable to open file!\n");
        exit(-1);
    }
    /*
    if(!silentMode) {
        printf("===Command===|===Address===\n");
    }
    */

    // Reads and parses file into struct
    while(fscanf(fp, "%d %x", &trace.command[i], &trace.address[i]) != EOF) {
        /*
        if(!silentMode) {
            printf("      %d          %x\n", trace.command[i], trace.address[i]);
        }
        */
        i++;
    }
    trace.size = i;

    return trace;
}





