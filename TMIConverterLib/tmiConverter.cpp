#include "tmiConverter.hpp"
#include <ranges>
#define ELEMENT_ADDRESS(buffer, element) &(buffer[element])
#define NET_TIME_TO_UNIX_TIME_SEC 62135596800
#define INITIAL_TMI_MARKER 0x551CBEA7

InitialTMI decodeInitialTMI(void *inputTMIPtr, size_t inputLen)
{
    constexpr auto dataMemberPosWord =                                    // Позиция поля адреса данных (в словах)
        offsetof(InitialTMI, InitialTMI::data) / sizeof(uint32_t);        //
    constexpr auto minBufferSize_1 = sizeof(InitialTMI) - 1;              // Минимальный размер буфера (мин длина UDP пакета = 8 байт) - 1
    InitialTMI initialTMI;                                                // Определение исходной структуры
    if (inputLen > minBufferSize_1)                                       // Проверка на размер буфера
    {                                                                     //
        auto wordArray = (uint32_t *)inputTMIPtr;                         // Интепретация входного буфера в виде массива слов
        for (size_t i = 0; i < inputLen - minBufferSize_1; ++i)           // Цикл поиска начала структуры пословно
        {                                                                 //
            if (wordArray[i] == INITIAL_TMI_MARKER)                       // Начало исходной структуры
            {                                                             //
                memcpy(&initialTMI, &(wordArray[i]), sizeof(InitialTMI)); // Копирование полей исходной структуры
                initialTMI.data =                                         // Запись адреса массива данных в последнее поле исходной структуры
                    (uint8_t *)&(wordArray[i + dataMemberPosWord]);       //
                break;                                                    // Выход из цикла
            }                                                             //
        }                                                                 //
    }                                                                     //
    return initialTMI;                                                    // Возврат найденной структуры
}

size_t convertTMI(void *targetTMIPtr, void *inputTMIPtr, size_t inputLen)
{
    InitialTMI initialTMI = decodeInitialTMI(inputTMIPtr, inputLen);                // Получение первичной структуры ТМИ
    if (initialTMI.marker != INITIAL_TMI_MARKER)                                    // Проверка маркера правильно загруженной первичной структуры
        return 0;                                                                   // Возврат нулевой длины выходного буфера в качестве ошибки
    TargetInfoTMI targetTMI;                                                        // Определение целевой структуры
    targetTMI.srcType = {3, "UDP"};                                                 // Запись строки
    targetTMI.srcNum.data = initialTMI.srcPort;                                     // Копирование номера порта источника в качестве номер источника
    targetTMI.time = convertTime(initialTMI.timeStamp);                             // Преобразование .Net времени
    targetTMI.data = {(uint16_t)initialTMI.dataLen, initialTMI.data};               // Копирование байтового массива с пакетом UDP
    targetTMI.propertiesCount = 3;                                                  // Число определенных свойств
    targetTMI.properties = new Property_t[targetTMI.propertiesCount.data];          // Выделение памяти для хранения свойств
    targetTMI.properties[0] = {0, sizeof(initialTMI.srcIP), &initialTMI.srcIP};     // Копирование IP адреса источника
    targetTMI.properties[1] = {1, sizeof(initialTMI.dstPort), &initialTMI.dstPort}; // Копирование порта получателя
    targetTMI.properties[2] = {2, sizeof(initialTMI.srcPort), &initialTMI.srcPort}; // Копирование порта источника
    auto addBytesLen = extractTargetTMI((uint8_t *)targetTMIPtr, targetTMI);        // Распаковка предварительной структуры
    delete[] targetTMI.properties;                                                  // Освобождение выделенной памяти для хранения свойств
    return addBytesLen;                                                             // Возврат числа байт записанных в выходной буфер
}

size_t extractTargetTMI(uint8_t *targetTMIPtr, TargetInfoTMI &targetStruct)
{
    // Смещение следующего копируемого байта
    size_t targetTMICurrentByte = 0;

    /// Распаковка целевой структуры по адресу targetTMIPtr
    // Извлечение заголовка и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += ((TargetHeaderTMI *)&(targetStruct.header))->extract(targetTMIPtr);
    // Извлечение строки типа источника и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.srcType.extract(ELEMENT_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение номера пакета и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.srcNum.extract(ELEMENT_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение времени и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.time.extract(ELEMENT_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение данных и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.data.extract(ELEMENT_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение числа переданных свойств и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.propertiesCount.extract(ELEMENT_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Цикл извлечения свойств
    for (size_t i = 0; i < targetStruct.propertiesCount.data; ++i)
        // Извлечение свойства и добавление к переменной общего числа байт значения на которое увеличился буфер
        targetTMICurrentByte += targetStruct.properties[i].extract(ELEMENT_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Возврат общего числа байт
    return targetTMICurrentByte;
}

Instant_t convertTime(uint64_t NETTime) // .NET DataTime - число 100 нс интервалов от 01.01.0001 00:00:00
                                        // UNIX Timestamp - число секунд от 01.01.1970 00:00:00
                                        // Число секундных интервалов до UNIX Timestamp 62135596800 (NET_TIME_TO_UNIX_TIME_SEC)
{
    // constexpr uint64_t testNETTime = 638261216460000000;          // Тестовое время 28.07.2023 06:14:06
    int32_t nanoseconds = (NETTime % int(1e7)) * 100;               // Вычисление наносекунд (Получение числа 100 нс тиков и умножение на 100)
    int64_t unixTime = (NETTime / 1e7) - NET_TIME_TO_UNIX_TIME_SEC; // Перевод секунд в .NET времени в UNIX время
    return {unixTime, nanoseconds};
}