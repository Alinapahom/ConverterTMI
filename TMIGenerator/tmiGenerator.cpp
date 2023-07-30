#include "tmiGenerator.hpp"
#include <cstdint>
#include <ctime>
#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <netdb.h>
#endif
#define TMI_MARKER 0xA7BE1C55
#define NET_TIME_TO_UNIX_TIME_SEC 62135596800
#define HUNDRED_NS_IN_SEC 10000000

struct UDPHeader
{
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;
    uint16_t dataLen = 0;
    uint16_t checkSum = 0;
};

#pragma pack(1)
struct InitialTMI
{
    uint32_t marker = 0;
    uint32_t srcIP = 0;
    uint32_t dstIP = 0;
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;
    uint32_t dataLen = 0;
    uint32_t counter = 0;
    uint64_t timeStamp = 0;
    uint32_t flags = 0;
    uint8_t minSizeData[sizeof(UDPHeader)] = {};
    uint32_t CRCCode = 0;
};

TMIGenerator::TMIGenerator()
{
    std::srand(std::time(nullptr) << 8); // Установка сида для генератора случайных чисел
    innerCounter = rand() % 100;         // Установка случайного числа начального счетчика пакетов
}

size_t TMIGenerator::generateTMI(void *byteBuffer, size_t bufferSize)
{
    constexpr auto tmiStructLenWithoutData = sizeof(InitialTMI) - sizeof(InitialTMI::minSizeData);

    if (bufferSize < sizeof(InitialTMI)) // Проверка на малый размер буфера для генерируемой структуры
        return 0;                        // Возврат ошибки

    auto tmiStruct = (InitialTMI *)byteBuffer;  // Интепретация в виде генерируемой структуры
    tmiStruct->marker = TMI_MARKER;             // Копирование маркера пакета
    tmiStruct->srcIP = getMyIP();               // Копирование IP адреса компьютера к качестве источника
    tmiStruct->dstIP = (rand() << 16) + rand(); // Генерирование случайного IP адрес назначения
    uint16_t srcPort = rand();                  // Случайный порт источника
    uint16_t dstPort = rand();                  // Случайный порт приемника
    tmiStruct->srcPort = srcPort;               // Копирование порта источника
    tmiStruct->dstPort = dstPort;               // Копирование порта приемника
    tmiStruct->counter = innerCounter++;        // Копирование номера пакета с инкрементированием
    tmiStruct->timeStamp = getCurrentNETTime(); // Получение текущего времени
    tmiStruct->flags = 0;                       // Отсутствие флагов

    size_t maxDataSize = bufferSize -                            // Максимальное число генерируемых байт UDP пакета
                         tmiStructLenWithoutData -               // Размер генерируемой структуры без блока данных
                         sizeof(UDPHeader);                      // Место для заголовка UDP пакета (8 байт)
    uint32_t dataSize = 4 * ((rand() % maxDataSize) / 4);        // Получение размера генерируемых данных с выравниванием по 4 байтам
    tmiStruct->dataLen = dataSize + sizeof(UDPHeader);           // Копирование длины данных, с учетом длины заголовка UDP
    auto dataPolePtr = (uint8_t *)&(tmiStruct->minSizeData); // Получение адреса начала блока данных
    auto crcPolePtr =                                            // Получение адреса поля CRC
        (uint32_t *)(dataPolePtr + tmiStruct->dataLen);          // Интепретация адреса следующего байта после блока данных

    genUDPData(dataPolePtr, dataSize, srcPort, dstPort); // Генерирование случайных данных
    *crcPolePtr = 0;                                     // Создание пакета без генерирования CRC
    return tmiStructLenWithoutData + tmiStruct->dataLen; // Возврат длины созданного пакета
}

uint32_t TMIGenerator::getMyIP()
{
    auto host_entry = gethostbyname("localhost");               // Получение адреса структуры IP адреса
    if (host_entry == nullptr)                                  // Проверка на получение структуры IP адреса
        return 0x7F000001;                                      // Возврат адрес локального хоста (127.0.0.1), с случае ошибки
    uint32_t outIP = *(uint32_t *)(host_entry->h_addr_list[0]); // Интепретация байтового массива хранения IP в число unsigned int
    return outIP;                                               // Возврат определенного IP адреса
}

void TMIGenerator::genUDPData(void *dataBuffer, size_t dataLen, uint16_t srcPort, uint16_t dstPort)
{
    auto udpHeader = (UDPHeader *)dataBuffer;         // Интепретация в виде заголовка UDP пакета
    udpHeader->srcPort = srcPort;                     // Копирование порта источника
    udpHeader->dstPort = dstPort;                     // Копирование порта приемника
    udpHeader->dataLen = dataLen + sizeof(UDPHeader); // Копирование длины пакета (с учетом заголовка)
    auto udpPacket = (uint8_t *)dataBuffer;           // Интепретация в виде байтовго массива
    for (size_t i = 0; i < dataLen; ++i)              // Цикл заполения данными
        udpPacket[sizeof(UDPHeader) + i] = rand();    // Заполнение случайными данными
}

uint64_t TMIGenerator::getCurrentNETTime()
{
    return HUNDRED_NS_IN_SEC * (std::time(nullptr) + NET_TIME_TO_UNIX_TIME_SEC); // Возврат текущего времени в виде .NET DateTime.Ticks
}
