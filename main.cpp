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
    vector<vector<long double> > classifier;
    while ( classifierFile >> readInput )
    {
        readInput.push_back(',');
        int last = 0;
        vector<long double> tempVector;
        for (int i = 0 ; i < readInput.length() ; i++)
            if ( readInput[i] == ',')
            {
                string first = readInput.substr(last,i - last);
                last = i + 1;
                tempVector.push_back(stold(first));
            }
        classifier.push_back(tempVector);
    }
    classifierFile.close();
    
    ifstream datasetFile;
    datasetFile.open(d1 + "/dataset.csv");
    vector<vector<long double> > dataset;
    datasetFile >> readInput ;
    while ( datasetFile >> readInput )
    {
        readInput.push_back(',');
        vector<long double> descriptor;
        int last = 0;
        for (int i = 0 ; i < readInput.length() ; i++)
            if ( readInput[i] == ',')
            {
                string first = readInput.substr(last,i - last);
                last = i + 1;
                descriptor.push_back(stold(first));
            }
        if ( descriptor.size() != 2 )
            cout << "dsize:" << descriptor.size() << endl ;
        dataset.push_back(descriptor);
    }
    datasetFile.close();
    vector<int> bestClassifierForDataset;
    for ( int i = 0 ; i < dataset.size() ; i++ )
    {
        long double length = dataset[i][0] , width = dataset[i][1] ;
        int label = -1;
        long double maxInnerProduct = -INF;
        for ( int j = 0 ; j < classifier.size() ; j++ )
        {
            long double b0 = classifier[j][0], b1 = classifier[j][1], bias = classifier[j][2];
            long double innerProduct = b0 * length + b1 * width + bias;
            if ( innerProduct > maxInnerProduct )
                maxInnerProduct = innerProduct , label = j;
        }
        bestClassifierForDataset.push_back(label);
    }
    
    string childToVoter = "/tmp/file_" + to_string(id) + ".txt";
    
    mkfifo(childToVoter.c_str(), 0666); 
    int namedPipe = open(childToVoter.c_str(), O_WRONLY) ;
    string FINALWRITTEN;
    for ( int i = 0 ; i < bestClassifierForDataset.size() ; i++ )
        FINALWRITTEN += to_string(bestClassifierForDataset[i]) + ",";
    
    write(namedPipe, FINALWRITTEN.c_str(), FINALWRITTEN.size());
    close(namedPipe);
    return;
}

void voter(int classifierCount)
{
    string passPID = "/tmp/PIDs";
    mkfifo(passPID.c_str(), 0666); 
    int getPID = open(passPID.c_str(), O_RDONLY);
    int readPIDCount = 0;
    vector<int> PIDs;
    while ( readPIDCount != classifierCount )
    {
        char temp;
        string saveBuf;
        while ( read(getPID, &temp , 1) )
        {
            if ( temp == ',' ) break;
            saveBuf.push_back(temp);
        }
        PIDs.push_back(stoi(saveBuf));
        readPIDCount++;
    }

    vector<int> namedPipeVec;
    for (int i = 0 ; i < classifierCount ; i++)
    {
        string childPipe = "/tmp/file_" + to_string(i) + ".txt";
        mkfifo(childPipe.c_str(), 0666);
        int namedPipe = open(childPipe.c_str(), O_RDONLY) ;
        namedPipeVec.push_back(namedPipe);
    }
    

    for ( int i = 0 ; i < PIDs.size() ; i++ )
        wait(&(PIDs[i]));
    
    vector< vector<int> > sample_classCount; 
    for ( int i = 0 ; i < classifierCount ; i++ )
    {
        char temp;
        string saveBuf;
        int namedPipe = namedPipeVec[i];
        int cntSample = 0;
        while ( read(namedPipe, &temp , 1) )
        {
            if ( temp == ',' )
            {
                while( sample_classCount.size() <= cntSample )
                {
                    vector<int> EMPTY_VEC;
                    sample_classCount.push_back(EMPTY_VEC);
                }
                int ans = stoi(saveBuf);
                while ( sample_classCount[cntSample].size() <= ans )
                    sample_classCount[cntSample].push_back(0);
                
                sample_classCount[cntSample][ans]++;
                saveBuf.clear();
                cntSample++;
                continue;
            }
            saveBuf.push_back(temp);
        }
    }

    vector<string> finalResult;
    for ( int i = 0 ; i < sample_classCount.size() ; i++ )
    {
        int mx = -1 , ID = -1;
        for ( int j = 0 ; j < sample_classCount[i].size() ; j++ )
            if ( mx < sample_classCount[i][j] )
                mx = sample_classCount[i][j], ID = j;
        finalResult.push_back(to_string(ID) + ",");
    }
    string resDir = "/tmp/results";
    mkfifo(resDir.c_str(), 0666);
    int sendRes = open(resDir.c_str(), O_WRONLY);

    for ( int i = 0 ; i < finalResult.size() ; i++ )
    {
        write(sendRes, finalResult[i].c_str(), finalResult[i].size());
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
        //cout << "child process starting:" << i << endl;
        childProcess(i,csvFile.size());
        //cout << "child process ending:" << i << endl;
        return 0;
    }
    
    
    
    int fork_res = fork();
    if ( fork_res == 0 )
    {
        voter(i);
        return 0 ;
    }

    string passPID = "/tmp/PIDs";
    mkfifo(passPID.c_str(), 0666); 
    int sendPID = open(passPID.c_str(), O_WRONLY);
    for ( int j = 0 ; j < i ; j++ )
    {
        string temp = to_string(childProcessPIDs[j]) + ",";
        write(sendPID, temp.c_str(), temp.size() );
    }
        
    string resDir = "/tmp/results";
    mkfifo(resDir.c_str(), 0666);
    int getRes = open(resDir.c_str(), O_RDONLY);
    wait(&fork_res);

    char temp;
    string saveBuf;
    vector<int> results;
    
    while ( read(getRes, &temp , 1) )
    {
        if ( temp == ',' )
        {   
            int ans = stoi(saveBuf);
            results.push_back(ans);
            saveBuf.clear();
            continue;
        }
        saveBuf.push_back(temp);
    }

    ifstream labelFile;
    labelFile.open(d1 + "/labels.csv");

    int trueRes = 0 ;
    int someInput;
    vector<int> labelAns;

    string tempString;
    labelFile >> tempString;
    labelFile >> tempString;
    while ( labelFile >> someInput )
        labelAns.push_back(someInput);
    
    

    for ( int i = 0 ; i < results.size() ; i++ )
    {
        if ( labelAns[i] == results[i] )
            trueRes++;
    }
    cout << "Accuracy:" << (long double)trueRes * 100 /results.size() << "%" << endl ;


	return 0;
}