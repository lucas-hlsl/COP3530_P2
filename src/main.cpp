#include "Render.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <GLFW/glfw3.h>

unordered_map<string,int> id_index; //[transaction id, int index]
unordered_map<int,int> m_r_map, q_r_map;
//[key : every thousandth transaction id, value: location of current index]
//will be used for rendering purposes only relevant thing for rendering
unordered_map<string,int> data_names; //[what the data is, index]
vector<vector<string>*>  m_data, q_data; //whole_dataset, each containing the transaction data.

void Merge(vector<vector<string>*>& data, int ctin, int stin, int mdin, int enin) {
    vector<vector<string>*> temp(enin-stin+1);
    int ldin = stin, rdin = mdin+1, tin = 0; //left subarray data index, right subarray data index, temp array index

    while (ldin <= mdin && rdin <= enin) {
        if (data[ldin]->at(ctin) <= data[rdin]->at(ctin) ) {
            temp[tin] = data[ldin];
            ++ldin;
        } else {
            temp[tin] = data[rdin];
            ++rdin;
        }
        ++tin;
    }
    for (int i = 0; i < (mdin - ldin) + (enin - rdin) + 2; ++i) {
        if ((mdin - ldin) == -1) {
            temp[tin] = data[rdin + i];
        } else if ((enin - rdin) == -1) {
            temp[tin] = data[ldin + i];
        }
        ++tin;
    }
    //^above deals with whats left after the while loop
    for (int i = 0; i < enin-stin+1; ++i) {
        data[stin+i] = temp[i];
        auto it = id_index.find(temp[i]->at(0));
        if (it != id_index.end()) {
            m_r_map[it->second] = stin+i;
            //rendering update needs to be called here
        }
        //check temp[i][0] is in the id_index
        //use that int to update the m_r_map to tin + stin
        //call an update
    }
}
void MergeSort(vector<vector<string>*>& data, int ctin, int stin, int enin) { //data, category index, start indsex, end index
    if (!(stin >= enin)) {
        int mdin = (stin + enin)/2; // middle index
        MergeSort(data, ctin, stin, mdin);
        MergeSort(data, ctin, (mdin+1), enin);
        Merge(data, ctin, stin, mdin, enin);
    }
}
int Pivot(vector<vector<string>*>& data,int ctin, int stin, int enin) {
    vector<vector<string>*> temp1(enin-stin+1);
    vector<vector<string>*> temp2(enin-stin);
    int pivin = 0, t2in = 0;
    for (int i = 1; i < enin-stin+1; ++i) {
        if (data[stin+i]->at(ctin) <= data[stin]->at(ctin)) {
            temp1[pivin] = data[stin+i];
            ++pivin;
        } else {
            temp2[t2in] = data[stin+i];
            ++t2in;
        }
    }
    temp1[pivin] = data[stin];
    for (int i = 0; i <= pivin; ++i) {
        data[stin+i] = temp1[i];
        auto it = id_index.find(temp1[i]->at(0));
        if (it != id_index.end()) {
            q_r_map[it->second] = stin + i;
            //rendering update needs to be called here
        }
        //check temp1[i][0] is in the id_index
        //use that int to update the q_r_map to stin + 1
        //call an update

    }

    for (int i = 0; i < t2in; ++i) {
        data[stin+pivin+i+1] = temp2[i];
        auto it = id_index.find(temp2[i]->at(0));
        if (it != id_index.end()) {
            q_r_map[it->second] = stin+pivin+i+1;
            //rendering update needs to be called here
        }
        //check temp2[i][0] is in the id_index
        //use that int to update the q_r_map to stin+pivin+i+1
        //call an update

    }
    return stin+pivin;
}
void QuickSort(vector<vector<string>*>& data, int ctin, int stin, int enin) {
    if (!(stin >= enin)) {
        int pin = Pivot(data, ctin, stin, enin);
        QuickSort(data, ctin, stin, pin-1);
        QuickSort(data, ctin, (pin+1), enin);
    }
}

int main(int argc, char* argv[]) {
    if (!glfwInit()) return -1;
    Render render(1280, 720, "Supermarket Sort");   // Starts the main process loop
    return 0;
}