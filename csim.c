#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

const char *help =  "Options:\n"
                    "  -h         Print this help message.\n"
                    "  -v         Optional v flag.\n"
                    "  -s <num>   Number of set index bits.\n"
                    "  -E <num>   Number of lines per set.\n"
                    "  -b <num>   Number of block offset bits.\n"
                    "  -t <file>  Trace file.\n\n"
                    "Examples:\n"
                    "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
                    "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace";

int main(int argc, char *argv[])
{
    int v = 0, s = 0, E = 0, b = 0;
    FILE *trace = NULL;
    int opt;
    char op;

    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:")))
    {
        switch (opt)
        {
            case 'h': printf("%s", help); break;
            case 'v': v = 1; break;
            case 's': s = atoi(optarg); break;
            case 'E': E = atoi(optarg); break;
            case 'b': b = atoi(optarg); break;
            case 't': trace = fopen(optarg, "r"); break;
            default : break;
        }
    }

    if (trace == NULL || s <= 0 || E <= 0 || b<= 0)
    {
        printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <trace>\n");
        return 0;
    }

    int sets = 1 << s;
    long **cache;

    cache = malloc(sets * sizeof(long *));

    int i, j;
    for (i = 0; i < sets; i++)
    {
        cache[i] = malloc(E * sizeof(long));
        for (j = 0; j < E; j++)
            cache[i][j] = -1;
    }

    unsigned addr;
    int bytes;
    int hit = 0, miss = 0, eviction = 0;
    int id;

    while (fscanf(trace, " %c %x,%d", &op, &addr, &bytes) > 0)
    {
        if (op == 'I') continue;
        if (v)
            printf("%c %x,%d ", op, addr, bytes);
        addr >>=  b;
        long *tag = cache[addr & (sets - 1)];

        
access: 
        id = -1;
        for (i = 0; i < E; i++)
        {
            if (tag[i] == addr) {
                id = i;
                hit++;
                if (v)
                    printf(" hit");
                break;
            }
        }
        if (id == -1)
        {
            miss++;
            if(v)
                printf(" miss");
            if(tag[E-1] != -1)
            {
                if (v)
                    printf(" eviction");
                eviction++;
            }
            id = E - 1;
            tag[id] = addr;
        }
        
        long tmp = tag[id];
        for (int i = id; i > 0; i--)
            tag[i] = tag[i - 1];
        tag[0] = tmp;

        if (op == 'M'){
            op = 'S';
            goto access;
        }
        
        printf("\n");
    }

    fclose(trace);
    for (int i = 0; i < sets; i++)
        free(cache[i]);
    free(cache);

    printSummary(hit, miss, eviction);

    return 0;
}
