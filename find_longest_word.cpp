#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

std::mutex mtx;  // Мьютекс для защиты глобального ресурса
std::string longestWordGlobal;  // Глобальная переменная для самого длинного слова

// Функция для нахождения самого длинного слова в буфере данных
void findLongestWordInBuffer(const std::string& buffer) {
    std::string currentWord;
    std::string longestWordLocal;

    for (char ch : buffer) {
        if (ch == ',' || ch == ' ' || ch == '\n' || ch == '\t') {
            // Проверяем текущее слово
            if (!currentWord.empty()) {
                if (currentWord.length() > longestWordLocal.length()) {
                    longestWordLocal = currentWord;
                }
                currentWord.clear();
            }
        }
        else {
            currentWord += ch;  // Добавляем символ к слову
        }
    }

    // Проверяем последнее слово в буфере
    if (!currentWord.empty() && currentWord.length() > longestWordLocal.length()) {
        longestWordLocal = currentWord;
    }

    // Защита глобальной переменной через мьютекс
    std::lock_guard<std::mutex> guard(mtx);
    if (longestWordLocal.length() > longestWordGlobal.length()) {
        longestWordGlobal = longestWordLocal;
    }
}

// Функция для чтения фрагмента файла и передачи его на обработку
void processFileChunk(const std::string& fileName, std::streampos start, std::streampos end) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Не удалось открыть файл: " << fileName << std::endl;
        return;
    }

    // Переходим к началу нужного куска
    file.seekg(start);

    // Читаем блок данных
    std::streamsize bufferSize = end - start;
    std::vector<char> buffer(bufferSize);

    file.read(buffer.data(), bufferSize);

    // Преобразуем в строку для обработки
    std::string chunk(buffer.begin(), buffer.end());

    findLongestWordInBuffer(chunk);
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

        // Запуск потоков для обработки кусков
        threads.push_back(std::thread(processFileChunk, fileName, start, end));
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
