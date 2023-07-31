#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <ctime>
#include "tmiConverter.hpp"

int main(int argc, char const *argv[])
{
    // Проверка переданных аргументов программы
    if (argc < 2)
    {
        std::cout << "Few arguments: Use " << argv[0] << " {initial TMI filename}\r\n";
        return -2;
    }
    else if (argc > 2)
    {
        std::cout << "Too many arguments. Use " << argv[0] << " {initial TMI filename}\r\n";
        return -2;
    }

    // Проверка имени файла на наличие даты
    auto dateString = strchr(argv[1], '_') + 1;
    std::string initialTMIFileName = argv[1];
    if (dateString == nullptr)
    {
        std::cerr << "Invalid file name: " << initialTMIFileName << ". "
                  << "Need filename like: sskp_%dd_%mm_%yyyy_%hh_%mm_%ss.bin" << std::endl;
        return -1;
    }

    // Открытие входного файла
    std::ifstream initialTMIFile;                                                             // Создание дескриптора входного файла
    initialTMIFile.open(initialTMIFileName, std::ios::binary | std::ios::in | std::ios::ate); // Открытие файла исходной структуры
    if (initialTMIFile.is_open() == false)                                                    // Проверка открытия
    {                                                                                         //
        std::cerr << "Can't open file " << initialTMIFileName << std::endl;                   // Вывод ошибки в консоль
        return -1;                                                                            // Возврат ошибки из программы
    }                                                                                         //
    size_t initialTMIFileSize = initialTMIFile.tellg();                                       // Извлечение размера файла
    initialTMIFile.seekg(0);                                                                  // Перенос указателя файла в начало
    if (initialTMIFileSize < sizeof(InitialTMI))                                              // Проверка на минимальный размер структуры
    {                                                                                         //
        std::cerr << "Invalid file size " << initialTMIFileSize << " B"                       // Вывод ошибки в консоль
                  << "Minimum size is " << sizeof(InitialTMI) << " B" << std::endl;           //
        return -1;                                                                            // Возврат ошибки из программы
    }                                                                                         //
    std::cout << "Initial TMI file name: " << initialTMIFileName << std::endl                 // Вывод в консоль информации об открытом файле
              << "File size of initial TMI: " << initialTMIFileSize << std::endl;             //

    // Переконвертация даты из имени файла
    std::istringstream initialTMIDate(dateString);
    std::tm initialTMITime = {};
    initialTMIDate >> std::get_time(&initialTMITime, "%d_%m_%Y_%H_%M_%S");
    auto targetTMIDate = std::put_time(&initialTMITime, "%Y-%m-%d_%H-%M-%S");

    // Создание папки и проверка наличия конечного файла
    std::error_code fsError;
    if (std::filesystem::create_directory("tmi", fsError) == false)
    {
        std::cerr << "Can't create directory tmi" << std::endl;
        fsError.clear();
        initialTMIFile.close();
        return -1;
    }
    fsError.clear();

    // Генерация имени выходного файла
    std::ostringstream targetTMIName, targetTMIPath;
    targetTMIName << targetTMIDate << "_UDP_0";
    targetTMIPath << "tmi/" << targetTMIName.str() << ".sspp";
    int currentFileNum = 0;
    while (std::filesystem::exists(targetTMIPath.str()))
    {
        ++currentFileNum;
        targetTMIPath.seekp(0);
        targetTMIPath << "tmi/" << targetTMIName.str() << "_" << currentFileNum << ".sspp";
    }

    // Создание выходного файла
    std::ofstream targetTMIFile;
    auto targetTMIFileName = targetTMIPath.str();
    targetTMIFile.open(targetTMIFileName, std::ios::binary | std::ios::out);
    if (targetTMIFile.is_open() == false)
    {
        std::cerr << "Can't create file " << targetTMIPath.str() << std::endl;
        initialTMIFile.close();
        return -1;
    }

    // Инициализация байтовых массивов
    char *initialTMIBuffer, *targetTMIBuffer;
    try
    {
        initialTMIBuffer = new char[initialTMIFileSize];
        targetTMIBuffer = new char[1024];
    }
    catch (std::bad_alloc)
    {
        std::cerr << "Can't allocate memory for buffers" << std::endl;
        initialTMIFile.close();
        targetTMIFile.close();
        return -1;
    }

    // Конвертация структур
    initialTMIFile.read(initialTMIBuffer, initialTMIFileSize);       // Копирование исходной структуры в байтовый массив
    size_t targetTMIFileSize = 0;                                    // Смещение целевого буфера
    size_t initialTMIBufferShift = 0;                                // Смещение исходного буфера
    TMIConverter converter;                                          // Инициализация конвертера
    size_t headerSize = converter.addFileHeaderTMI(targetTMIBuffer); // Созданение заголовка в байтовом массиве
    targetTMIFile.write(targetTMIBuffer, headerSize);                // Запись заголовка файла
    targetTMIFileSize += headerSize;                                 // Добавление размера заголовка
    while (1)                                                        // Цикл конвертации файлов
    {
        auto currentInitialTMIBufAddress = initialTMIBuffer + initialTMIBufferShift;
        auto currentInitialTMIBufSize = initialTMIFileSize - initialTMIBufferShift;
        std::cout << "Initial struct address: " << (void *)currentInitialTMIBufAddress << std::endl
                  << "Initial struct shift: " << initialTMIBufferShift << std::endl;

        size_t nextShift = converter.nextInitialTMIPos( // Поиск следующей структуры в файле
            currentInitialTMIBufAddress,                // Адрес исходной структуры со смещением
            currentInitialTMIBufSize);                  // Оставшийся размер исходной струтуры
        size_t dataSize = converter.convertTMI(         // Конвертация структур
            targetTMIBuffer,                            // Буфер целевой структуры
            currentInitialTMIBufAddress,                // Адрес исходной структуры со смещением
            currentInitialTMIBufSize);                  // Оставшийся размер исходной струтуры
        targetTMIFile.write(targetTMIBuffer, dataSize); // Запись целевой структуры в выходной файл
        targetTMIFileSize += dataSize;                  // Увеличение переменной размера файла

        std::cout << "Write " << dataSize << " bytes of target struct" << std::endl;
        if (nextShift)                          // Если есть следующая структура во входном пакете
            initialTMIBufferShift += nextShift; // Смещение входного буфера для следующей структуры
        else                                    // Если нет следующего пакета
            break;                              // Выход из цикла по окончанию входного пакета
    }

    // Завершение работы программы
    delete[] initialTMIBuffer;
    delete[] targetTMIBuffer;
    std::cout
        << "Create file: " << targetTMIFileName << std::endl
        << "File size: " << targetTMIFileSize << std::endl;
    targetTMIFile.close();
    initialTMIFile.close();
    return 0;
}
