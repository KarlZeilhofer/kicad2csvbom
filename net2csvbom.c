/*
 ============================================================================
 Name        : net2csvbom.c
 Author      : Karl Zeilhofer
 Version     :
 Copyright   : www.zeilhofer.co.at, published under GPL3
 Description : Convert a KiCad netlist file to a BOM as a CSV
 ============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// parsing a Netlist file from KiCad, and extract a bill of material (BOM) as a csv

/* example:
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
      (libsource
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

Component components[1000];
int components_len = 1000;


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

// program name: kicadnet2csv
int main(int argc, char** args)
{
    if(argc <= 1)
    {
	printf("please give me a kicad .net file as inputfile.\n");
	return 1;
    }
    char* infile = args[1];
    FILE* in = fopen(infile, "r");

    initComponents();
    int ci=-1; // component index
    int nLine=0;

    // parse file
    while(!feof(in))
    {
		char line[256];
		fgets(line, 256, in);
//		printf("line %d\n", ++nLine);

		if(strstr(line, "(comp "))
		{
			// start new component.
			ci++;
		}
		if(ci>=0 && ci<1000)
		{
			copyMiddleStr(line, components[ci].ref, "(ref ", ")");
			copyMiddleStr(line, components[ci].value, "(value ", ")");
			copyMiddleStr(line, components[ci].footprint, "(footprint ", ")");
			copyMiddleStr(line, components[ci].lib, "(lib ", ")");
			copyMiddleStr(line, components[ci].part, "(part ", ")");
		}
    }

//    printf("found %d components:\n", ci);
    printf("%s\t%s\t%s\t%s\t%s\t%s\n", "id", "ref", "value", "footprint", "lib", "part");

    for(int i=0; i<ci; i++)
    {
    	printf("%d\t%s\t%s\t%s\t%s\t%s\n", i, components[i].ref, components[i].value, components[i].footprint, components[i].lib, components[i].part);
    }
}
