#include "message.h"

// Receive from ESP
void msgDecodeFrameMaster(uint8_t *dataIn, FrameMaster_t *frameOut)
{
    uint16_t index = 0;

    frameOut->startFrame = Convert_From_Bytes_To_U16(dataIn[index], dataIn[index + 1]);
    index += 2;

    frameOut->typeMsg = dataIn[index++];

    frameOut->lengthData = Convert_From_Bytes_To_U8(dataIn[index]);
    index += 1;

    for (uint16_t i = 0; i < frameOut->lengthData && i < 12; i++)
    {
        frameOut->data[i] = dataIn[index++];
    }

    frameOut->checkFrame = Convert_From_Bytes_To_U16(dataIn[index], dataIn[index + 1]);
}

// Respond to ESP
uint8_t msgCreateFrameSlave(FrameSlave_t *frameIn, uint8_t *dataOut)
{
    uint8_t frameLen = 0;

    if (frameIn->startFrame != START_BYTE)
        return 0;

    switch (frameIn->typeMsg)
    {
    case TYPE_MSG_ANALOG_MRPM_UPDATE:
    case TYPE_MSG_DIGITAL_MRPM_UPDATE:
    case TYPE_MSG_RAMP_MRPM_UPDATE:
        frameIn->lengthData = LENGTH_SLAVE_DATA_MRPM;
        break;
    case TYPE_MSG_RAMP_FINISH:
        frameIn->lengthData = LENGTH_OTHER_DATA_TYPE;
        break;
    default:
        frameIn->lengthData = LENGTH_OTHER_DATA_TYPE;
        break;
    }

    dataOut[0] = frameIn->startFrame & 0xFF;
    dataOut[1] = (frameIn->startFrame >> 8) & 0xFF;
    dataOut[2] = frameIn->typeMsg;
    dataOut[3] = frameIn->lengthData;

    for (uint8_t i = 0; i < frameIn->lengthData; i++)
    {
        dataOut[4 + i] = frameIn->data[i];
    }

    frameLen = LENGTH_DATA_HEADER + frameIn->lengthData;

    uint16_t crc = msgCalculateCrc(dataOut, frameLen);
    frameIn->checkFrame = crc;

    dataOut[frameLen] = crc & 0xFF;
    dataOut[frameLen + 1] = (crc >> 8) & 0xFF;

    frameLen += 2;
    return frameLen;
}

uint16_t msgCalculateCrc(uint8_t *buf, uint8_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t pos = 0; pos < length; pos++)
    {
        crc ^= (uint16_t)buf[pos];
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

uint8_t msgCheckCrc(uint8_t *frameData, uint16_t bufLen)
{
    if (bufLen < LENGTH_DATA_HEADER + 2)
        return 0;

    uint8_t lengthData = Convert_From_Bytes_To_U8(frameData[3]);

    uint16_t crcCalc = msgCalculateCrc(frameData, LENGTH_DATA_HEADER + lengthData);
    uint16_t crcFrame = Convert_From_Bytes_To_U16(frameData[LENGTH_DATA_HEADER + lengthData],
                                                  frameData[LENGTH_DATA_HEADER + lengthData + 1]);

    if (crcCalc == crcFrame)
        return 1;

    return 0;
}
