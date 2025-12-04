#include "convert.h"

/**
 * @name Convert_From_Float_To_Bytes
 * @brief Chuyen doi tu float sang mang byte
 * @param data: Gia tri float can chuyen doi
 * @return Mang byte sau khi chuyen doi
 */
uint8_t *Convert_From_Float_To_Bytes(float data)
{
    static data_format_float_bytes input_convert;
    input_convert.data_float = data;
    return input_convert.byte;
}

/**
 * @name Convert_From_Bytes_To_Float
 * @brief Chuyen doi tu mang byte sang float
 * @param data0: Byte 0
 * @param data1: Byte 1
 * @param data2: Byte 2
 * @param data3: Byte 3
 * @return Gia tri float sau khi chuyen doi
 */
float Convert_From_Bytes_To_Float(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3)
{
    data_format_float_bytes input_convert;
    input_convert.byte[0] = data0;
    input_convert.byte[1] = data1;
    input_convert.byte[2] = data2;
    input_convert.byte[3] = data3;
    return input_convert.data_float;
}

/**
 * @name Convert_From_Int_To_Bytes
 * @brief Chuyen doi tu uint32_t sang mang byte
 * @param data: Gia tri uint32_t can chuyen doi
 * @return Mang byte sau khi chuyen doi
 */
uint8_t *Convert_From_Int_To_Bytes(uint32_t data)
{
    static data_format_int_bytes input_convert;
    input_convert.data_int = data;
    return input_convert.byte;
}

/**
 * @name Convert_From_Bytes_To_Int
 * @brief Chuyen doi tu mang byte sang uint32_t
 * @param data0: Byte 0
 * @param data1: Byte 1
 * @param data2: Byte 2
 * @param data3: Byte 3
 * @return Gia tri uint32_t sau khi chuyen doi
 */
uint32_t Convert_From_Bytes_To_Int(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3)
{
    data_format_int_bytes input_convert;
    input_convert.byte[0] = data0;
    input_convert.byte[1] = data1;
    input_convert.byte[2] = data2;
    input_convert.byte[3] = data3;
    return input_convert.data_int;
}

/**
 * @name Convert_From_U16_To_Bytes
 * @brief Chuyen doi tu uint16_t sang mang byte
 * @param data: Gia tri uint16_t can chuyen doi
 * @return Mang byte sau khi chuyen doi
 */
uint8_t *Convert_From_U16_To_Bytes(uint16_t data)
{
    static data_format_u8_u16 input_convert;
    input_convert.data_u16 = data;
    return input_convert.byte;
}

/**
 * @name Convert_From_Bytes_To_U16
 * @brief Chuyen doi tu mang byte sang uint16_t
 * @param data0: Byte 0
 * @param data1: Byte 1
 * @return Gia tri uint16_t sau khi chuyen doi
 */
uint16_t Convert_From_Bytes_To_U16(uint8_t data0, uint8_t data1)
{
    data_format_u8_u16 input_convert;
    input_convert.byte[0] = data0;
    input_convert.byte[1] = data1;
    return input_convert.data_u16;
}

/**
 * @name Convert_From_Bytes_To_U8
 * @brief Chuyen doi tu byte sang uint8_t
 * @param data: Byte can chuyen doi
 * @return Gia tri uint8_t sau khi chuyen doi
 */
uint8_t Convert_From_Bytes_To_U8(uint8_t data)
{
    return data;
}
