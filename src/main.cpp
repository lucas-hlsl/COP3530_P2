#include "Render.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <GLFW/glfw3.h>

#include "Dataset.h"

Dataset Data;
std::unordered_map<int,int> m_r_map, q_r_map;
//[key : every thousandth transaction id, value: location of current index]
//will be used for rendering purposes only relevant thing for rendering
std::vector<int>  m_data = Data.identifyOrder(), q_data = m_data; //each int represents a transaction row

void Merge(vector<int>& data, int ctin, int stin, int mdin, int enin) {
    vector<int> temp(enin-stin+1);
    int ldin = stin, rdin = mdin+1, tin = 0; //left subarray data index, right subarray data index, temp array index
    while (ldin <= mdin && rdin <= enin) {
        if (Data.less(ctin, ldin, rdin)) {
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
        if (temp[i]%1000 == 999 && temp[i]/1000 > 0) {
            m_r_map[temp[i]/1000] = stin+i;
            //rendering update needs to be called here
        }
        //check temp[i][0] is in the id_index
        //use that int to update the m_r_map to tin + stin
        //call an update
    }
}
void MergeSort(vector<int>& data, int ctin, int stin, int enin) { //data, category index, start indsex, end index
    if (!(stin >= enin)) {
        int mdin = (stin + enin)/2; // middle index
        MergeSort(data, ctin, stin, mdin);
        MergeSort(data, ctin, (mdin+1), enin);
        Merge(data, ctin, stin, mdin, enin);
    }
}
int Pivot(vector<int>& data,int ctin, int stin, int enin) {
    vector<int> temp1(enin-stin+1);
    vector<int> temp2(enin-stin);
    int pivin = 0, t2in = 0;
    for (int i = 1; i < enin-stin+1; ++i) {
        if (Data.less(ctin, (stin+i), stin)) {
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
        if (temp1[i]%1000 == 999 && temp1[i]/1000 > 0) {
            q_r_map[temp1[i]/1000] = stin + i;
            //rendering update needs to be called here
        }
        //check temp1[i][0] is in the id_index
        //use that int to update the q_r_map to stin + 1
        //call an update
    }

    for (int i = 0; i < t2in; ++i) {
        data[stin+pivin+i+1] = temp2[i];
        if (temp2[i]%1000 == 999 && temp2[i]/1000 > 0) {
            q_r_map[temp2[i]/1000] = stin+pivin+i+1;
            //rendering update needs to be called here
        }
        //check temp2[i][0] is in the id_index
        //use that int to update the q_r_map to stin+pivin+i+1
        //call an update
    }
    return stin+pivin;
}
void QuickSort(vector<int>& data, int ctin, int stin, int enin) {
    if (!(stin >= enin)) {
        int pin = Pivot(data, ctin, stin, enin);
        QuickSort(data, ctin, stin, pin-1);
        QuickSort(data, ctin, (pin+1), enin);
    }
}

int main(int argc, char* argv[]) {
    Data.load();
    m_data = Data.identifyOrder();
    q_data = m_data;
    for (int i = 1; i < m_data.size()/1000; ++i) {
        m_r_map[i] = m_data[i*1000-1];
        q_r_map[i] = q_data[i*1000-1];
    }
    if (!glfwInit()) return -1;
    Render render(1280, 720, "Supermarket Sort");   // Starts the main process loop
    return 0;
}