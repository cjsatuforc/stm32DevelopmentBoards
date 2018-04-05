/*******************************************************************************
 * StmPlusPlus: object-oriented library implementing device drivers for 
 * STM32F3 and STM32F4 MCU
 * *****************************************************************************
 * Copyright (C) 2016-2017 Mikhail Kulesh
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "AudioDac_UDA1334.h"

#include <cmath>
#define M_PI 3.14159265358979323846

using namespace StmPlusPlus;
using namespace StmPlusPlus::Devices;

#define USART_DEBUG_MODULE "DAC: "

AudioDac_UDA1334::AudioDac_UDA1334 (I2S & _i2s, IOPort::PortName powerPort, uint32_t powerPin):
    i2s(_i2s),
    power(powerPort, powerPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_HIGH, true, false),
    sourceType(SourceType::STREAM),
    dataPtr1(NULL),
    dataPtr2(NULL),
    currDataBuffer(NULL),
    testPin(NULL)
{
    dataPtr1 = &dataBuffer1[0];
    dataPtr2 = &dataBuffer2[0];
}


bool AudioDac_UDA1334::start (AudioDac_UDA1334::SourceType s)
{
    sourceType = s;
    currDataBuffer = NULL;

    power.putBit(true);
    HAL_StatusTypeDef status = i2s.start();
    USART_DEBUG("I2S start status: " << status);
    if (status != HAL_OK)
    {
        return false;
    }

    switch (sourceType)
    {
    case SourceType::STREAM:
        // nothing to do
        break;
    case SourceType::TEST_LIN:
        makeTestSignalLin();
        break;
    case SourceType::TEST_SIN:
        makeTestSignalSin();
        break;
    }

    currDataBuffer = dataPtr2;
    onBlockTransmissionFinished();

    return true;
}


void AudioDac_UDA1334::onBlockTransmissionFinished ()
{
    if (testPin != NULL)
    {
        testPin->putBit(!testPin->getBit());
    }
    if (currDataBuffer == NULL)
    {
        return;
    }
    HAL_StatusTypeDef status = HAL_ERROR;
    if (currDataBuffer == dataPtr1)
    {
        currDataBuffer = dataPtr2;
        status = i2s.transmit(dataPtr2, BLOCK_SIZE2);
    }
    else
    {
        currDataBuffer = dataPtr1;
        status = i2s.transmit(dataPtr1, BLOCK_SIZE2);
    }
    if (status != HAL_OK)
    {
        USART_DEBUG("I2S/DMA transmission error: " << status);
    }
}


void AudioDac_UDA1334::makeTestSignalLin ()
{
    uint16_t l, r;
    double maxValue = (double)0xFFFF;
    double f = (double)(BLOCK_SIZE2);
    for (size_t i = 0; i < BLOCK_SIZE2; i+=2)
    {
        l = r = (uint16_t)(((double)i/f) * maxValue) + MSB_OFFSET;
        dataPtr1[i + 0] = l;
        dataPtr1[i + 1] = r;
        l = r = (uint16_t)((1.0 - (double)i/f) * maxValue) + MSB_OFFSET;
        dataPtr2[i + 0] = l;
        dataPtr2[i + 1] = r;
    }
    USART_DEBUG("WAV streaming (LIN test signal) started...");
}


void AudioDac_UDA1334::makeTestSignalSin ()
{
    uint16_t l, r;
    double maxValue = (double)0xFFFF;
    double f = (double)(BLOCK_SIZE2);
    for (size_t i = 0; i < BLOCK_SIZE2; i+=2)
    {
        l = (sin(2.0*M_PI*(double)i/f) + 1.0) * maxValue/2.0 + MSB_OFFSET;
        r = (cos(4.0*M_PI*(double)i/f) + 1.0) * maxValue/2.0 + MSB_OFFSET;
        dataPtr1[i + 0] = l;
        dataPtr1[i + 1] = r;
        dataPtr2[i + 0] = l;
        dataPtr2[i + 1] = r;
    }
    USART_DEBUG("WAV streaming (SIN test signal) started...");
}

