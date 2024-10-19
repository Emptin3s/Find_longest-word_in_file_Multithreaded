#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <chrono>

// Мьютекс для синхронизации потоков при доступе к общим ресурсам
std::mutex mtx;

// Глобальная переменная для хранения самого длинного слова
std::string longestWordGlobal;

// Глобальная переменная для общего количества слов во всём файле
unsigned long long totalWordCount = 0;

// Глобальная хеш-таблица для хранения частоты слов
std::unordered_map<std::string, unsigned long long> wordFrequencyGlobal;

// Функция для нахождения самого длинного слова и подсчета частоты слов в буфере данных
void findLongestWordAndCountFrequency(const std::string& buffer, unsigned long long& localWordCount, std::unordered_map<std::string, unsigned long long>& localWordFrequency) {
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

                // Увеличиваем частоту появления этого слова в локальной хеш-таблице
                ++localWordFrequency[currentWord];

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

        // Увеличиваем частоту последнего слова
        ++localWordFrequency[currentWord];

        ++localWordCount;  // Учитываем последнее слово
    }

    // Обновляем глобальные переменные с использованием мьютекса
    std::lock_guard<std::mutex> guard(mtx);
    if (longestWordLocal.length() > longestWordGlobal.length()) {
        longestWordGlobal = longestWordLocal;
    }

    // Обновляем глобальную хеш-таблицу частот
    for (const auto& pair : localWordFrequency) {
        wordFrequencyGlobal[pair.first] += pair.second;
    }
}

// Функция для чтения фрагмента файла и передачи его на обработку
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

    // Локальная хеш-таблица частот для данного потока
    std::unordered_map<std::string, unsigned long long> localWordFrequency;

    // Ищем самое длинное слово и считаем количество слов в данном фрагменте
    findLongestWordAndCountFrequency(chunk, localWordCount, localWordFrequency);

    // Обновляем глобальный счётчик слов с использованием мьютекса
    std::lock_guard<std::mutex> guard(mtx);
    totalWordCount += localWordCount;
}

// Функция для разделения файла на части и создания потоков
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
    // Вывод частоты слов
    std::cout << "\nЧастота слов в файле:" << std::endl;
    for (const auto& pair : wordFrequencyGlobal) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    // Выводим результаты: самое длинное слово и общее количество слов
    std::cout << "Самое длинное слово в файле: " << longestWordGlobal << std::endl;
    std::cout << "Общее количество слов в файле: " << totalWordCount << std::endl;
}

int main() {
    setlocale(LC_ALL, "rus");
    std::string fileName = "warandpeace.txt";  // Укажите имя вашего файла
    int numThreads = 8;  // Количество потоков для многопоточной обработки

    // Замеряем время начала выполнения программы
    auto startTime = std::chrono::high_resolution_clock::now();

    // Запуск обработки файла с заданным количеством потоков
    processFileInChunks(fileName, numThreads);

    // Замеряем время окончания выполнения программы
    auto endTime = std::chrono::high_resolution_clock::now();

    // Вычисляем продолжительность работы программы
    std::chrono::duration<double> duration = endTime - startTime;

    // Выводим продолжительность работы программы
    std::cout << "Время работы программы: " << duration.count() << " секунд" << std::endl;

    return 0;
}
