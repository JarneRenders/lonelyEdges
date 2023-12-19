/**
 * lonelyEdges.c
 * 
 * Author: Jarne Renders (jarne.renders@kuleuven.be)
 *
 */

#define USAGE "Usage: ./lonelyEdges [-o#|-d|-a] [-vmh]"
#define HELPTEXT "Helptext: Program for finding the lonely edges of a graph.\n\
\n\
Input graphs should be in graph6 format. Without any parameters, the program\n\
outputs those graphs which contain at least one lonely edge.\n\
\n\
  -o# : Only output those graphs with exactly # lonely edges; Not compatible\n\
        with option -a or -d\n\
  -d  : For every graph in the input, output all of its children which have\n\
        the same number of lonely edges; The children might be isomorphic as\n\
        abstract graphs; Not compatible with option -a or -o#\n\
  -a  : Output all children of the input graphString; The children might be\n\
        isomorphic as abstract graphs; Not compatible with option -d or -o#\n\
  -v  : Output extra information, such as the labeling of each graph and which\n\
        lonely edges it has\n\
  -m  : Output all perfect matchings of each graph; Best to combine with -v\n\
  -h  : Print this helptext"

#define getIndex(g,x,y) (g)->edgeIndices[(g)->numberOfVertices*(x)+(y)]

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include "readGraph/readGraph6.h"
#include "bitset.h"

struct options {
    bool allChildrenFlag;
    bool printMatchingsFlag;
    bool printChildrenWithSameNumber;
    bool verboseFlag;
};

struct graph {
    int numberOfVertices;
    bitset* adjacencyList;
    int *edgeIndices;
    int *labelToEdge;
}; 

//******************************************************************************
//
//                  Methods for reading/writing graphs
//
//******************************************************************************

//  Print in readable format to stderr.
void printGraph(struct graph *g) {
    for(int i = 0; i < g->numberOfVertices; i++) {
        fprintf(stderr, "%d: ", i);
        forEach(nbr, g->adjacencyList[i]) {
            fprintf(stderr, "%d ", nbr);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

//  Print in graph6 format to stdout.
void writeToG6(bitset adjacencyList[], int numberOfVertices) {
    char graphString[8 + numberOfVertices*(numberOfVertices - 1)/2];
    int pointer = 0;

    //  Save number of vertices in the first one, four or 8 bytes.
    if(numberOfVertices <= 62) {
        graphString[pointer++] = (char) numberOfVertices + 63;
    }
    else if(numberOfVertices <= 258047) {
        graphString[pointer++] = 63 + 63;
        for(int i = 2; i >= 0; i--) {
            graphString[pointer++] = (char) (numberOfVertices >> i*6) + 63;
        }
    }
    else if(numberOfVertices <= 68719476735) {
        graphString[pointer++] = 63 + 63;
        graphString[pointer++] = 63 + 63;
        for(int i = 5; i >= 0; i--) {
            graphString[pointer++] = (char) (numberOfVertices >> i*6) + 63;
        }
    }
    else {
        fprintf(stderr, "Error: number of vertices too large.\n");
        exit(1);
    }

    // Group upper triangle of adjacency matrix in groups of 6. See B. McKay's 
    // graph6 format.
    int counter = 0;
    char charToPrint = 0;
    for(int i = 1; i < numberOfVertices; i++) {
        for(int j = 0; j < i; j++) {
            charToPrint = charToPrint << 1;
            if(contains(adjacencyList[i], j)) {
                charToPrint |= 1;
            }
            if(++counter == 6) {
                graphString[pointer++] = charToPrint + 63;
                charToPrint = 0;
                counter = 0;
            }
        }
    }

    //  Pad final character with 0's.
    if(counter != 0) {
        while(counter < 6) {
            charToPrint = charToPrint << 1;
            if(++counter == 6) {
                graphString[pointer++] = charToPrint + 63;
            }
        }
    }

    //  End with newline and end of string character.
    graphString[pointer++] = '\n';
    graphString[pointer++] = '\0';
    printf("%s", graphString);
}

//  Give an index from 0 to |E(G)|-1 to each edge of G.
int numberEdges(struct graph *g) {

    int index = 0;
    for(int i = 0; i < g->numberOfVertices; i++) {
        forEachAfterIndex(nbr, g->adjacencyList[i], i) {

            // List for mapping edge to index.
            g->edgeIndices[g->numberOfVertices*i + nbr] = index;
            g->edgeIndices[g->numberOfVertices*nbr + i] = index;

            // List for mapping index to edge.
            if(i < nbr) {
                g->labelToEdge[2*index] = i;
                g->labelToEdge[2*index + 1] = nbr;
            }

            index++;

            // Edges are stored in a bitset which at most has size
            // MAXBITSETSIZE.
            if(index > MAXBITSETSIZE) {
                return -1;
            }

        }
    }

    return 0;
}

//  Read graph from graphString in graph6 format and put data in graph struct g.
bool readGraph(struct graph *g, struct options *options, char *graphString) {
    g->numberOfVertices = getNumberOfVertices(graphString);
    if(g->numberOfVertices == -1 || g->numberOfVertices > MAXBITSETSIZE) {
        if(options->verboseFlag){
            fprintf(stderr, "Skipping invalid graph!\n");
        }
        return false;
    }
    g->adjacencyList = malloc(sizeof(bitset) * g->numberOfVertices);
    if(loadGraph(graphString, g->numberOfVertices, g->adjacencyList) == -1) {
        if(options->verboseFlag){
            fprintf(stderr, "Skipping invalid graph!\n");
        }
        free(g->adjacencyList);
        return false;
    }
    g->edgeIndices = 
     malloc(sizeof(int) * (g->numberOfVertices * g->numberOfVertices));
    g->labelToEdge =
     malloc(sizeof(int) * (g->numberOfVertices * g->numberOfVertices));
    if(numberEdges(g) == -1) {
        free(g->adjacencyList);
        free(g->edgeIndices);
        free(g->labelToEdge);
        return false;
    }
    return true;
}

void freeGraph(struct graph *g) {
    free(g->adjacencyList);
    free(g->labelToEdge);
    free(g->edgeIndices);
}

//******************************************************************************
//
//                      Methods for finding lonely edges
//
//******************************************************************************

//  Generate all perfect matchings of the graph and keep track of the edges
//  which belong to a perfect matching and those belonging to at least 2
//  perfect matchings.
void generatePerfectMatchings(struct graph *g, struct options *options,
 bitset remainingVertices, bitset matching, bitset *edgesHit, 
 bitset *edgesHitTwice) {

    //  If there is no remaining vertex, we have a perfect matching.
    int nextVertex = next(remainingVertices, -1);
    if(nextVertex == -1) {

        if(options->printMatchingsFlag) {
            forEach(e, matching) {
                fprintf(stderr, "%d-%d ",
                 g->labelToEdge[2*e], g->labelToEdge[2*e+1]);
            }
            fprintf(stderr, "\n");
        }

        *edgesHitTwice =
         union(*edgesHitTwice, intersection(*edgesHit, matching));
        *edgesHit = union(*edgesHit, matching);

        return;
    }

    //  If not yet a perfect matching, try recursively all (still available)
    //  edges incident to the first remaining vertex.
    forEach(neighbor,
     intersection(g->adjacencyList[nextVertex], remainingVertices)) {

        bitset newMatching =
         union(matching, singleton(getIndex(g, nextVertex, neighbor)));
        bitset newRemainingVertices = difference(remainingVertices,
         union(singleton(nextVertex), singleton(neighbor)));

        generatePerfectMatchings(g, options, newRemainingVertices, newMatching,
         edgesHit, edgesHitTwice);

    }
}

//  Store the graph g with v blown up to a triangle in newG and print it.
//  Assumes that g is a cubic graph. 
void blowUpToTriangle(struct graph *g, int v, struct graph *newG) {

    newG->numberOfVertices = g->numberOfVertices + 2;
    newG->adjacencyList = malloc(sizeof(bitset) * newG->numberOfVertices);
    for(int j = 0; j < g->numberOfVertices; j++) {
        newG->adjacencyList[j] = g->adjacencyList[j];
    }
    newG->adjacencyList[g->numberOfVertices + 1] =
     union(singleton(v),singleton(g->numberOfVertices));
    newG->adjacencyList[g->numberOfVertices] =
     union(singleton(v),singleton(g->numberOfVertices + 1));

    int k = 0;
    forEach(nbr, g->adjacencyList[v]) {
        if(k == 2) {
            continue;
        }
        removeElement(newG->adjacencyList[nbr], v);
        removeElement(newG->adjacencyList[v], nbr);
        add(newG->adjacencyList[nbr], g->numberOfVertices + k);
        add(newG->adjacencyList[g->numberOfVertices + k], nbr);
        add(newG->adjacencyList[v], g->numberOfVertices + k);
        k++;
    }

    writeToG6(newG->adjacencyList, newG->numberOfVertices);
}

//  Given a graph g, count and print all children, i.e. graphs obtained from g
//  by blowing a vertex up to a triangle, which have the same number of lonely
//  edges as g. We do not generate all such children but use the statement
//  about v-joins from the article.
long long unsigned int generateChildrenWithSameNumberOfLonelyEdges(
 struct graph *g, struct options *options, bitset lonelyEdgesOfg) {

    long long unsigned int counter = 0;
    for(int v = 0; v < g->numberOfVertices; v++) {

        //  For each v compute the v-join (join where v is the unique degree 3
        //  vertex.)
        bitset edgesHitByvJoin = EMPTY;
        bitset edgesHitTwice = EMPTY;
        bitset matching = EMPTY;
        bitset remainingVertices = complement(singleton(v), g->numberOfVertices);
        remainingVertices = difference(remainingVertices, g->adjacencyList[v]);

        //  If v and its neighbours are not remaining, finding all v-joins is
        //  the same procedure as finding all perfect matchings of the graph
        //  induced by the remaining vertices.
        generatePerfectMatchings(g, options, remainingVertices, matching,
         &edgesHitByvJoin, &edgesHitTwice);

        //  Edges incident with v are never in edgesHitByvJoin as v is not in
        //  remainingVertices at start, but it is not a problem, since if they
        //  are lonely, the stop being lonely but give rise to a new lonely
        //  edge in the new triangle.
        if(size(intersection(edgesHitByvJoin, lonelyEdgesOfg)) == 0) {
            if(options->verboseFlag) {
                fprintf(stderr, "Blowing up %d\n", v);
            }
            struct graph newG;
            blowUpToTriangle(g, v, &newG);
            free(newG.adjacencyList);
            counter++;
        }
    }

    return counter;
}

//  Given a graph g, count and print all children, i.e. graphs obtained from g
//  by blowing a vertex up to a triangle.
long long unsigned int generateAllChildren( struct graph *g, 
 struct options *options) {

    long long unsigned int counter = 0;
    for(int v = 0; v < g->numberOfVertices; v++) {

        if(options->verboseFlag) {
            fprintf(stderr, "Blowing up %d\n", v);
        }
        struct graph newG;
        blowUpToTriangle(g, v, &newG);
        free(newG.adjacencyList);
        counter++;
    }

    return counter;
}

//  Counts the number of lonely edges in the graph.
int countLonelyEdges(struct graph *g, struct options *options,
 bitset *lonelyEdges) {

    bitset matching = EMPTY;
    bitset edgesHit = EMPTY;
    bitset edgesHitTwice = EMPTY;

    generatePerfectMatchings(g, options, complement(EMPTY, g->numberOfVertices),
     matching, &edgesHit, &edgesHitTwice);

    *lonelyEdges = difference(edgesHit, edgesHitTwice);

    return size(*lonelyEdges);
}

int main(int argc, char ** argv) {
    struct options options = {0};
    int opt;
    int output = -1;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = 
        {
            {"all", no_argument, NULL, 'a'},
            {"help", no_argument, NULL, 'h'},
            {"descendants", no_argument, NULL, 'd'},
            {"matchings", no_argument, NULL, 'm'},
            {"output", required_argument, NULL, 'o'},
            {"verbose", no_argument, NULL, 'v'}
        };

        opt = getopt_long(argc, argv, "adhmo:v", long_options, &option_index);
        if (opt == -1) break;
        switch(opt) {
            case 'a':
                options.allChildrenFlag = true;
                fprintf(stderr, "Warning: Children might be isomorphic.\n");
                fprintf(stderr, 
                 "\tAlso generating children without lonely edges.\n");
                break;
            case 'd':
                options.printChildrenWithSameNumber = true;
                fprintf(stderr, 
                    "Warning: -d is only intended for 3-connected cubic graphs.\n");
                fprintf(stderr, "\tChildren may be isomorphic to each other.\n");
            break;
            case 'h':
                fprintf(stderr, "%s\n", USAGE);
                fprintf(stderr, "%s", HELPTEXT);
                return 0;
            case 'm':
                options.printMatchingsFlag = true;
                break;
            case 'o':
                output = (int) strtol(optarg, (char **)NULL, 10);
                if(output < 0) {
                    fprintf(stderr,
                     "Error: number of lonely edges should at least 0.\n");
                    return 1;
                }
                break;
            case 'v':
                options.verboseFlag = true;
                break;
            case '?':
                fprintf(stderr,"Error: Unknown option: %c\n", optopt);
                fprintf(stderr, "%s\n", USAGE);
                fprintf(stderr,
                 "Use ./lonelyEdges --help for more detailed instructions.\n");
                return 1;
        }
    }
    if(options.printMatchingsFlag && !options.verboseFlag) {
        fprintf(stderr, "Error: use -m only with -v.\n");
        return 1;
    }
    if(output != -1 && options.printChildrenWithSameNumber) {
        fprintf(stderr, "Error: cannot use -o and -d simultaneously.\n");
        return 1;
    }
    if(output != -1 && options.allChildrenFlag) {
        fprintf(stderr, "Error: cannot use -o and -a simultaneously.\n");
        return 1;
    }
    if(options.printChildrenWithSameNumber && options.allChildrenFlag) {
        fprintf(stderr, "Error: cannot use -a and -d simultaneously.\n");
        return 1;
    }

    unsigned long long int counter = 0;
    unsigned long long int skippedGraphs = 0;
    unsigned long long int passedGraphs = 0;
    unsigned long long int frequencies[MAXBITSETSIZE] = {0};
    clock_t start = clock();

    //  Start looping over lines of stdin.
    char * graphString = NULL;
    size_t size;
    while(getline(&graphString, &size, stdin) != -1) {
        struct graph g;
        if(!readGraph(&g, &options, graphString)) {
            skippedGraphs++;
            continue;
        }

        if(options.verboseFlag) {
            fprintf(stderr, "\nLooking at: %s", graphString);
            printGraph(&g);
        }

        counter++;

        if(options.allChildrenFlag) {
            passedGraphs += generateAllChildren(&g, &options);
            freeGraph(&g);
            continue;
        }

        bitset lonelyEdges;
        int numberOfLonelyEdges = countLonelyEdges(&g, &options, &lonelyEdges);
        frequencies[numberOfLonelyEdges]++;

        //  With -d, we should only print children of the current graph who have
        //  the same number of lonely edges.
        if(options.printChildrenWithSameNumber) {
            passedGraphs += generateChildrenWithSameNumberOfLonelyEdges(
             &g, &options,lonelyEdges);
            freeGraph(&g);
            continue;
        }

        if(numberOfLonelyEdges) {
            if(output == -1 || output == numberOfLonelyEdges) {
                passedGraphs++;
                printf("%s",graphString);
            }
            if(options.verboseFlag) {
                fprintf(stderr, "%d lonely edges: ", numberOfLonelyEdges);
                forEach(e, lonelyEdges) {
                    fprintf(stderr, "(%d,%d) ",
                     g.labelToEdge[2*e], g.labelToEdge[2*e+1]);
                }
                fprintf(stderr, "\n");
            }
        }

        freeGraph(&g);
    }
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    if(!options.allChildrenFlag) {
        fprintf(stderr, "\n");
        for(int i = 0; i < MAXBITSETSIZE; i++) {
            if(frequencies[i]) {
                fprintf(stderr,
                 "\tInput graphs with %d lonely edges: %llu\n", i, frequencies[i]);
            }
        }
        fprintf(stderr, "\n");
    }


    fprintf(stderr,"\rChecked %lld graphs in %f seconds: %llu passed.\n",
     counter, time_spent, passedGraphs);
    if(skippedGraphs > 0) {
        fprintf(stderr, "Warning: %lld graphs were skipped.\n", skippedGraphs);
    }

    return 0;
}