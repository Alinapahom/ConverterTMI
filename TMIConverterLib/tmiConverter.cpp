#include "tmiConverter.hpp"
#include <bit>

#define NET_TIME_TO_UNIX_TIME_SEC 62135596800
#define INITIAL_TMI_MARKER 0x551CBEA7
#define BYTE_ADDRESS(buffer, element) &((uint8_t *)buffer)[element]
#define WORD_ADDRESS(buffer, element) ((uint32_t *)BYTE_ADDRESS(buffer, element))

InitialTMI TMIConverter::decodeInitialTMI(void *inputTMIPtr, size_t inputLen)
{
    constexpr auto dataMemberPosWord = offsetof(InitialTMI, InitialTMI::data);         // Позиция поля адреса данных
    constexpr auto minBufferSize_1 = sizeof(InitialTMI) - 1;                           // Минимальный размер буфера (мин длина UDP пакета = 8 байт) - 1
    InitialTMI initialTMI;                                                             // Определение исходной структуры
    if (inputLen > minBufferSize_1)                                                    // Проверка на размер буфера
    {                                                                                  //
        for (size_t i = 0; i < inputLen - minBufferSize_1; ++i)                        // Цикл поиска начала структуры пословно
        {                                                                              //
            if (*WORD_ADDRESS(inputTMIPtr, i) == INITIAL_TMI_MARKER)                   // Начало исходной структуры
            {                                                                          //
                memcpy(&initialTMI, BYTE_ADDRESS(inputTMIPtr, i), sizeof(InitialTMI)); // Копирование полей исходной структуры
                initialTMI.data = BYTE_ADDRESS(inputTMIPtr, i + dataMemberPosWord);    // Запись адреса массива данных в последнее поле исходной структуры
                break;                                                                 // Выход из цикла
            }                                                                          //
        }                                                                              //
    }                                                                                  //
    return initialTMI;                                                                 // Возврат найденной структуры
}

size_t TMIConverter::convertTMI(void *targetTMIPtr, void *inputTMIPtr, size_t inputLen)
{
    InitialTMI initialTMI = decodeInitialTMI(inputTMIPtr, inputLen);  // Получение первичной структуры ТМИ
    if (initialTMI.marker != INITIAL_TMI_MARKER)                      // Проверка маркера правильно загруженной первичной структуры
        return 0;                                                     // Возврат нулевой длины выходного буфера в качестве ошибки
    TargetInfoTMI targetTMI;                                          // Определение целевой структуры
    targetTMI.srcType = {3, "UDP"};                                   // Запись строки в структуру
    targetTMI.srcNum.data = _byteswap_ushort(initialTMI.srcPort);     // Копирование номера порта источника в качестве номер источника с поворотом байт
    targetTMI.time = convertTime(initialTMI.timeStamp);               // Преобразование .Net времени
    targetTMI.data =                                                  // Копирование байтового массива с пакетом UDP
        {static_cast<uint16_t>(initialTMI.dataLen), initialTMI.data}; //
    targetTMI.propertiesCount = 3;                                    // Число определенных свойств
    targetTMI.properties = new Property_t[3];                         // Выделение памяти для хранения свойств
    targetTMI.properties[0] =                                         // Копирование IP адреса источника
        {0, sizeof(initialTMI.srcIP), &initialTMI.srcIP};             //
    targetTMI.properties[1] =                                         // Копирование порта получателя
        {1, sizeof(initialTMI.dstPort), &initialTMI.dstPort};         //
    targetTMI.properties[2] =                                         // Копирование порта источника
        {2, sizeof(initialTMI.srcPort), &initialTMI.srcPort};         //
    auto addBytesLen = extractTargetTMI(targetTMIPtr, targetTMI);     // Распаковка предварительной структуры
    delete[] targetTMI.properties;                                    // Освобождение выделенной памяти для хранения свойств
    return addBytesLen;                                               // Возврат числа байт записанных в выходной буфер
}

size_t TMIConverter::addFileHeaderTMI(void *targetTMIPtr)
{
    TargetFileHeaderTMI targetTMI;                                 // Определение целевой структуры
    targetTMI.version = {1, 1};                                    // Добавление версии протокола
    memcpy(targetTMIPtr, &targetTMI, sizeof(TargetFileHeaderTMI)); // Копирование
    return sizeof(TargetFileHeaderTMI);
}

size_t TMIConverter::extractTargetTMI(void *targetTMIPtr, TargetInfoTMI &targetStruct)
{
    // Смещение следующего копируемого байта
    size_t targetTMICurrentByte = 0;

    /// Распаковка целевой структуры по адресу targetTMIPtr
    // Извлечение заголовка и добавление к переменной общего числа байт значения на которое увеличился буфер
    TargetHeaderTMI outputHeader = targetStruct.header;
    targetTMICurrentByte += outputHeader.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение строки типа источника и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.srcType.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение номера пакета и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.srcNum.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение времени и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.time.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение данных и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.data.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Извлечение числа переданных свойств и добавление к переменной общего числа байт значения на которое увеличился буфер
    targetTMICurrentByte += targetStruct.propertiesCount.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Цикл извлечения свойств
    for (size_t i = 0; i < targetStruct.propertiesCount.data; ++i)
        // Извлечение свойства и добавление к переменной общего числа байт значения на которое увеличился буфер
        targetTMICurrentByte += targetStruct.properties[i].extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    // Возврат общего числа байт
    return targetTMICurrentByte;
}

Instant_t TMIConverter::convertTime(uint64_t NETTime) // .NET DataTime - число 100 нс интервалов от 01.01.0001 00:00:00
                                                      // UNIX Timestamp - число секунд от 01.01.1970 00:00:00
                                                      // Число секундных интервалов до UNIX Timestamp 62135596800 (NET_TIME_TO_UNIX_TIME_SEC)
{
    // constexpr uint64_t testNETTime = 638261216460000000;         // Тестовое время 28.07.2023 06:14:06
    int32_t nanoseconds = (NETTime % int(1e7)) * 100;               // Вычисление наносекунд (Получение числа 100 нс тиков и умножение на 100)
    int64_t unixTime = (NETTime / 1e7) - NET_TIME_TO_UNIX_TIME_SEC; // Перевод секунд в .NET времени в UNIX время
    return {unixTime, nanoseconds};                                 // Возврат структуры времени
}

size_t TMIConverter::nextInitialTMIPos(void *inputTMIPtr, size_t inputLen)
{
    constexpr auto dataLenPos =                                       // Позиция поля длины данных (в байтах)
        offsetof(InitialTMI, InitialTMI::dataLen);                    //
    constexpr auto dataPos =                                          // Позиция поля адреса данных (в байтах)
        offsetof(InitialTMI, InitialTMI::data);                       //
    constexpr auto minTwoBufferSize_1 = (2 * sizeof(InitialTMI)) - 1; // Минимальный размер буфера (мин длина UDP пакета = 8 байт) - 1
    size_t nextStructPos = 0;                                         // Позиция для поиска следующей структуры
    if (inputLen < minTwoBufferSize_1)                                // Проверка на размер входного буфера (минимум два буфера)
        return nextStructPos;                                         // Возврат нуля в случае меньшего размера буфера
    for (size_t i = 0; i < inputLen - minTwoBufferSize_1; ++i)        // Цикл поиска начала структуры пословно
        if (*WORD_ADDRESS(inputTMIPtr, i) == INITIAL_TMI_MARKER)      // Начало исходной структуры
        {                                                             //
            uint32_t dataLen =                                        // Извлечение длины сообщения
                *WORD_ADDRESS(inputTMIPtr, i + dataLenPos);           //
            nextStructPos = dataPos + dataLen + sizeof(uint32_t);     // Вычисление позиции следующего поиска (с добавлением поля CRC)
            break;                                                    // Выход из цикла
        }                                                             //
    if (nextStructPos >= inputLen)                                    // Если позиция следующей структуры выходит за пределы файла
        nextStructPos = 0;                                            // Обнуление позиции
    return nextStructPos;                                             // Возврат позиции начала поиска следующей структуры
}
