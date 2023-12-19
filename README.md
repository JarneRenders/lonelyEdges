# FindDominationNumber 
This repository contains a program created for the article "J. Goedgebeur, D. Mattiolo, G. Mazzuoccolo, J. Renders, I. H. Wolf. Non-double covered cubic graphs, manuscript."

The program uses McKay's graph6 format to read and write graphs. See <http://users.cecs.anu.edu.au/~bdm/data/formats.txt>. 

### Short manual
This program can be used to determine whether or not bridgeless cubic graphs are non-double covered. It is also able to determine which edges in a given graph are lonely. Moreover, the program can output all children of the input graphs, these are graphs obtained from the input graph where one vertex is replaced by a triangle. Or it can output only those children with precisely the same number of lonely edges as the parent.

This program by default supports cubic graphs with less than 42 vertices or with less than 85 vertices if using the 128 bit versions.

### Installation

This requires a working shell and `make`. Navigate to the folder containing lonelyEdges.c and compile using: 

* `make` to create a binary for the 64-bit version;
* `make 128bit` to create a binary for the 128-bit version;
* `make 128bitarray` to create a binary for an alternative 128-bit version;
* `make all` to create all the above binaries.

The 64-bit version supports graphs only up to 42 vertices, the 128-bit versions up to 85 vertices. For graphs containing up to 42 vertices the 64-bit version performs significantly faster than the 128-bit versions. Use `make clean` to remove all binaries created in this way.

### Usage of lonelyEdges 

All options can be found by executing `./lonelyEdges -h`.

Usage: `./lonelyEdges [-o#|-d|-a] [-vmh]`

Program for finding the lonely edges of a graph.

Input graphs should be in graph6 format. Without any parameters, the program outputs those graphs which contain at least one lonely edge.

```
  -o# : Only output those graphs with exactly # lonely edges; Not compatible
        with option -a or -d
  -d  : For every graph in the input, output all of its children which have
        the same number of lonely edges; The children might be isomorphic as
        abstract graphs; Not compatible with option -a or -o#
  -a  : Output all children of the input graphString; The children might be
        isomorphic as abstract graphs; Not compatible with option -d or -o#
  -v  : Output extra information, such as the labeling of each graph and which
        lonely edges it has
  -m  : Output all perfect matchings of each graph; Best to combine with -v
  -h  : Print this helptext
```

### Examples

`./lonelyEdges < file.g6`
Output those graphs of `file.g6` which are non-double covered, i.e. contain at least one lonely edge. `file.g6` should be a file in the same directory as lonelyEdges containing the graph6 code of cubic graphs.

`./lonelyEdges -o3 < file.g6`
Output those graphs of `file.g6` which contain precisely three lonely edges.

`./lonelyEdges -d < file.g6`
Output children of the graphs in `file.g6`, i.e., graphs where one vertex is blown up to a triangle, which have the same number of lonely edges as their parent. Note that they might (and very likely will) contain isomorphic copies.
 
`./lonelyEdges -a < file.g6`
Output all children of the graphs in `file.g6`. Note that they might (and very likely will) contain isomorphic copies.
