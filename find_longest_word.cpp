#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <algorithm>

std::mutex mtx;  // Мьютекс для защиты глобального ресурса
std::string longestWordGlobal;  // Глобальная переменная для самого длинного слова

// Функция для нахождения самого длинного слова в части файла
void findLongestWordInChunk(std::ifstream& file, std::streampos start, std::streampos end) {
    file.seekg(start);
    std::string line, longestWordLocal;
    char ch;

    // Буфер для хранения текущего слова
    std::string currentWord;

    while (file.tellg() < end && file.get(ch)) {
        // Если нашли запятую или пробел — проверяем текущее слово
        if (ch == ',' || ch == ' ' || ch == '\n') {
            if (!currentWord.empty()) {
                if (currentWord.length() > longestWordLocal.length()) {
                    longestWordLocal = currentWord;
                }
                currentWord.clear();  // Очищаем для нового слова
            }
        }
        else {
            currentWord += ch;  // Добавляем символ к слову
        }
    }

    // Проверяем последнее слово после завершения цикла
    if (currentWord.length() > longestWordLocal.length()) {
        longestWordLocal = currentWord;
    }

    // Защита глобальной переменной через мьютекс
    std::lock_guard<std::mutex> guard(mtx);
    if (longestWordLocal.length() > longestWordGlobal.length()) {
        longestWordGlobal = longestWordLocal;
    }
}

// Функция для разделения файла на части и создания потоков
void processFileInChunks(const std::string& fileName, int numThreads) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);  // Открытие файла в бинарном режиме
    if (!file) {
        std::cerr << "Не удалось открыть файл: " << fileName << std::endl;
        return;
    }

    std::streampos fileSize = file.tellg();  // Получаем размер файла
    std::streampos chunkSize = fileSize / numThreads;  // Размер каждой части

    std::vector<std::thread> threads;

    // Создаем потоки для обработки каждого куска
    for (int i = 0; i < numThreads; ++i) {
        std::streampos start = i * chunkSize;
        std::streampos end = (i == numThreads - 1) ? fileSize : start + chunkSize;
        threads.push_back(std::thread(findLongestWordInChunk, std::ref(file), start, end));
    }

    // Ожидаем завершения всех потоков
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Самое длинное слово в файле: " << longestWordGlobal << std::endl;
}

int main() {
    std::string fileName = "file.txt";  // Укажите имя вашего файла
    int numThreads = 8;  // Количество потоков

    processFileInChunks(fileName, numThreads);

    return 0;
}