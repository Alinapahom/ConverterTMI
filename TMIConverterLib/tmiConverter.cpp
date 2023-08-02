#include "tmiConverter.hpp"
#include <bit>

#define NET_TIME_TO_UNIX_TIME_SEC 62135596800
#define INITIAL_TMI_MARKER 0x551CBEA7
#define BYTE_ADDRESS(buffer, element) &((uint8_t *)buffer)[element]
#define WORD_ADDRESS(buffer, element) ((uint32_t *)BYTE_ADDRESS(buffer, element))

InitialTMI TMIConverter::decodeInitialTMI(void *inputTMIPtr, size_t inputLen)
{
    constexpr auto dataMemberPosWord = 36;                                            // Позиция поля адреса данных
    InitialTMI initialTMI;                                                            // Определение исходной структуры
    auto structOffset = checkInitialTMI(inputTMIPtr, inputLen);                       // Получение смещения начала структуры
    if (structOffset == -1)                                                           // Проверка на ошибки
        return initialTMI;                                                            // Возврат пустой структуры
    memcpy(&initialTMI, BYTE_ADDRESS(inputTMIPtr, structOffset), sizeof(InitialTMI)); // Копирование исходной структуры
    initialTMI.data = BYTE_ADDRESS(inputTMIPtr, structOffset + dataMemberPosWord);    // Запись адреса массива данных в последнее поле исходной структуры
    return initialTMI;                                                                // Возврат найденной структуры
}

size_t TMIConverter::convertTMI(void *targetTMIPtr, void *inputTMIPtr, size_t inputLen)
{
    InitialTMI initialTMI = decodeInitialTMI(inputTMIPtr, inputLen);  // Получение первичной структуры ТМИ
    if (initialTMI.marker != INITIAL_TMI_MARKER)                      // Проверка маркера правильно загруженной первичной структуры
        return 0;                                                     // Возврат нулевой длины выходного буфера в качестве ошибки
    TargetInfoTMI targetTMI;                                          // Определение целевой структуры
    targetTMI.srcType = {3, "UDP"};                                   // Запись строки в структуру
    targetTMI.srcNum = initialTMI.srcPort;                            // Копирование номера порта источника в качестве номер источника
    targetTMI.time = convertTime(initialTMI.timeStamp);               // Преобразование .Net времени
    targetTMI.data =                                                  // Копирование байтового массива с пакетом UDP
        {static_cast<uint16_t>(initialTMI.dataLen), initialTMI.data}; //
    targetTMI.propertiesCount = 3;                                    // Число определенных свойств

    // Инициализация свойств
    targetTMI.properties = new Property_t[3];                    // Выделение памяти для хранения свойств
    uint32_t propertySrcIP = _OSSwapInt32(initialTMI.srcIP);     // Запись IP источника с перестановкой байт
    targetTMI.properties[0] =                                    // Копирование IP адреса источника
        {0, sizeof(initialTMI.srcIP), &propertySrcIP};           //
    uint32_t propertyDstPort = _OSSwapInt16(initialTMI.dstPort); // Запись порта назначения с перестановкой байт
    targetTMI.properties[1] =                                    // Копирование порта получателя
        {1, sizeof(initialTMI.dstPort), &propertyDstPort};       //
    uint32_t propertySrcPort = _OSSwapInt16(initialTMI.srcPort); // Запись порта источника с перестановкой байт
    targetTMI.properties[2] =                                    // Копирование порта источника
        {2, sizeof(initialTMI.srcPort), &propertySrcPort};       //

    auto addBytesLen = extractTargetTMI(targetTMIPtr, targetTMI); // Распаковка предварительной структуры
    delete[] targetTMI.properties;                                // Освобождение выделенной памяти для хранения свойств
    return addBytesLen;                                           // Возврат числа байт записанных в выходной буфер
}

size_t TMIConverter::addFileHeaderTMI(void *targetTMIPtr)
{
    TargetFileHeaderTMI targetTMI;   // Определение структуры заголовка файла
    targetTMI.version = {1, 1};      // Добавление версии протокола
    size_t targetTMICurrentByte = 0; // Смещение следующего копируемого байта

    TargetHeaderTMI outputHeader = targetTMI.header;
    targetTMICurrentByte += outputHeader.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    targetTMICurrentByte += targetTMI.version.extract(BYTE_ADDRESS(targetTMIPtr, targetTMICurrentByte));
    return targetTMICurrentByte;
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
    constexpr auto dataLenPos = 16;                                   // Позиция поля длины данных (в байтах)
    constexpr auto dataPos = 36;                                      // Позиция поля адреса данных (в байтах)
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

int TMIConverter::checkInitialTMI(void *inputTMIPtr, size_t inputLen)
{
    constexpr auto minBufferSize = sizeof(InitialTMI);           // Минимальный размер буфера (мин длина UDP пакета = 8 байт)
    if (inputLen < minBufferSize)                                // Проверка на размер буфера
        return -1;                                               //
    for (size_t i = 0; i < inputLen - minBufferSize + 1; ++i)    // Цикл поиска начала структуры
    {                                                            //
        if (*WORD_ADDRESS(inputTMIPtr, i) == INITIAL_TMI_MARKER) // Начало исходной структуры
            return i;                                            //
    }                                                            //
    return -1;                                                   // Если не нашелся маркер, возвращаем ошибку
}

int TMIConverter::numInitialTMI(void *inputTMIPtr, size_t inputLen)
{
    auto structOffset = checkInitialTMI(inputTMIPtr, inputLen);              // Получение смещения начала структуры
    if (structOffset == -1)                                                  // Проверка на ошибки
        return structOffset;                                                 // Возврат ошибки
    auto initialTMI = (InitialTMI *)BYTE_ADDRESS(inputTMIPtr, structOffset); // Определение исходной структуры
    return initialTMI->srcPort;                                              // Возврат порта отправителя в качестве номера
}
