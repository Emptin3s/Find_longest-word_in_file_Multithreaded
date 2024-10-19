#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

std::mutex mtx;  // Mutex to protect shared resources (e.g., longest word)
std::string longestWordGlobal;  // Store the longest word globally

// Function to find the longest word in a file chunk
void findLongestWordInChunk(std::ifstream& file, std::streampos start, std::streampos end) {
    file.seekg(start);
    std::string word, longestWordLocal;

    while (file.tellg() < end && file >> word) {
        if (word.length() > longestWordLocal.length()) {
            longestWordLocal = word;
        }
    }

    // Protect the global longest word with a mutex
    std::lock_guard<std::mutex> guard(mtx);
    if (longestWordLocal.length() > longestWordGlobal.length()) {
        longestWordGlobal = longestWordLocal;
    }
}

// Function to split the file into chunks and create threads
void processFileInChunks(const std::string& fileName, int numThreads) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);  // Open file at the end
    if (!file) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return;
    }

    std::streampos fileSize = file.tellg();  // Get file size
    std::streampos chunkSize = fileSize / numThreads;

    std::vector<std::thread> threads;

    // Create threads to process each chunk
    for (int i = 0; i < numThreads; ++i) {
        std::streampos start = i * chunkSize;
        std::streampos end = (i == numThreads - 1) ? fileSize : start + chunkSize;
        threads.push_back(std::thread(findLongestWordInChunk, std::ref(file), start, end));
    }

    // Wait for all threads to finish
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "The longest word in the file is: " << longestWordGlobal << std::endl;
}

int main() {
    std::string fileName = "file.txt";  // Replace with your file name
    int numThreads = 8;  // Number of threads

    processFileInChunks(fileName, numThreads);

    return 0;
}