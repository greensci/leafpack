#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <bitset>
#include <string>
#include <algorithm>
#include <cstdint>
#include <array>
#include <iostream>
#include <iomanip>
#include <random>
using namespace std;

// CRC32 table
uint32_t crc32_table[256];

uint8_t* split32To8(uint32_t value)
{
    uint8_t* bytes = new uint8_t[4];
    bytes[0] = (value >> 24) & 0xFF; // Most significant byte (MSB)
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF; // Least significant byte (LSB)
    return bytes;
}

void generate_crc32_table()
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++)
        {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
        }
        crc32_table[i] = crc;
    }
}

uint32_t crc32(const std::string &text)
{
    uint32_t crc = 0xFFFFFFFF;
    for (char c : text)
    {
        crc = (crc >> 8) ^ crc32_table[(crc ^ c) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

int globalmode = 0; // if 0 = normal retail mode, if 1 = unpack mode, if 2 = pack mode
int generateRandom(int min, int max)
{
    // Initialize random number generator (done only once)
    static std::random_device rd;  // True random number generator
    static std::mt19937 gen(rd()); // Mersenne Twister engine

    // Define the distribution range [0, 253]
    static std::uniform_int_distribution<int> distrib(min, max);

    return distrib(gen);
}

unsigned char swapNibbles(int mode, unsigned char value)
{
    switch (mode)
    {
    case 0:                                 // Original: (value >> 4) | (value << 4)
        return (value << 4) | (value >> 4); // Self-inverse
    case 1:                                 // Original: (value >> 2) | (value << 6)
        return (value << 2) | (value >> 6); // Inverse of mode 1
    case 2:                                 // Original: (value >> 6) | (value << 2)
        return (value << 6) | (value >> 2); // Inverse of mode 2
    case 3:                                 // Original: (value >> 5) | (value << 3)
        return (value << 5) | (value >> 3); // Inverse of mode 3
    case 4:                                 // Original: (value >> 3) | (value << 5)
        return (value << 3) | (value >> 5); // Inverse of mode 4
    case 5:                                 // Original: (value >> 7) | (value << 1)
        return (value << 7) | (value >> 1); // Inverse of mode 5
    case 6:                                 // Original: (value >> 1) | (value << 7)
        return (value << 1) | (value >> 7); // Inverse of mode 6
    case 7:                                 // Original: same as mode 3
        return (value << 5) | (value >> 3); // Same inverse as mode 3
    default:
        throw out_of_range("Invalid mode value (must be 0-7).");
    }
}
unsigned char reswapNibbles(int mode, unsigned char value)
{
    switch (mode)
    {
    case 0:                                 // Original: (value >> 4) | (value << 4)
        return (value >> 4) | (value << 4); // Self-inverse
    case 1:                                 // Original: (value >> 2) | (value << 6)
        return (value >> 2) | (value << 6); // Inverse of mode 1
    case 2:                                 // Original: (value >> 6) | (value << 2)
        return (value >> 6) | (value << 2); // Inverse of mode 2
    case 3:                                 // Original: (value >> 5) | (value << 3)
        return (value >> 5) | (value << 3); // Inverse of mode 3
    case 4:                                 // Original: (value >> 3) | (value << 5)
        return (value >> 3) | (value << 5); // Inverse of mode 4
    case 5:                                 // Original: (value >> 7) | (value << 1)
        return (value >> 7) | (value << 1); // Inverse of mode 5
    case 6:                                 // Original: (value >> 1) | (value << 7)
        return (value >> 1) | (value << 7); // Inverse of mode 6
    case 7:                                 // Original: same as mode 3
        return (value >> 5) | (value << 3); // Same inverse as mode 3
    default:
        throw out_of_range("Invalid mode value (must be 0-7).");
    }
}

string byteToBinaryString(unsigned char b)
{
    bitset<8> bits(b);
    return bits.to_string();
}

vector<unsigned char> readFile(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file)
    {
        throw runtime_error("Could not open file");
    }

    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0, ios::beg);

    vector<unsigned char> data(fileSize);
    file.read(reinterpret_cast<char *>(data.data()), fileSize);

    return data;
}

void writeFile(const string &filename, const vector<unsigned char> &data)
{
    ofstream file(filename, ios::binary);
    if (!file)
    {
        throw runtime_error("Could not create file");
    }
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
}

string getOutputFilename(const string &inputFilename)
{
    size_t dotPos = inputFilename.find_last_of('.');
    string nameWithoutExt = inputFilename.substr(0, dotPos);
    string extension = (dotPos != string::npos) ? inputFilename.substr(dotPos) : "";
    return nameWithoutExt + "_packed.lpk";
}

int main(int argc, char *argv[])
{
    generate_crc32_table();
    if (argc < 2)
    {
        cerr << "Usage: leafpack <argument> <inputfile>" << endl;
        cerr << "Options:" << endl;

        cerr << "  -p : Pack the input file" << endl;
        cerr << "  -d : Unpack the input file" << endl;
        std::string hi;
        std::getline(std::cin, hi);

        return 1;
    }
    string appmode = "";

    if (globalmode == 1)
    {
        appmode = "(unpack mode)";
    }
    else if (globalmode == 2)
    {
        appmode = "(pack mode)";
    }

    if (std::string(argv[1]).find(".lpk") != std::string::npos)
    {
        globalmode = 1;
    }
    else if (std::string(argv[1]) == "-p")
    {
    }
    else if (std::string(argv[1]) == "-pp")
    {
    }
    else if (std::string(argv[1]) == "-d")
    {
    }
    else
    {
        globalmode = 2;
    }

    std::cout << "LeafPack V1 - by greensci " << appmode << endl;
    if (std::string(argv[1]) == "-p" || globalmode == 2)
    {
        std::cout << "Packing file..." << endl;

        try
        {

            std::string arg2;
            if (globalmode == 2)
                arg2 = argv[1];
            else
                arg2 = argv[2];

            size_t length = arg2.size() + 1; // Now you can use .size()

            vector<unsigned char> data = readFile(arg2);
            vector<unsigned char> packedData((data.size() + 6) + length);

            packedData[0] = 0x4C; // 'L'
            packedData[1] = 0x50; // 'P'
            packedData[2] = 0x4B; // 'K'
            packedData[3] = 0x31; // Version 1

            unsigned char bitter = generateRandom(0, 253); // Binary literal (C++14)
            packedData[4] = bitter;
            packedData[5] = length;
            packedData[6] = generateRandom(0x45, 254);

            string bitsBinKey = byteToBinaryString(bitter);

            // std::cout << "Key bits: " << bitsBinKey << endl;

            for (int i = 7; i < (length + 7) - 1; i += 8)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (i + x >= (7 + length))
                    {
                        continue; // Avoid out-of-bounds access
                    }

                    unsigned char byter = 0x00;
                    if (bitsBinKey[x] == '1')
                    {
                        // Apply the inverse operation for this mode
                        byter = swapNibbles(x, arg2[(i + x) - 7]);
                    }
                    else
                    {
                        // Mode 0 is its own inverse
                        byter = swapNibbles(0, arg2[(i + x) - 7]);
                    }

                    packedData[i + x] = byter;
                }
            }

            for (int i = 6 + length; i < (data.size() + 6) + length; i += 8)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (i + x >= data.size() + (6 + length))
                    {
                        continue; // Avoid out-of-bounds access
                    }

                    unsigned char byter = 0x00;
                    if (bitsBinKey[x] == '1')
                    {
                        // Apply the inverse operation for this mode
                        byter = swapNibbles(x, data[(i + x) - (6 + length)]);
                    }
                    else
                    { 
                        // Mode 0 is its own inverse
                        byter = swapNibbles(0, data[(i + x) - (6 + length)]);
                    }

                    packedData[i + x] = byter;
                }
            }

            // Save the packed data
            string outputFilename = getOutputFilename(arg2);
            writeFile(outputFilename, packedData);
            std::cout << "File packed successfully to " << outputFilename << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
    }
    else if (std::string(argv[1]) == "-pp" || globalmode == 2)
    {
        std::cout << "Packing file..." << endl;

        try
        {
            std::string password;
            std::cout << "Enter a password for the packed file: \n";
            std::getline(std::cin, password);
            uint32_t passchecksum = crc32(password);

            int passwordLength = 4;

            uint8_t* passcheckumbytes = split32To8(passchecksum);

            std::string arg2 = argv[2];

            size_t length = arg2.size() + 1; // Now you can use .size()

            vector<unsigned char> data = readFile(arg2);
            vector<unsigned char> packedData(((data.size() + 6) + length) + passwordLength);

            packedData[0] = 0x4C; // 'L'
            packedData[1] = 0x50; // 'P'
            packedData[2] = 0x4B; // 'K'
            packedData[3] = 0x31; // Version 1

            unsigned char bitter = generateRandom(0, 253); // Binary literal (C++14)
            packedData[4] = bitter;
            packedData[5] = length;
        
          
            int hahahah = generateRandom(0, 4);

            packedData[6] = passwordLength;

            if(hahahah == 0){
                packedData[6] = 0x11; // No password
                bitter = (int)passcheckumbytes[0]; // Use the last byte of data as the key
            }
            else if(hahahah == 1){
                packedData[6] = 0x25; // Password
                bitter = (int)passcheckumbytes[1]; // Use the second last byte of data as the key
            }
            else if(hahahah == 2){
                packedData[6] = 0x38; // Password
                bitter = (int)passcheckumbytes[2]; // Use the third last byte of data as the key
            }
            else{
                packedData[6] = 0x43; // Password
                bitter = (int)passcheckumbytes[3]; // Use the last byte of data as the key
            }

            string bitsBinKey = byteToBinaryString(bitter);

            int hihihi = 0;
            

            // std::cout << "Key bits: " << bitsBinKey << endl;
          


           
          
            for (int i = 7; i < (length + 7 + passwordLength) - 1; i += 8)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (i + x >= ((7 + length) + passwordLength))
                    {
                        continue; // Avoid out-of-bounds access
                    }

                    unsigned char byter = 0x00;
                    if (bitsBinKey[x] == '1')
                    {
                        // Apply the inverse operation for this mode
                        byter = swapNibbles(x, arg2[(i + x) - 7]);
                    }
                    else
                    {
                        // Mode 0 is its own inverse
                        byter = swapNibbles(0, arg2[(i + x) - 7]);
                    }

                    packedData[i + x] = byter;
                }
            }

            for (int i = 6 + length; i < (data.size() + 6) + length + passwordLength; i += 8)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (i + x >= data.size() + (6 + length + passwordLength))
                    {
                        continue; // Avoid out-of-bounds access
                    }

                    unsigned char byter = 0x00;
                    if (bitsBinKey[x] == '1')
                    {
                        // Apply the inverse operation for this mode
                        byter = swapNibbles(x, data[(i + x) - (6 + length)]);
                    }
                    else
                    {
                        // Mode 0 is its own inverse
                        byter = swapNibbles(0, data[(i + x) - (6 + length)]);
                    }

                    packedData[i + x] = byter;
                }
            }
            packedData[packedData.size() - 4] = passcheckumbytes[0];
            packedData[packedData.size() - 3] = passcheckumbytes[1];
            packedData[packedData.size() - 2] = passcheckumbytes[2];
            packedData[packedData.size() - 1] = passcheckumbytes[3];

            // Save the packed data
            string outputFilename = getOutputFilename(arg2);
            writeFile(outputFilename, packedData);
            std::cout << "File packed successfully to " << outputFilename << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
    }
    else if (std::string(argv[1]) == "-d" || globalmode == 1)
    {
        std::cout << "Unpacking file..." << endl;

        try
        {

            vector<unsigned char> data;
            if (globalmode == 1)
                data = readFile(argv[1]);
            else
                data = readFile(argv[2]);

            size_t length = data[5]; // Now you can use .size()

            vector<unsigned char> unpackedData(data.size() - (6 + length));

            // Use the same key byte position (4) as in the unpacker
          
unsigned char bitter = data[4]; // Binary literal (C++14)
            // std::cout << "Key bits: " << bitsBinKey << endl;
            string filename = "";

            if (data[6] <= 0x45)
            {
                std::string password;
                std::cout << "Enter a password for the packed file: \n";
                std::getline(std::cin, password);
              

             
                uint32_t passchecksum = crc32(password);

        
                uint8_t* passcheckumbytes = split32To8(passchecksum);


                std::cout << "Checksum: " << passchecksum << endl;

                if(data[6] == 0x11){
                    bitter = data[data.size() - 4];
                }
                else if(data[6] == 0x25){
                    bitter = data[data.size() - 3];
                }
                else if(data[6] == 0x38){
                    bitter = data[data.size() - 2];
                }
                else if(data[6] == 0x43){
                    bitter = data[data.size() - 1];
                }else{
                    std::cout << "Error: Invalid file" << endl;
                    return 1;
                }


                
               

               

                if (data[data.size() - 4] == passcheckumbytes[0] &&
                data[data.size() - 3] == passcheckumbytes[1] &&
                data[data.size() - 2] == passcheckumbytes[2] &&
                data[data.size() - 1] == passcheckumbytes[3])
                {
                    std::cout << "Password is correct, unpacking..." << endl;
                }
                else
                {
                    std::cout << "Password is incorrect, aborting unpacking." << endl;
                    return 1;
                }
            }
              

            string bitsBinKey = byteToBinaryString(bitter);

            for (int i = 7; i < (data[5] + 7) - 1; i += 8)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (i + x >= (7 + data[5]) - 1)
                    {
                        continue; // Avoid out-of-bounds access
                    }

                    unsigned char byter = 0x00;
                    if (bitsBinKey[x] == '1')
                    {
                        // Apply the inverse operation for this mode
                        byter = reswapNibbles(x, data[i + x]);
                    }
                    else
                    {
                        // Mode 0 is its own inverse
                        byter = reswapNibbles(0, data[i + x]);
                    }

                    filename += byter;
                }
            }
            std::cout << "Filename: " << filename << endl;

            for (int i = 6 + length; i < data.size() - 4; i += 8)
            {
                for (int x = 0; x < 8; x++)
                {
                    if (i + x >= data.size() - 4)
                    {
                        continue; // Avoid out-of-bounds access
                    }

                    unsigned char byter = 0x00;
                    if (bitsBinKey[x] == '1')
                    {
                        // Apply the inverse operation for this mode
                        byter = reswapNibbles(x, data[(i + x)]);
                    }
                    else
                    {
                        // Mode 0 is its own inverse
                        byter = reswapNibbles(0, data[(i + x)]);
                    }

                    unpackedData[(i + x) - (6 + length)] = byter;
                }
            }
            

            // Save the packed data

            writeFile(filename, unpackedData);
            std::cout << "File unpacked successfully to " << filename << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
    }
    else
    {
        cerr << "Usage: leafpack <argument> <inputfile>" << endl;
        cerr << "Options:" << endl;

        cerr << "  -p : Pack the input file" << endl;
        cerr << "  -d : Unpack the input file" << endl;
        std::string hi;
        std::getline(std::cin, hi);
        return 1;
    }

    return 0;
}