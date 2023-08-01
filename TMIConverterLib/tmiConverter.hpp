#ifndef __TMI_CONVERTER_HPP
#define __TMI_CONVERTER_HPP
#pragma once
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <bit>
#define SSPP_MARKER 0x50505353

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
    uint8_t *data = nullptr;
};

struct UShort_t
{
    uint16_t data;

    UShort_t(uint16_t inData = 0) : data(inData) {}
    operator uint16_t() { return (std::endian::native == std::endian::little) ? _byteswap_ushort(data) : data; }

    size_t extract(void *outputBuffer)
    {
        size_t expandSize = 0;                                         // Переменная показывающая на сколько байт увеличился размер буфера
        auto shortBuffer = reinterpret_cast<uint16_t *>(outputBuffer); // Интепретация буфера в виде массива элементов c типом полуслова
        shortBuffer[0] = *this;                                        // Запись значения в ячейку
        expandSize += sizeof(data);                                    // Увеличение на один short
        return expandSize;                                             // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Byte_t
{
    uint8_t data;

    Byte_t(uint8_t inData = 0) : data(inData) {}
    operator uint8_t() { return data; }

    size_t extract(void *outputBuffer)
    {
        size_t expandSize = 0;                                       // Переменная показывающая на сколько байт увеличился размер буфера
        auto byteBuffer = reinterpret_cast<uint8_t *>(outputBuffer); // Интепретация буфера в виде байтового массива
        byteBuffer[0] = data;                                        // Копирование значения длины массива
        expandSize += sizeof(data);                                  // Увеличение на один short
        return expandSize;                                           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Instant_t
{
    int64_t utcTime;
    int32_t nanoSec;

    size_t extract(void *outputBuffer)
    {
        auto doubleWordBuffer = reinterpret_cast<uint64_t *>(outputBuffer);                     // Интепретация буфера в виде массива 64-битных слов
        doubleWordBuffer[0] =                                                                   // Копирование значения времени UTC c проверкой порядка байт платформы обработчика
            (std::endian::native == std::endian::little) ? _byteswap_uint64(utcTime) : utcTime; //
                                                                                                //
        auto wordBuffer = reinterpret_cast<uint32_t *>(outputBuffer);                           // Интепретация буфера в виде массива слов
        wordBuffer[2] =                                                                         // Копирование значения наносекунд c проверкой порядка байт платформы обработчика
            (std::endian::native == std::endian::little) ? _byteswap_ulong(nanoSec) : utcTime;  //
        return sizeof(Instant_t);                                                               // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct VeryShortString_t
{
    Byte_t stringCount;
    const char *string;

    size_t extract(void *outputBuffer)
    {
        auto byteBuffer = reinterpret_cast<uint8_t *>(outputBuffer); // Интепретация буфера в виде байтового массива
        byteBuffer[0] = stringCount;                                 // Копирование значения длины строки
        size_t expandSize = sizeof(stringCount);                     // Увеличение на один байт
                                                                     //
        memcpy(&byteBuffer[1], string, stringCount.data);            // Копирование строки со смещением в один байт
        expandSize += stringCount.data;                              // Увеличение на число скопированных байт
        return expandSize;                                           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct ShortByteArray_t
{
    UShort_t arrayCount;
    uint8_t *array;

    size_t extract(void *outputBuffer)
    {
        auto shortBuffer = reinterpret_cast<uint16_t *>(outputBuffer); // Интепретация буфера в виде массива полуслов
        shortBuffer[0] = arrayCount;                                   // Копирование значения длины строки
        size_t expandSize = sizeof(arrayCount);                        // Увеличение на размер поля arrayCount
                                                                       //
        auto byteBuffer = reinterpret_cast<uint8_t *>(outputBuffer);   // Интепретация буфера в виде байтового массива
        memcpy(&byteBuffer[2], array, arrayCount.data);                // Копирование строки со смещением в один байт
        expandSize += arrayCount.data;                                 // Увеличение на число скопированных байт
        return expandSize;                                             // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Version_t
{
    UShort_t major;
    UShort_t minor;
    size_t extract(void *outputBuffer)
    {
        auto shortBuffer = reinterpret_cast<uint16_t *>(outputBuffer); // Интепретация буфера в виде массива полуслов
        shortBuffer[0] = major;                                        // Копирование значения major                                                                  //
        shortBuffer[1] = minor;                                        // Копирование значения minor
        return sizeof(Version_t);                                      // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Property_t
{
    Byte_t type;
    Byte_t dataLen;
    void *data;
    size_t extract(void *outputBuffer)
    {
        auto byteBuffer = reinterpret_cast<uint8_t *>(outputBuffer); // Интепретация буфера в виде байтового массива
        byteBuffer[0] = type;                                        // Копирование значения типа свойства
        byteBuffer[1] = dataLen;                                     // Копирование значения длины
        size_t expandSize = 2;                                       // Увеличение на два байта
                                                                     //
        if (dataLen != 0)                                            // Проверка на ненулевую длину свойства
            memcpy(&byteBuffer[2], data, dataLen);                   // Копирование свойства со смещением в два байта
        expandSize += dataLen;                                       // Увеличение на число скопированных байт
        return expandSize;                                           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

#pragma pack(1)
struct TargetHeaderTMI
{
    uint32_t signature;
    uint8_t type;

    size_t extract(void *outputBuffer)
    {
        size_t expandSize = 0;                               // Переменная показывающая на сколько байт увеличился размер буфера
        memcpy(outputBuffer, this, sizeof(TargetHeaderTMI)); // Копирование структуры в буфер
        expandSize += sizeof(TargetHeaderTMI);               // Увеличение на число скопированных байт
        return expandSize;                                   // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct TargetInfoTMI
{
    const TargetHeaderTMI header = {SSPP_MARKER, 0};
    VeryShortString_t srcType;
    UShort_t srcNum;
    Instant_t time;
    ShortByteArray_t data;
    Byte_t propertiesCount;
    Property_t *properties;
};

#pragma pack(1)
struct TargetFileHeaderTMI
{
    const TargetHeaderTMI header = {SSPP_MARKER, 128};
    Version_t version;
};

// constexpr uint16_t* testPtr = (uint16_t*)0x456;
// constexpr uint16_t* newPtr = testPtr + 1;
// auto test = sizeof(Version_t);

class TMIConverter
{
public:
    size_t convertTMI(void *targetTMIPtr, void *inputTMIPtr, size_t inputLen);
    size_t addFileHeaderTMI(void *targetTMIPtr);

    static int checkInitialTMI(void *inputTMIPtr, size_t inputLen);
    static int numInitialTMI(void *inputTMIPtr, size_t inputLen);
    static Instant_t convertTime(uint64_t NETTime);
    static size_t nextInitialTMIPos(void *inputTMIPtr, size_t inputLen);

private:
    InitialTMI decodeInitialTMI(void *inputTMIPtr, size_t inputLen);
    size_t extractTargetTMI(void *targetTMIPtr, TargetInfoTMI &targetStruct);
};

#endif // __TMI_CONVERTER_HPP