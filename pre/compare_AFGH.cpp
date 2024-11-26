#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>

using namespace std;

// void ReadFile(string path, vector<double> &array)
// {
//     ifstream file(path, ios::in);
//     if (!file)
//     {
//         cout << path + " error" << endl;
//         exit(EXIT_FAILURE);
//     }
    
//     int testing = 0;
//     string line = {};
//     while(getline(file, line))
//     {
//         stringstream ss(line);
//         string tmp = {};
//         while(getline(ss, tmp, ','))
//         {
//             array.push_back(stod(tmp));
//         }
//     }
//     file.close();
// }

// int main()
// {
//     cout.precision(7);
//     for (int round = 1; round <= 3; round++) // round 3 epsn 10 1 differ
//     {
//         for (int epsn = 10; epsn <= 18; epsn++)
//         {
//             string dataset = "HeartScale";
//             int size = 270;
//             int noises_size = pow(size, 2);
//             int client_amount = 3;
//             int precision = 1;
//             int counter = 0;
//             double diff_total = .0;
//             string dataset_path = "./Dataset/" + dataset + "/" + dataset + "_round" + to_string(round);
//             vector<double> afgh = {}, orig = {};
//             ReadFile(dataset_path+"/"+dataset+"_Epsn"+to_string(epsn)+"_round"+to_string(round)+"_SUM.csv", afgh);
//             ReadFile(dataset_path+"/"+dataset+"_Epsn"+to_string(epsn)+"_round"+to_string(round)+"_SUM_ORIGIN.csv", orig);

//             bool flag = true;
//             for (int i = 0; i < noises_size; i++)
//             {
//                 if (afgh[i] != orig[i])
//                 {
//                     flag = false;
//                     cout << i << '\t' << i/size+1 << '/' << i%size+1 << '\t' << afgh[i] << " " << orig[i] << endl;
//                     diff_total += abs(afgh[i]-orig[i]);
//                     counter += 1;
//                 }
//             }
//             cout << "round:" << round << "\tepsn:" << epsn << endl;
//             if (flag)
//                 cout << "two files are the same" << endl << endl;
//             else
//             {
//                 cout << "total differ : " << diff_total << endl;
//                 cout << counter << " of differences" << endl;
//             }
//         }
//     }

//     return 0;
// }

vector<double> ReadFile(string path)
{
    ifstream input(path, ios::in);
    if(!input)
    {
        cout << "Unable to open input file!" << endl;
        exit(EXIT_FAILURE);
    }
    vector<double> result = {};
    string str;
    while(!input.eof())
    {
        input >> str;
        result.push_back(stod(str));
    }
    input.close();
    return result;
}

int main()
{
    string dataset_path = "./clients_noises/";
    vector<double> afgh = ReadFile(dataset_path + "noises_sum.txt");
    vector<double> orig = ReadFile(dataset_path + "original_noises_sum.txt");
    int noises_size = afgh.size();
    int counter = 0;
    double diff_total = .0;

    bool flag = true;
    for (int i = 0; i < noises_size; i++)
    {
        if (afgh[i] != orig[i])
        {
            flag = false;
            cout << i << '\t' << afgh[i] << " " << orig[i] << endl;
            diff_total += abs(afgh[i]-orig[i]);
            counter += 1;
        }
    }
    if (flag)
        cout << "two files are the same" << endl << endl;
    else
    {
        cout << "total differ : " << diff_total << endl;
        cout << counter << " of differences" << endl;
    }
}