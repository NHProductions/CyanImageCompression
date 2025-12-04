#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <cstddef>
// just adds a ImgMetadata class that contains width, height, & bits per pixel.
#include "classes.h"
using namespace std;
int GridX = 64;
int GridY = 56;
int Bits = 9;
string PATH = "mountain.bmp";
uint32_t JoinByteVector(vector<unsigned char> V) {
    // ex: {AA,5B,C9,D1};
    int P = V.size(); // 4
    uint32_t output = 0ULL;
    for (unsigned char c : V) {
        output |= (uint32_t)c << ((P-1)*8); // shift it by P bytes
        P-=1;
    }
    return output;
};
uint32_t JoinByteVectorLE(const std::vector<unsigned char>& V) {
    uint32_t output = 0;
    for (size_t i = 0; i < V.size(); i++) {
        output |= (uint32_t)V[i] << (8 * i);
    }
    return output;
}
int TakeAvg(vector<int>& I) {
    double sum = 0;
    for (int v : I) sum += v;
    return static_cast<int>(round(sum / I.size()));
}
int ConvertBPP(uint8_t num, uint8_t OG, uint8_t Target) {
    // ex: 245; 8;
    // first: 193 218 246
    // 6 6 7
    double OGmax = pow(2,OG)-1;
    double TMax = pow(2,Target)-1;
    double proportion = (num/OGmax);
    double ret = TMax*proportion;
    return ceil(ret);
}
std::vector<unsigned char> IntToBytesBE(uint32_t value) {
    std::vector<unsigned char> bytes(4);
    bytes[0] = (value >> 24) & 0xFF;  // highest byte
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;          // lowest byte
    return bytes;
}
uint64_t ConvertBPP2(vector<int> V,int OG, int Target) {
    vector<uint64_t> R;
    for (int i=0;i<V.size();i++) {
        R.push_back(ConvertBPP(V[i], OG, Target));
    }
    uint64_t j=0;
    j |= R[0];
    j |= R[1] << 20;
    j |= R[2] << 40;
    return j;
}
uint32_t joinints(int R, int G, int B) {
    uint32_t A = 0;
    A |= (R << 16);
    A |= (G << 8);
    A |= (B);
    return A;
}
std::string intToBinary(int n) {
    if (n == 0) {
        return "0";
    }
    std::string binaryString = "";
    while (n > 0) {
        binaryString += (n % 2 == 0 ? '0' : '1'); // Append '0' or '1' based on LSB
        n /= 2; // Right shift (integer division by 2)
    }
    std::reverse(binaryString.begin(), binaryString.end()); // Reverse to get correct order
    if (binaryString.size() < 3) {
        while (binaryString.size() < 3) {
            binaryString = "0" + binaryString;
        }
    }
    return binaryString;
}
int main() {


    std::cout << "GH's Lossy Image Compression" << std::endl;
    string ToReturn = "";
    vector<vector<int>> Pixels;
    std::ifstream file(PATH,std::ios::binary);
    if (!file) {
        std::cout << "Failed to open file";
        return 1;
    };
    // unimportant header stuff, just moves point to byte 15 so we can get actual important stuff
    file.seekg(18,std::ios::cur); // skips unimportant header stuff
    ImgMetadata M;
    
    vector<unsigned char> ImgWidth(4);
    file.read(reinterpret_cast<char*>(ImgWidth.data()), ImgWidth.size());
    M.Width = JoinByteVectorLE(ImgWidth);
    vector<unsigned char> ImgHeight(4);
    file.read(reinterpret_cast<char*>(ImgHeight.data()), ImgHeight.size());
    M.Height = JoinByteVectorLE(ImgHeight);
    file.seekg(2, std::ios::cur); // skips size 
    vector<unsigned char> ImgBPP(2);
    file.read(reinterpret_cast<char*>(ImgBPP.data()), ImgBPP.size() );
    M.BPP = JoinByteVectorLE(ImgBPP);
    std::cout << "img is " << M.Width << " x " << M.Height;
    uint32_t Offset = 0;
    file.seekg(10,std::ios::beg);
    file.read(reinterpret_cast<char*>(&Offset), 4);

    file.seekg(Offset, std::ios::beg);
    std::cout << "Has Metadata; getting pixel colors" << std::endl;

    // Each row in a BMP is padded to a multiple of 4 bytes
    int rowPadding = (4 - (M.Width * 3) % 4) % 4;

   vector<int> Red(M.Width * M.Height);
vector<int> Greens(M.Width * M.Height);
vector<int> Blues(M.Width * M.Height);

for (int y = 0; y < M.Height; y++) {
    for (int x = 0; x < M.Width; x++) {
        unsigned char Colors[3];
        file.read(reinterpret_cast<char*>(Colors), 3);
        int index = x + (M.Height - 1 - y) * M.Width;
        Blues[index] = Colors[0];
        Greens[index] = Colors[1];
        Red[index] = Colors[2];
    }
    file.ignore(rowPadding);
}
cout << "BPP:" << M.BPP;
cout << "Pixel(0,0): R=" << Red[0] << " G=" << Greens[0] << " B=" << Blues[0] << "\n";
    std::cout << "Has pixel colors; getting grid colors" << std::endl;
    pair<int,int> Grid = {M.Width/GridX, M.Height/GridY}; // this is how big the grid spaces are
    vector<vector<int>> GridColors;
    for (int i=0;i<GridY;i++) {
        for (int j=0;j<GridX;j++) {
            pair<int,int> GridCorner = {j*Grid.first,i*Grid.second};
            vector<int> Grid_Reds;
            vector<int> Grid_Blues;
            vector<int> Grid_Greens;
            for (int gy=0;gy<Grid.second;gy++) {
                for (int gx=0;gx<Grid.first;gx++) {
                    int Index = (GridCorner.first + gx) + ((M.Height - 1 - (GridCorner.second + gy)) * M.Width);
                    Grid_Reds.push_back(Red[Index]);
                    Grid_Blues.push_back(Blues[Index]);
                    Grid_Greens.push_back(Greens[Index]);
                }
            }
            GridColors.push_back({TakeAvg(Grid_Reds), TakeAvg(Grid_Greens), TakeAvg(Grid_Blues)});


        }
    }
    std::cout << "Avg of first: " << setw(11/4) << std::setfill('0') << GridColors[0][0] << setw(11/4) << std::setfill('0') << GridColors[0][1] << setw(11/4) << std::setfill('0') << GridColors[0][2] ; 
    std::cout << "Has grid colors, starting printing" << std::endl;
    // so after that, we should have GridColors be an array containing the average colors of the gridspaces
    for (int in = GridY-1; in >= 0; in--) {
        for (int j = 0; j < GridX; j++) {
        int i = (in*GridX)+j;
        vector<int> Colors = {ConvertBPP(GridColors[i][0],8,Bits/3), ConvertBPP(GridColors[i][1],8,Bits/3), ConvertBPP(GridColors[i][2],8,Bits/3)};
        cout << intToBinary(Colors[0]) << "" << intToBinary(Colors[1]) << "" << intToBinary(Colors[2]);
}
}





    return 0;
}











