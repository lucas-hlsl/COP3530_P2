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
int renderCycles; // number of cycles since last render

void Merge(std::vector<int>& data, int ctin, int stin, int mdin, int enin) {
    std::vector<int> temp(enin-stin+1);
    int ldin = stin, rdin = mdin+1, tin = 0; //left subarray data index, right subarray data index, temp array index
    while (ldin <= mdin && rdin <= enin) {
        if (Data.less(ctin, data[ldin], data[rdin])) {
            temp[tin] = data[ldin];
            ++ldin;
        } else {
            temp[tin] = data[rdin];
            ++rdin;
        }
        ++tin;
    }
    while (ldin <= mdin) {              // put remaining left sub array elements in temp
        temp[tin++] = data[ldin++];
    }
    while (rdin <= enin) {              // put remaining right sub array elements in temp
        temp[tin++] = data[rdin++];
    }
    for (int i = 0; i < enin-stin+1; ++i) {
        data[stin+i] = temp[i];
        if (temp[i]%1000 == 999) {
            m_r_map[temp[i]/1000 + 1] = stin+i;
            renderCycles++;
            if (renderCycles >= 10) Render::__instance->draw(&m_r_map, &q_r_map, &Data);
            renderCycles %= 10;
        }
        //check temp[i][0] is in the id_index
        //use that int to update the m_r_map to tin + stin
        //call an update
    }
}
void MergeSort(std::vector<int>& data, int ctin, int stin, int enin) { //data, category index, start indsex, end index
    if (stin < enin) {
        int mdin = (stin + enin)/2; // middle index
        MergeSort(data, ctin, stin, mdin);
        MergeSort(data, ctin, (mdin+1), enin);
        Merge(data, ctin, stin, mdin, enin);
    }
}
int Pivot(std::vector<int>& data,int ctin, int stin, int enin) {
    std::vector<int> temp1(enin-stin+1);
    std::vector<int> temp2(enin-stin);
    int pivin = 0, t2in = 0;
    for (int i = 1; i < enin-stin+1; ++i) {
        if (Data.less(ctin, data[(stin+i)], data[stin])) {
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
        if (temp1[i]%1000 == 999) {
            q_r_map[temp1[i]/1000 + 1] = stin + i;
            renderCycles++;
            if (renderCycles >= 10) Render::__instance->draw(&m_r_map, &q_r_map, &Data);
            renderCycles %= 10;
        }
        //check temp1[i][0] is in the id_index
        //use that int to update the q_r_map to stin + 1
        //call an update
    }

    for (int i = 0; i < t2in; ++i) {
        data[stin+pivin+i+1] = temp2[i];
        if (temp2[i]%1000 == 999) {
            q_r_map[temp2[i]/1000 + 1] = stin+pivin+i+1;
            renderCycles++;
            if (renderCycles >= 10) Render::__instance->draw(&m_r_map, &q_r_map, &Data);
            renderCycles %= 10;
        }
        //check temp2[i][0] is in the id_index
        //use that int to update the q_r_map to stin+pivin+i+1
        //call an update
    }
    return stin+pivin;
}
void QuickSort(std::vector<int>& data, int ctin, int stin, int enin) {
    if (stin < enin) {
        int pin = Pivot(data, ctin, stin, enin);
        QuickSort(data, ctin, stin, pin-1);
        QuickSort(data, ctin, (pin+1), enin);
    }
}

void MergeSort(int col) {
    std::vector<int> data = Data.identityOrder();
    for (int i = 1; i < data.size()/1000; ++i) {
        m_r_map[i] = i * 1000 - 1;
    }
    MergeSort(data, col, 0, data.size() - 1);
}

void QuickSort(int col) {
    std::vector<int> data = Data.identityOrder();
    for (int i = 1; i < data.size()/1000; ++i) {
        q_r_map[i] = i * 1000 - 1;
    }
    QuickSort(data, col, 0, data.size() - 1);
}

int main(int argc, char* argv[]) {
    if (!glfwInit()) return -1;

    Data.load("datasets/supermarket_large_dataset.csv");

    Render render(1280, 720, "Supermarket Sort");   // Starts the main process loop
    while (render.draw(&m_r_map, &q_r_map, &Data));
    glfwTerminate();
    return 0;
}