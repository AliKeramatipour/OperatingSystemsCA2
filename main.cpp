#include <iostream>
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 

using namespace std;

string d1 = "Assets/validation";
string d2 = "Assets/weight_vectors";

int getNumOfClassifiers()
{
	int res = 0 ;
	while (1)
	{
        string csvFile = d2 + "/classifier_" + to_string(res) + ".csv";
        int fd = open(csvFile.c_str(),O_RDONLY);
        if ( fd == -1 ) break;
        close(fd);
        res++;
	}
	return res ;
}

int main(int argc, char *argv[]) {
	
	int numOfClassifiers = getNumOfClassifiers();

    for (int i = 0; i < numOfClassifiers ; i++)
    {
        string csvFile = d2 + "/classifier_" + to_string(res) + ".csv";
        int fd = open(csvFile.c_str(),O_RDONLY);
        
    }
	
	
	return 0;
}