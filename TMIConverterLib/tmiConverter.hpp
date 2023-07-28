#ifndef __TMI_CONVERTER_HPP
#define __TMI_CONVERTER_HPP
#pragma once
#include <cstdint>
#include <cstdlib>
#include <memory>

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
    operator uint16_t() { return data; }
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;                       // Переменная показывающая на сколько байт увеличился размер буфера
        auto shortBuffer = (uint16_t *)outputBuffer; // интепретация буфера массива в виде массива массива элементов типа short
        shortBuffer[0] = data;                       // Копирование значения длины массива
        expandSize += sizeof(data);                  // Увеличение на один short
        return expandSize;                           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Byte_t
{
    uint8_t data;

    Byte_t(uint8_t inData = 0) : data(inData) {}
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;      // Переменная показывающая на сколько байт увеличился размер буфера
        outputBuffer[0] = data;     // Копирование значения длины массива
        expandSize += sizeof(data); // Увеличение на один short
        return expandSize;          // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Instant_t
{
    int64_t utcTime;
    int32_t nanoSec;
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;                         // Переменная показывающая на сколько байт увеличился размер буфера
        memcpy(outputBuffer, this, sizeof(Instant_t)); // Копирование структуры в буфер
        expandSize += sizeof(Instant_t);               // Увеличение на число скопированных байт
        return expandSize;                             // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct VeryShortString_t
{
    uint8_t stringCount;
    const char *string;
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;         // Переменная показывающая на сколько байт увеличился размер буфера
        outputBuffer[0] = stringCount; // Копирование значения длины строки
        ++expandSize;                  // Увеличение на один байт
        memcpy(outputBuffer + 1,       // Копирование строки со смещением в один байт
               string,                 //
               stringCount);           //
        expandSize += stringCount;     // Увеличение на число скопированных байт
        return expandSize;             // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct ShortByteArray_t
{
    uint16_t arrayCount;
    uint8_t *array;
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;                       // Переменная показывающая на сколько байт увеличился размер буфера
        auto shortBuffer = (uint16_t *)outputBuffer; // интепретация буфера массива в виде массива массива элементов типа short
        shortBuffer[0] = arrayCount;                 // Копирование значения длины массива
        expandSize += sizeof(arrayCount);            // Увеличение на один short
        memcpy(shortBuffer + 1,                      // Копирование массива со смещением в один short
               array,                                //
               arrayCount);                          //
        expandSize += arrayCount;                    // Увеличение на число скопированных байт
        return expandSize;                           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Version_t
{
    uint16_t major;
    uint16_t minor;
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;                       // Переменная показывающая на сколько байт увеличился размер буфера
        auto shortBuffer = (uint16_t *)outputBuffer; // интепретация байтового буфера в виде массива элементов типа short
        shortBuffer[0] = major;                      // Копирование значения major
        shortBuffer[1] = minor;                      // Копирование значения minor
        expandSize += sizeof(Version_t);             // Увеличение на число скопированных байт
        return expandSize;                           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct Property_t
{
    uint8_t type;
    uint8_t dataLen;
    void *data;
    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;       // Переменная показывающая на сколько байт увеличился размер буфера
        outputBuffer[0] = type;      // Копирование значения типа свойства
        outputBuffer[1] = dataLen;   // Копирование значения длины
        expandSize += 2;             // Увеличение на два байта
        if (dataLen != 0)            // Проверка на ненулевую длину свойства
        {                            //
            memcpy(outputBuffer + 2, // Копирование свойства со смещением в два байта
                   data,             //
                   dataLen);         //
            expandSize += dataLen;   // Увеличение на число скопированных байт
        }                            //
        return expandSize;           // Возвращаем число, указывающее на сколько расширился буфер
    };
};

#pragma pack(1)
struct TargetHeaderTMI
{
    uint32_t signature;
    uint8_t type;

    size_t extract(uint8_t *outputBuffer)
    {
        size_t expandSize = 0;                               // Переменная показывающая на сколько байт увеличился размер буфера
        memcpy(outputBuffer, this, sizeof(TargetHeaderTMI)); // Копирование структуры в буфер
        expandSize += sizeof(TargetHeaderTMI);               // Увеличение на число скопированных байт
        return expandSize;                                   // Возвращаем число, указывающее на сколько расширился буфер
    };
};

struct TargetInfoTMI
{
    const TargetHeaderTMI header = {0x53535050, 0}; // Код 0x53535050 => Числовое отображение строки SSPP
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
    const TargetHeaderTMI header = {0x53535050, 128}; // Код 0x53535050 => Числовое отображение строки SSPP
    Version_t version;
};

// constexpr uint16_t* testPtr = (uint16_t*)0x456;
// constexpr uint16_t* newPtr = testPtr + 1;
// auto test = sizeof(TargetInfoTMI);

InitialTMI decodeInitialTMI(void *inputTMIPtr, size_t inputLen);
void *nextInitialTMI(void *inputTMIPtr);
size_t convertTMI(void *targetTMIPtr, void *inputTMIPtr, size_t inputLen);
size_t extractTargetTMI(uint8_t *targetTMIPtr, TargetInfoTMI &targetStruct);
Instant_t convertTime(uint64_t NETTime);

#endif // __TMI_CONVERTER_HPP