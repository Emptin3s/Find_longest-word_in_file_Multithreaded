#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

// Мьютекс для синхронизации потоков при доступе к общим ресурсам
std::mutex mtx;

// Глобальная переменная для хранения самого длинного слова
std::string longestWordGlobal;

// Глобальная переменная для общего количества слов во всём файле
unsigned long long totalWordCount = 0;

// Функция для нахождения самого длинного слова и подсчета количества слов в буфере данных
// buffer - строка, содержащая фрагмент данных из файла
// localWordCount - локальный счётчик слов для данного потока
void findLongestWordInBuffer(const std::string& buffer, unsigned long long& localWordCount) {
    std::string currentWord;  // Переменная для хранения текущего слова
    std::string longestWordLocal;  // Переменная для самого длинного слова в данном потоке

    // Проходим по каждому символу в буфере
    for (char ch : buffer) {
        // Если текущий символ - разделитель (запятая, пробел, новая строка, табуляция)
        if (ch == ',' || ch == ' ' || ch == '\n' || ch == '\t') {
            // Проверяем текущее слово (если оно не пустое)
            if (!currentWord.empty()) {
                // Если текущее слово длиннее найденного ранее, обновляем самое длинное слово
                if (currentWord.length() > longestWordLocal.length()) {
                    longestWordLocal = currentWord;
                }
                currentWord.clear();  // Очищаем текущее слово для начала нового
                ++localWordCount;  // Увеличиваем локальный счётчик слов
            }
        }
        else {
            // Если символ не разделитель, добавляем его в текущее слово
            currentWord += ch;
        }
    }

    // Проверяем последнее слово в буфере
    if (!currentWord.empty()) {
        if (currentWord.length() > longestWordLocal.length()) {
            longestWordLocal = currentWord;
        }
        ++localWordCount;  // Учитываем последнее слово
    }

    // Обновляем глобальную переменную с самым длинным словом, используя мьютекс
    std::lock_guard<std::mutex> guard(mtx);
    if (longestWordLocal.length() > longestWordGlobal.length()) {
        longestWordGlobal = longestWordLocal;
    }
}

// Функция для чтения фрагмента файла и передачи его на обработку
// fileName - имя файла
// start - начало фрагмента в файле
// end - конец фрагмента в файле
void processFileChunk(const std::string& fileName, std::streampos start, std::streampos end) {
    std::ifstream file(fileName, std::ios::binary);  // Открываем файл в бинарном режиме
    if (!file) {
        std::cerr << "Не удалось открыть файл: " << fileName << std::endl;
        return;
    }

    // Переходим к нужной позиции в файле (начало фрагмента)
    file.seekg(start);

    // Читаем данные в буфер
    std::streamsize bufferSize = end - start;  // Размер куска данных
    std::vector<char> buffer(bufferSize);  // Создаём буфер для хранения данных

    file.read(buffer.data(), bufferSize);  // Читаем данные из файла в буфер

    // Преобразуем буфер данных в строку для обработки
    std::string chunk(buffer.begin(), buffer.end());

    // Локальный счётчик слов для данного потока
    unsigned long long localWordCount = 0;

    // Ищем самое длинное слово и считаем количество слов в данном фрагменте
    findLongestWordInBuffer(chunk, localWordCount);

    // Обновляем глобальный счётчик слов с использованием мьютекса
    std::lock_guard<std::mutex> guard(mtx);
    totalWordCount += localWordCount;
}

// Функция для разделения файла на части и создания потоков
// fileName - имя файла
// numThreads - количество потоков для обработки
void processFileInChunks(const std::string& fileName, int numThreads) {
    // Открываем файл в бинарном режиме и перемещаем указатель в конец файла
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Не удалось открыть файл: " << fileName << std::endl;
        return;
    }

    // Получаем размер файла (указывает позицию в конце файла)
    std::streampos fileSize = file.tellg();

    // Размер куска файла, который будет обрабатываться каждым потоком
    std::streampos chunkSize = fileSize / numThreads;

    // Вектор для хранения потоков
    std::vector<std::thread> threads;

    // Создаём потоки для обработки каждого куска файла
    for (int i = 0; i < numThreads; ++i) {
        std::streampos start = i * chunkSize;  // Начало куска
        std::streampos end = (i == numThreads - 1) ? fileSize : start + chunkSize;  // Конец куска

        // Запускаем поток для обработки данного куска файла
        threads.push_back(std::thread(processFileChunk, fileName, start, end));
    }

    // Ожидаем завершения всех потоков
    for (auto& th : threads) {
        th.join();
    }

    // Выводим результаты: самое длинное слово и общее количество слов
    std::cout << "Самое длинное слово в файле: " << longestWordGlobal << std::endl;
    std::cout << "Общее количество слов в файле: " << totalWordCount << std::endl;
}

int main() {
    setlocale(LC_ALL, "RUS");
    std::string fileName = "warandpeace.txt";  // Укажите имя вашего файла
    int numThreads = 8;  // Количество потоков для многопоточной обработки

    // Запуск обработки файла с заданным количеством потоков
    processFileInChunks(fileName, numThreads);

    return 0;
}