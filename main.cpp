#include <iostream>
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <fstream>
#include <vector>

using namespace std;

typedef pair<float,pair<float,float> > floatTriplePair ;

const int MAX_pipes = 100;
const int MAX_filename = 100;
const int INF = 1e8;

string d1 = "Assets/validation";
string d2 = "Assets/weight_vectors";
int fd[MAX_pipes][2];

void childProcess(int id, int readByteCount)
{
    char csvFile[MAX_filename] ;
    read(fd[id][0], csvFile, readByteCount);
    ifstream classifierFile;
    classifierFile.open(csvFile);

    string readInput;
    classifierFile >> readInput;
    vector<vector<float> > classifier;
    while ( classifierFile >> readInput )
    {
        readInput.push_back(',');
        int last = 0;
        vector<float> tempVector;
        for (int i = 0 ; i < readInput.length() ; i++)
            if ( readInput[i] == ',')
            {
                string first = readInput.substr(last,i - last);
                last = i + 1;
                tempVector.push_back(stof(first));
            }
        classifier.push_back(tempVector);
    }
    classifierFile.close();
    
    ifstream datasetFile;
    datasetFile.open(d1 + "/dataset.csv");
    vector<vector<float> > dataset;
    datasetFile >> readInput ;
    while ( datasetFile >> readInput )
    {
        readInput.push_back(',');
        vector<float> descriptor;
        int last = 0;
        for (int i = 0 ; i < readInput.length() ; i++)
            if ( readInput[i] == ',')
            {
                string first = readInput.substr(last,i - last);
                last = i + 1;
                descriptor.push_back(stof(first));
            }
        dataset.push_back(descriptor);
    }
    datasetFile.close();
    vector<int> bestClassifierForDataset;

    cout << classifier.size() << endl ;
    for ( int i = 0 ; i < dataset.size() ; i++ )
    {
        int length = dataset[i][0] , width = dataset[i][1] ;
        int label = -1;
        float maxInnerProduct = -INF;
        for ( int j = 0 ; j < classifier.size() ; j++ )
        {
            float b0 = classifier[j][0], b1 = classifier[j][1], bias = classifier[j][2];
            float innerProduct = b0 * length + b1 * width + bias;
            if ( innerProduct > maxInnerProduct )
                maxInnerProduct = innerProduct , label = j;
        }
        bestClassifierForDataset.push_back(label);
    }
    
    string childToVoter = "/tmp/file_" + to_string(id) + ".txt";
    char STRINGTEST[30] ; 
    STRINGTEST[childToVoter.size()] = 0;
    for ( int i = 0 ; i < childToVoter.size() ; i++ )
    {
        STRINGTEST[i] = childToVoter[i];
    }
    int retVAL = mkfifo(STRINGTEST, 0666); 
    cout << "RET:" << retVAL << endl;
    cout << "WTFFFF1" << childToVoter << endl ;
    int namedPipe = open(STRINGTEST,O_WRONLY) ;
    cout << "WTFFFF2" << endl ;
    for ( int i = 0 ; i < bestClassifierForDataset.size() ; i++ )
    {
        string temp = to_string(bestClassifierForDataset[i]) + "";
        write(namedPipe, temp.c_str(), temp.size());
    }
    close(namedPipe);
    cout << "WTFFFF " << endl ;
    return;
}

void voter(int classifierCount)
{
    cout << " here voter " << endl ;
    for (int i = 0 ; i < classifierCount ; i++)
    {
        string childDataDir = "/tmp/" + to_string(i);
        int namedPipe = open(childDataDir.c_str(), O_RDONLY) ;
        char data;
        vector<int> votes;
        string temp;
        while (read(namedPipe, &data, 1)){
            if ( data == '\n' )
            {
                reverse(temp.begin(), temp.end() );
                votes.push_back(stoi(temp));
                cout << temp << endl;
                continue;
            }
            temp.push_back(data);
        }
        close(namedPipe);
    }
}


int main(int argc, char *argv[]) {

    bool isChildProcess = 0;
    string csvFile;
    int i = 0;
    vector<int> childProcessPIDs;
    for (;;i++)
    {
        csvFile = d2 + "/classifier_" + to_string(i) + ".csv";
        int csvFd = open(csvFile.c_str(), O_RDONLY);
        if ( csvFd != -1 )
        {
            close(csvFd);
            int pid = fork();
            childProcessPIDs.push_back(pid);
            if ( pid < 0 )
            {
                cout << " fork error\n";
                return 0 ;
            }
            pipe(fd[i]);
            write(fd[i][1], csvFile.c_str(), csvFile.size()); 
            if ( pid == 0){
                isChildProcess = 1;
                break;
            }
        } 
        else 
            break;
    }
    if ( isChildProcess ){
        childProcess(i,csvFile.size());
        cout << "child process ending:" << i << endl;
        return 0;
    }
    
    for ( int i = 0 ; i < childProcessPIDs.size() ; i++ )
        wait(&(childProcessPIDs[i]));
    
    int fork_res = fork();
    if ( fork_res == 0 )
    {
        voter(i);
    }
	return 0;
}