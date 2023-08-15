//
// Created by cha on 23. 8. 15.
//

#include "gps.hpp"
#include "gpsInterface.hpp"

#define CLK_A 0x00
#define CLK_B 0x0A

int main()
{
    char testSequence[GPS_MAX_START_SEQUENCE_SIZE];
    char testLine[GPS_MAX_LINE_SIZE];

    int testSequenceSize[GPS_MAX_START_SEQUENCE_SIZE];
    int testLineSize[GPS_MAX_LINE_SIZE];
    int count = 1;

    uint8_t temp[300];
    uint64_t tempTimeStamp;

    gps_serial_port hrtkF9P;
    gps_parsed_data_t testData;
    gps_protocol_and_message testMatch;

    testMatch.protocol = GPS_PROTOCOL_TYPE_UBX;
    testMatch.message = GPS_UBX_TYPE_NAV_PVT;

    gps_interface_open(&hrtkF9P, "/dev/ttyUSB0", B115200);

    while (true){
        gps_interface_get_line(&hrtkF9P,
                               testSequence,
                               testSequenceSize,
                               testLine,
                               testLineSize,
                               0);

        gps_parse_result_t tempResult = gps_parse_buffer(&testData, &testMatch, testLine, tempTimeStamp);
        if(tempResult == GPS_PARSE_RESULT_OK){
            printf( "%f",testData.pvt.lat);
            printf(", ");
            printf( "%f",testData.pvt.lon);
            printf("\n");
            // Do stuff
        }else{
            printf("Protocol %d. Message %d. Parse failed with error: %s", testMatch.protocol, testMatch.message, gps_parse_result_string[tempResult]);
        }
    }


}
