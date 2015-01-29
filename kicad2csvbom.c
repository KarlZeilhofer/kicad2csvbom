/*
 ============================================================================
 Name        : net2csvbom.c
 Author      : Karl Zeilhofer
 Version     :
 Copyright   : www.zeilhofer.co.at, published under GPL3
 Description : Convert a KiCad netlist file to a BOM as a CSV
 ============================================================================
 */

// TODO: bugfix or workaround.
// 		"(ref" und auch "(lib" kommt auch in anderen gruppen außer export:components:comp vor.
// 		--> letzte componente erhält falsche einträge in ref.

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// parsing a Netlist file from KiCad, and extract a bill of material (BOM) as a csv

/* example of a netlist file:
(export (version D)
  (design
    (source /home/karl/DSD-Battery-Monitoring-System/KiCad/BatteryMonitoringSystem-V1.1/BatteryMonitoringSystem.sch)
    (date "Wed 14 Jan 2015 12:24:17 PM CET")
    (tool "Eeschema (2014-08-05 BZR 5054)-product"))
  (components
    (comp (ref F101)
      (value RXEF030)
      (footprint bat-mon-sys:Polyfuse-MC33173)
      (libsource (lib device) (part FUSE))
      (sheetpath (names /) (tstamps /))
      (tstamp 5420FC1D))
    (comp (ref P101)
      (value T6)
      (libsource (lib device) (part TST))
      (sheetpath (names /) (tstamps /))
      (tstamp 54B6E39C))
    (comp (ref P102)
      (value T7)
      ...
*/

#define CES 256 // allocated component element size
typedef struct component_
{
    char ref[CES];
    char value[CES];
    char footprint[CES];
    char lib[CES];
    char part[CES];
}Component;

#define MC 1000 // max components
Component components[MC];
int components_len = MC;


// zero terminate all component-element-strings:
void initComponents()
{
    for(int n=0; n<components_len; n++)
    {
	components[n].ref[0]=0;
	components[n].value[0]=0;
	components[n].footprint[0]=0;
	components[n].lib[0]=0;
	components[n].part[0]=0;
    }
}

/* example:
 * source = abcdefghi
 * strLeft = abc
 * strRight = ghi
 * --> dest = def
 *
 * when strLeft is not found, nothing will be coppied
 * when middle string starts with ", strRight will be searched from the second " on.
*/
void copyMiddleStr(char* source, char* dest, char* strLeft, char* strRight)
{
	int sizeL = strlen(strLeft);
	//int sizeR = strlen(strRight);
	long int pos0L = (long int) strstr(source, strLeft);

	if(pos0L)
	{
		pos0L-= (long int)source;
	}
	else
	{
		return;
	}

	int pos0M = pos0L + sizeL;
	long int pos0R = pos0M; // use pos0R as starting point for search of strRight
	if(source[pos0R] == '\"') // if mid-string starts with "
	{
		pos0R++;
		while(source[pos0R] != '\"')
		{
			pos0R++;
		}
		pos0R++;
	}


	pos0R = (long int) strstr(source+pos0R, strRight);
	if(pos0R)
	{
		pos0R-= (long int) source;
	}
	else
	{
		pos0R = strlen(source);
	}


	int sizeM;


	sizeM = pos0R - pos0M;

	if(pos0M >= strlen(source))
	{
		printf("Error, pos0M larger than strlen(source)\n");
	}else
	{
		// copy
		strncpy(dest, source+pos0M, sizeM);
		// terminate dest:
		dest[sizeM] = 0;
	}

}

bool contains(int* array, int len, int element)
{
	for(int i=0; i<len; i++)
	{
		if(array[i] == element)
			return true;
	}

	return false;
}


// program name: kicad2csvbom
int main(int argc, char** args)
{
	if(argc <= 1)
	{
		printf("  please give me a kicad .net file as inputfile.\n");
		printf("    e.g. net2csvbom MyPcb.net\n");
		printf("    or   net2csvbom --compressed MyPcb.net\n");
	}
	bool compressed = false;


	if(strcmp("--compressed", args[1])==0){
		compressed=true;
	}

	if(argc <= 2 && compressed)
	{
		printf("  please give me a kicad .net file as inputfile.\n");
		printf("    e.g. net2csvbom MyPcb.net\n");
		printf("    or   net2csvbom --compressed MyPcb.net\n");
		return 1;
	}

	char* infile;
	if(compressed)
		infile = args[2];
	else
		infile = args[1];


	FILE* in = fopen(infile, "r");

	initComponents();
	int ci=-1; // component index

	// parse file
	while(!feof(in))
	{
		char line[256];
		fgets(line, 256, in);
		//		printf("line %d\n", ++nLine);

		if(strstr(line, "(comp (ref "))
		{
			// start new component.
			ci++;

		}
		if(ci>=0 && ci<MC)
		{

			if(strlen(components[ci].ref)==0)
				copyMiddleStr(line, components[ci].ref, "(ref ", ")");
			if(strlen(components[ci].value)==0)
				copyMiddleStr(line, components[ci].value, "(value ", ")");
			if(strlen(components[ci].footprint)==0)
				copyMiddleStr(line, components[ci].footprint, "(footprint ", ")");
			if(strlen(components[ci].lib)==0)
				copyMiddleStr(line, components[ci].lib, "(lib ", ")");
			if(strlen(components[ci].part)==0)
				copyMiddleStr(line, components[ci].part, "(part ", ")");
		}
	}

	int N = ci+1; // number of extracted components


	if(compressed)
	{
		// print header:
		printf("%s\t%s\t%s\t%s\t%s\t%s\n", "Count", "Reference", "Value", "Footprint", "Library", "Part");

		int nSimilar=0; // number of similar components
		int iSimilars[MC]; // list of index of similar components

		int iUsed[MC]; // list of used component-indices
		int nUsed=0;


		int cc = 0; // current component index

		// collect all similar indices
		for(cc=0; cc<N; cc++)
		{
			if(contains(iUsed, nUsed, cc) == false) // do it only for items, which were not used already.
			{
				nSimilar = 0;

				// add first occurance
				iSimilars[nSimilar++] = cc;
				iUsed[nUsed++]=cc;

				for(int i=cc+1; i<N; i++)
				{
					if(contains(iUsed, nUsed, i) == false) // do it only for items, which were not used already.
					{
						if(strcmp(components[i].part, components[cc].part) == 0 &&
								strcmp(components[i].value, components[cc].value) == 0 &&
								strcmp(components[i].footprint, components[cc].footprint) == 0 )
						{
							iSimilars[nSimilar++] = i;
							iUsed[nUsed++]=i;
						}
					}
				}

				// print all similar references in one line
				printf("%d\t", nSimilar); // count

				for(int i=0; i<nSimilar; i++) // print all Refs
				{
					printf("%s ", components[iSimilars[i]].ref);
				}
				printf("\t");
				printf("%s\t%s\t%s\t%s\n", components[iSimilars[0]].value, components[iSimilars[0]].footprint, components[iSimilars[0]].lib, components[iSimilars[0]].part);

			}
		}
	}
	else
	{
		printf("%s\t%s\t%s\t%s\t%s\t%s\n", "ID", "Reference", "Value", "Footprint", "Library", "Part");

		for(int i=0; i<N; i++)
		{
			printf("%d\t%s\t%s\t%s\t%s\t%s\n", i, components[i].ref, components[i].value, components[i].footprint, components[i].lib, components[i].part);
		}
	}
}

