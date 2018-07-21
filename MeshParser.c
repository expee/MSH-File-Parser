/* 
 * MSH File parser
 *
 * Copyright (C) 2018 Experian Elitiawan <experian.elitiawan@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall 
 * be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
 * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


/* Curently This Parser only support :
 * - up to 8 Nodes Hexahedron
 * - up to 32 letters name
 * - up to 10 tags per element 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum elementTypes
{
    _2_NODE_LINE = 1,
    _3_NODE_TRIANGLE,
    _4_NODE_QUADRANGLE,
    _4_NODE_TETRAHEDRON,
    _8_NODE_HEXAHEDRON,
    _6_NODE_PRISM,
    _5_NODE_PYRAMID
};

typedef struct 
{
    int dimension;
    int physicalNumber;
    char name[32];
}PhysicalNameInfo;
PhysicalNameInfo* physicalNamesInfo = NULL;

typedef struct
{
    int number;
    float x;
    float y;
    float z;
}NodeInfo;
NodeInfo* nodesInfo = NULL;

typedef struct
{
    int number;
    int type;
    int tagCount;
    int tags[10];
    int nodes[8];
}ElementInfo;
ElementInfo* elementsInfo = NULL;

int versionCodeMacro = 0;
int versionCodeMicro = 0;
int fileType = 0;
int dataSize = 0;

int nodesCount = 0;
int elementsCount = 0;
int physicalNamesCount = 0;


const char* physicalNameKeyword = "$PhysicalNames";
const char* physicalNameEndKeyword = "$EndPhysicalNames";


int count_string_length(const char* str)
{
    int i;
    int length = 0;
    for(i = 0; str[i] != '\0'; i++)
    {
        length++;
    }
    return length;
}

// !!IMPORTANT!!
// This function must be called before exiting the program or before starting new file reading
// Otherwise memory leak will occur
void free_memory_()
{
    if(nodesInfo)
    {
        printf("Freeing nodesInfo...\n");
        free(nodesInfo);
        nodesInfo = NULL;
    }

    if(physicalNamesInfo)
    {
        printf("Freeing physicalNamesInfo...\n");
        free(physicalNamesInfo);
        physicalNamesInfo = NULL;
    }

    if(elementsInfo)
    {
        printf("Freeing elementsInfo...\n");
        free(elementsInfo);
        elementsInfo = NULL;
    }
}

void parse_input_file_(int* out_physicalNamesCount, int* out_nodesCount, int* out_elementsCount)
{
    //Arbitrary variables (for for-loops and such)
    int i, j, k;
    //Temporary string to store region name 
    char currentRegionName[32];
    int seekPos = 0;

    //Make sure that there's no memory leak
    free_memory_();
    FILE* file = fopen("./../Input/Curvature.msh", "r");
    if(file)
    {
        printf("START READING FILE...\n");
        //File format
        fscanf(file, "$MeshFormat\n");
        fscanf(file, "%d.%d %d %d\n", &versionCodeMacro, &versionCodeMicro, &fileType, &dataSize);
        printf("==============================================\n");
        printf("File version            : %d.%d\n", versionCodeMacro, versionCodeMicro);
        printf("File Type               : %s\n", fileType == 0 ? "ASCII MSH File" : "Binary MSH File");
        printf("Atomic Data Size        : %d byte(s)\n", dataSize);
        printf("==============================================\n");
        fscanf(file, "$EndMeshFormat\n");
        
        //Physical Names (This region may not exist, thus a check is required)
        seekPos = ftell(file);
        fscanf(file, "%s\n", currentRegionName);
        if(strcmp(currentRegionName, physicalNameKeyword) == 0)
        {
            fscanf(file, "%d\n", &physicalNamesCount);
            *out_physicalNamesCount = physicalNamesCount;
            printf("Physical Names Count    : %d\n", physicalNamesCount);
            physicalNamesInfo = (PhysicalNameInfo*) calloc(physicalNamesCount, sizeof(PhysicalNameInfo));
            for(i = 0; i < physicalNamesCount; i++)
            {
                fscanf(file, "%d %d %s\n", &(physicalNamesInfo[i].dimension), &(physicalNamesInfo[i].physicalNumber), physicalNamesInfo[i].name);
            }
            fscanf(file, "$EndPhysicalNames\n");
        }
        else
        {
            printf("Physical Names Count    : %d\n", 0);
            fseek(file, seekPos, SEEK_SET);
        }


        //Nodes
        fscanf(file, "$Nodes\n");
        fscanf(file, "%d\n", &nodesCount);
        *out_nodesCount = nodesCount;
        printf("Nodes Count             : %d\n", nodesCount);
        nodesInfo = (NodeInfo*) calloc(nodesCount, sizeof(NodeInfo));
        for(i = 0; i < nodesCount; i++)
        {
            fscanf(file, "%d %f %f %f\n", &(nodesInfo[i].number), &(nodesInfo[i].x),&(nodesInfo[i].y), &(nodesInfo[i].z)); 
        }
        fscanf(file, "$EndNodes\n");
        
        //Elements
        fscanf(file, "$Elements\n");
        fscanf(file, "%d\n", &elementsCount);
        *out_elementsCount = elementsCount;
        printf("Elements Count          : %d\n", elementsCount);
        elementsInfo = (ElementInfo*) calloc(elementsCount, sizeof(ElementInfo));
        for(i = 0; i < elementsCount; i++)
        {
            fscanf(file, "%d", &(elementsInfo[i].number));
            fscanf(file, "%d", &(elementsInfo[i].type));
            fscanf(file, "%d", &(elementsInfo[i].tagCount));
            for(j = 0; j < elementsInfo[i].tagCount; j++)
            {
                fscanf(file, "%d", &(elementsInfo[i].tags[j]));
            }
            switch(elementsInfo[i].type)
            {
                case _2_NODE_LINE:
                {
                    for(k = 0; k < 2; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _3_NODE_TRIANGLE:
                {
                    for(k = 0; k < 3; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _4_NODE_QUADRANGLE:
                {
                    for(k = 0; k < 4; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _4_NODE_TETRAHEDRON:
                {
                    for(k = 0; k < 4; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _5_NODE_PYRAMID:
                {
                    for(k = 0; k < 5; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _6_NODE_PRISM:
                {
                    for(k = 0; k < 6; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _8_NODE_HEXAHEDRON:
                {
                    for(k = 0; k < 8; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
            }
        }
        fscanf(file, "$EndElements\n");
        
        //End file operation
        fclose(file);

        printf("==============================================\n");
        printf("READ SUCCESS...\n");
    }
    else
    {
        printf("!!!ERROR OPENING FILE!!!\n");
        printf("Please check that the directory is correct and the file does exist\n");
    }
}

void retrieve_read_data_(PhysicalNameInfo* out_physicalNamesInfo, NodeInfo* out_nodesInfo, ElementInfo* out_elementsInfo)
{
    int i;
    for(i = 0; i < physicalNamesCount; i++)
    {
        out_physicalNamesInfo[i] = physicalNamesInfo[i];
    }
    for(i = 0; i < nodesCount; i++)
    {
        out_nodesInfo[i] = nodesInfo[i];
    }
    for(i = 0; i < elementsCount; i++)
    {
        out_elementsInfo[i] = elementsInfo[i];
    }
}

/* This section is not required unless you want to debug the code
int main (int argc, char** argv)
{
    parse_input_file_();
}
*/
