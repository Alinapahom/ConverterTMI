#ifndef __TMI_GENERATOR_HPP
#define __TMI_GENERATOR_HPP
#pragma once
#include <cstdlib>
#include <cstdint>

class TMIGenerator
{
public:
    uint32_t innerCounter = 0;

public:
    TMIGenerator();
    /// @brief Генератор исходной структуры ТМИ и запись ее в буфер
    /// @param byteBuffer Адрес буфера для записи структуры
    /// @param bufferSize Размер буфера
    /// @return Размер сгенерированной структуры
    size_t generateTMI(void *byteBuffer, size_t bufferSize);

    /// @brief Получение IP адреса текущего компьютера
    /// @return IP адрес в байтовом формате
    static uint32_t getMyIP();

    static void genUDPData(void *dataBuffer, size_t dataLen, uint16_t srcPort, uint16_t dstPort);
    /// @brief Получение текущего времени
    /// @return Текущее время в формате .NET DateTime.Ticks
    static uint64_t getCurrentNETTime();
};

#endif // __TMI_GENERATOR_HPP