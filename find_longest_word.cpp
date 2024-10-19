#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

std::mutex mtx;  // Мьютекс для защиты глобальных ресурсов
std::string longestWordGlobal;  // Глобальная переменная для самого длинного слова
unsigned long long totalWordCount = 0;  // Глобальная переменная для общего количества слов

// Функция для нахождения самого длинного слова и подсчета количества слов в буфере данных
void findLongestWordInBuffer(const std::string& buffer, unsigned long long& localWordCount) {
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
                ++localWordCount;  // Увеличиваем локальный счётчик слов
            }
        }
        else {
            currentWord += ch;  // Добавляем символ к слову
        }
    }

    // Проверяем последнее слово в буфере
    if (!currentWord.empty()) {
        if (currentWord.length() > longestWordLocal.length()) {
            longestWordLocal = currentWord;
        }
        ++localWordCount;  // Учитываем последнее слово
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

    // Локальный счётчик слов для данного потока
    unsigned long long localWordCount = 0;
    findLongestWordInBuffer(chunk, localWordCount);

    // Обновляем глобальный счётчик слов с использованием мьютекса
    std::lock_guard<std::mutex> guard(mtx);
    totalWordCount += localWordCount;
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
    std::cout << "Общее количество слов в файле: " << totalWordCount << std::endl;
}

int main() {
    setlocale(LC_ALL, "RUS");
    std::string fileName = "warandpeace.txt";  // Укажите имя вашего файла
    int numThreads = 8;  // Количество потоков

    processFileInChunks(fileName, numThreads);

    return 0;
}
