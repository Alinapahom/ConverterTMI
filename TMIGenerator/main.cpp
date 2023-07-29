#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <cerrno>
#include "tmiGenerator.hpp"

int main(int argc, char const *argv[])
{
    // Определение числа генерируемых пакетов
    uint8_t packCount = 1;         // Число генерируемых пакетов
    if (argc > 1)                  // Расшифровка параметров переданных с программой
    {                              //
        auto arg2 = atoi(argv[1]); // Преобразование аргумента переданного в программу в число пакетов
        if (arg2 > 255)            // Если переданное число больше 255
            packCount = 255;       // Генерация 255 пакетов
        else if (arg2 < 1)         // Если переданное число пакетов меньше 0 или неудалось распознать аргумент
            packCount = 1;         // Генерация одного пакета
        else                       // Если параметр программы соответсвует предыдущим условиям
            packCount = arg2;      // Генерация требуемого числа пакетов
    }                              //

    // Создание файла
    std::cout << "Program start: \r\n";                                                                // Вывод на консоль информации о созданном файле
    const auto currentTime = std::time(nullptr);                                                       // Перевод текущего времени в формат time_t
    std::stringstream fileName;                                                                        // Создание потока строки для имени файла
    fileName << "sskp_" << std::put_time(std::localtime(&currentTime), "%d_%m_%Y_%H_%M_%S") << ".bin"; // Генерирование имени файла
    std::ofstream initialTmiFile;                                                                      // Создание дескриптора сохраняемого файла
    initialTmiFile.open(fileName.str(), std::ios::out | std::ios::binary);                             // Создание файла
    if (initialTmiFile.is_open() == false)                                                             // Проверка открытия файла
    {                                                                                                  //
        std::cerr << "Could't create file\r\n";                                                        // Вывод ошибки программы в консоль
        return ENOFILE;                                                                                // Возврат из программы с кодом ошибки ENOFILE
    }                                                                                                  //

    // Генерация пакетов
    char *fileData;                                                 // Определение байтового массива
    try                                                             // Попытка выделения памяти для байтового массива
    {                                                               //
        fileData = new char[1024];                                  // Выделение памяти для байтового массива генерируемой структуры (1 КБ)
    }                                                               //
    catch (const std::bad_alloc &e)                                 // Если выделение памяти завершилось с ошибкой
    {                                                               //
        std::cerr << "Could't allocate memory for initial TMI\r\n"; // Вывод ошибки программы в консоль
        return -1;                                                  // Возврат из программы с кодом ошибки -1
    }                                                               //
    TMIGenerator generator;                                         // Инициализация генератора
    size_t fullByteCount = 0;                                       // Переменная полного размера файла
    for (uint8_t packNum = 0; packNum < packCount; ++packNum)       // Цикл генерации пакетов
    {                                                               //
        auto dataSize = generator.generateTMI(fileData, 1024);      // Генерирование структуры
        initialTmiFile.write(fileData, dataSize);                   // Запись данных в файл
        fullByteCount += dataSize;                                  // Добавление числа сгенерированных байт с общему числу байт
    }                                                               //

    // Завершение работы программы
    std::cout                                                     // Вывод на консоль информации о созданном файле
        << "Create file with name: " << fileName.str() << "\r\n"; //
    std::cout << "File size: " << fullByteCount << "\r\n";        // Вывод на консоль информации о размере файла
    initialTmiFile.close();                                       // Закрытие файла
    delete[] fileData;                                            // Освобождение памяти для генерируемой структуры;
    return 0;
}
