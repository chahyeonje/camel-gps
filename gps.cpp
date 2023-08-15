//
// Created by cha on 23. 8. 15.
//

#include "gps.hpp"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void rfc1145_checksum(int8_t *ch_a, int8_t *ch_b, const char *buffer,
                      size_t size) {
    *ch_a = 0;
    *ch_b = 0;
    for (size_t i = 0; i < size; i++) {
        *ch_a = *ch_a + buffer[i];
        *ch_b = *ch_b + *ch_a;
    }
}

int gps_buffer_to_generic_ubx_message(gps_generic_ubx_message_t *message,
                                      const char buffer[GPS_MAX_LINE_SIZE]) {
    message->_class = buffer[0];
    message->_id = buffer[1];
    message->_size = buffer[2];
    message->_size += buffer[3] << 8;
    if (message->_size + 6 > GPS_MAX_LINE_SIZE)
    {
        printf("GPS Interface: exceding max size\n");
        return -1;
    }
    message->_ck_a = buffer[4 + message->_size];
    message->_ck_b = buffer[4 + message->_size + 1];
    return 0;
}

int gps_ubx_check_checksum(const char *buffer) {
    int8_t ch_a, ch_b;
    gps_generic_ubx_message_t generic_msg;
    if (gps_buffer_to_generic_ubx_message(&generic_msg, buffer) == -1)
        return -1;
    rfc1145_checksum(&ch_a, &ch_b, buffer, generic_msg._size + 4);
    if (ch_a == generic_msg._ck_a && ch_b == generic_msg._ck_b)
        return 0;
    return -1;
}

int gps_match_message(gps_protocol_and_message *match, const char *buffer,
                      gps_protocol_type protocol) {
    match->message = -1;
    if (protocol == GPS_PROTOCOL_TYPE_NMEA)
    {
        if (strncmp(buffer, "GGA", 3) == 0)
            match->message = GPS_NMEA_TYPE_GGA;
        else if (strncmp(buffer, "GSA", 3) == 0)
            match->message = GPS_NMEA_TYPE_GSA;
        else if (strncmp(buffer, "VTG", 3) == 0)
            match->message = GPS_NMEA_TYPE_VTG;
    }
    else if (protocol == GPS_PROTOCOL_TYPE_UBX)
    {
        if (buffer[0] != 1) { // match NAV message type
            match->message = GPS_UBX_TYPE_SIZE;
        }
        else
        {
            for (int i = 0; i < GPS_UBX_TYPE_SIZE; i++)
            {
                if (buffer[1] == gps_ubx_matches[i])
                {
                    match->message = i;
                    break;
                }
            }
        }
    }
    else
    {
        protocol = GPS_PROTOCOL_TYPE_SIZE;
    }
    match->protocol = protocol;
    if (match->message == -1)
        return -1;

    return 0;
}

void gps_parse_ubx_dop(gps_ubx_dop_t *data, uint8_t *buffer) {
    buffer += 4; // align to start of payload
    data->iTOW = *(uint32_t *)buffer;
    buffer += 4;
    data->gDOP = (*(uint16_t *)buffer) * 0.01f;
    buffer += 2;
    data->pDOP = (*(uint16_t *)buffer) * 0.01f;
    buffer += 2;
    data->tDOP = (*(uint16_t *)buffer) * 0.01f;
    buffer += 2;
    data->vDOP = (*(uint16_t *)buffer) * 0.01f;
    buffer += 2;
    data->hDOP = (*(uint16_t *)buffer) * 0.01f;
    buffer += 2;
    data->nDOP = (*(uint16_t *)buffer) * 0.01f;
    buffer += 2;
    data->eDOP = (*(uint16_t *)buffer) * 0.01f;
}

void gps_parse_ubx_pvt(gps_ubx_pvt_t *data, uint8_t *buffer) {
    buffer += 4; // align to start of payload
    data->iTOW = *(uint32_t *)buffer;
    buffer += 4;
    data->year = *(uint16_t *)buffer;
    buffer += 2;
    data->month = *(uint8_t *)buffer;
    buffer += 1;
    data->day = *(uint8_t *)buffer;
    buffer += 1;
    data->hour = *(uint8_t *)buffer;
    buffer += 1;
    data->min = *(uint8_t *)buffer;
    buffer += 1;
    data->sec = *(uint8_t *)buffer;
    buffer += 1;
    data->valid = *(uint8_t *)buffer;
    buffer += 1;
    data->tAcc = *(uint32_t *)buffer;
    buffer += 4;
    data->nano = *(int32_t *)buffer;
    buffer += 4;
    data->fixType = *(uint8_t *)buffer;
    buffer += 1;
    data->flags = *(uint8_t *)buffer;
    buffer += 1;
    data->flags2 = *(uint8_t *)buffer;
    buffer += 1;
    data->numSV = *(uint8_t *)buffer;
    buffer += 1;
    data->lon = *(int32_t *)buffer;
    buffer += 4;
    data->lat = *(int32_t *)buffer;
    buffer += 4;
    data->height = *(int32_t *)buffer;
    buffer += 4;
    data->hMSL = *(int32_t *)buffer;
    buffer += 4;
    data->hAcc = *(uint32_t *)buffer;
    buffer += 4;
    data->vAcc = *(uint32_t *)buffer;
    buffer += 4;
    data->velN = *(int32_t *)buffer;
    buffer += 4;
    data->velE = *(int32_t *)buffer;
    buffer += 4;
    data->velD = *(int32_t *)buffer;
    buffer += 4;
    data->gSpeed = *(int32_t *)buffer;
    buffer += 4;
    data->headMot = *(int32_t *)buffer;
    buffer += 4;
    data->sAcc = *(uint32_t *)buffer;
    buffer += 4;
    data->headAcc = *(uint32_t *)buffer;
    buffer += 4;
    data->pDOP = *(uint16_t *)buffer;
    buffer += 2;
    buffer += 6; // reserved
    data->headVeh = *(int32_t *)buffer;
    buffer += 4;
    data->magDec = *(int16_t *)buffer;
    buffer += 2;
    data->magAcc = *(uint16_t *)buffer;

    // conversions
    data->lat *= 1e-7;
    data->lon *= 1e-7;
    data->headMot *= 1e-5;
    data->headAcc *= 1e-5;
    data->pDOP *= 0.01;
    data->headVeh *= 1e-5;
    data->magDec *= 1e-2;
    data->magAcc *= 1e-2;
}

void gps_parse_ubx_hpposecef(gps_ubx_hpposecef_t *data, uint8_t *buffer) {
    buffer += 4; // align to start of payload
    data->version = *(uint8_t *)buffer;
    buffer += 1;
    buffer += 3; // 3 reserved bytes
    data->iTOW = *(uint32_t *)buffer;
    buffer += 4;
    data->ecefX = *(int32_t *)buffer;
    buffer += 4;
    data->ecefY = *(int32_t *)buffer;
    buffer += 4;
    data->ecefZ = *(int32_t *)buffer;
    buffer += 4;
    data->ecefXHp = *(int8_t *)buffer;
    buffer += 1;
    data->ecefYHp = *(int8_t *)buffer;
    buffer += 1;
    data->ecefZHp = *(int8_t *)buffer;
    buffer += 1;
    buffer += 1; // reserved
    data->pAcc = (*(uint32_t *)buffer) * 0.1;

    data->ecefX += data->ecefXHp * 1e-3;
    data->ecefY += data->ecefYHp * 1e-3;
    data->ecefZ += data->ecefZHp * 1e-3;
}

void gps_parse_ubx_hpposllh(gps_ubx_hpposllh_t *data, uint8_t *buffer) {
    buffer += 4;
    data->version = *(uint8_t *)buffer;
    buffer += 1;
    buffer += 3; // reserved
    data->iTOW = *(uint32_t *)buffer;
    buffer += 4;
    data->lon = *(int32_t *)buffer;
    buffer += 4;
    data->lat = *(int32_t *)buffer;
    buffer += 4;
    data->height = *(int32_t *)buffer;
    buffer += 4;
    data->hMSL = *(int32_t *)buffer;
    buffer += 4;
    data->lonHp = *(int8_t *)buffer;
    buffer += 1;
    data->latHp = *(int8_t *)buffer;
    buffer += 1;
    data->heightHp = *(int8_t *)buffer;
    buffer += 2;
    data->hMSLHp = *(int8_t *)buffer;
    buffer += 2;
    data->hAcc = *(uint32_t *)buffer;
    buffer += 4;
    data->vAcc = *(uint32_t *)buffer;

    // conversions
    data->lon = data->lon * 1e-7 + ((double)data->latHp * 1e-9);
    data->lat = data->lat * 1e-7 + ((double)data->lonHp * 1e-9);
    data->height += data->heightHp * 0.1f;
    data->hMSL += data->hMSLHp * 0.1f;
}


void gps_ubx_fields_dop(FILE *out) {
    fprintf(out, "_timestamp,iTOW,gDOP,pDOP,tDOP,vDOP,hDOP,nDOP,eDOP\n");
    fflush(out);
}

void gps_ubx_fields_pvt(FILE *out) {
    fprintf(out,
            "_timestamp,iTOW,year,month,day,hour,min,sec,valid,tAcc,nano,fixType,"
            "flags,flags2,numSV,lon,lat,height,hMSL,hAcc,vAcc,velN,velE,velD,"
            "gSpeed,headMot,sAcc,headAcc,pDOP,headVeh,magDec,magAcc\n");
    fflush(out);
}

void gps_ubx_fields_hpposecef(FILE *out) {
    fprintf(out, "_timestamp,version,iTOW,ecefX,ecefY,ecefZ,ecefXHp,ecefYHp,"
                 "ecefZHp,pAcc\n");
    fflush(out);
}

void gps_ubx_fields_hpposllh(FILE *out) {
    fprintf(out, "_timestamp,version,iTOW,lon,lat,height,hMSL,lonHp,latHp,"
                 "heightHp,hMSLHp,hAcc,vAcc\n");
    fflush(out);
}

void gps_ubx_value_to_file_dop(FILE *out, gps_ubx_dop_t *data) {
    fprintf(out,
            "%" PRIu64 ",%" PRIu32 ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%.9f"
            "\n",
            data->_timestamp, data->iTOW, data->gDOP, data->pDOP, data->tDOP,
            data->vDOP, data->hDOP, data->nDOP, data->eDOP);
    fflush(out);
}

void gps_ubx_value_to_file_pvt(FILE *out, gps_ubx_pvt_t *data) {
    fprintf(out,
            "%" PRIu64 ",%" PRIu32 ",%" PRIu16 ",%" PRIu8 ",%" PRIu8 ",%" PRIu8
            ",%" PRIu8 ",%" PRIu8 ",%" PRIu8 ",%" PRIu32 ",%" PRIi32 ",%" PRIu8
            ",%" PRIu8 ",%" PRIu8 ",%" PRIu8 ",%.9f"
            ",%.9f"
            ",%" PRIi32 ",%" PRIi32 ",%" PRIu32 ",%" PRIu32 ",%" PRIi32
            ",%" PRIi32 ",%" PRIi32 ",%" PRIi32 ",%.9f"
            ",%" PRIu32 ",%.9f"
            ",%" PRIu16 ",%.9f"
            ",%.9f"
            ",%.9f"
            "\n",
            data->_timestamp, data->iTOW, data->year, data->month, data->day,
            data->hour, data->min, data->sec, data->valid, data->tAcc, data->nano,
            data->fixType, data->flags, data->flags2, data->numSV, data->lon,
            data->lat, data->height, data->hMSL, data->hAcc, data->vAcc,
            data->velN, data->velE, data->velD, data->gSpeed, data->headMot,
            data->sAcc, data->headAcc, data->pDOP, data->headVeh, data->magDec,
            data->magAcc);
    fflush(out);
}

void gps_ubx_value_to_file_hpposecef(FILE *out, gps_ubx_hpposecef_t *data) {
    fprintf(out,
            "%" PRIu64 ",%" PRIu8 ",%" PRIu32 ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%" PRIi8 ",%" PRIi8 ",%" PRIi8 ",%" PRIu8 ",%.9f"
            "\n",
            data->_timestamp, data->version, data->iTOW, data->ecefX, data->ecefY,
            data->ecefZ, data->ecefXHp, data->ecefYHp, data->ecefZHp,
            data->reserved2, data->pAcc);
    fflush(out);
}

void gps_ubx_value_to_file_hpposllh(FILE *out, gps_ubx_hpposllh_t *data) {
    fprintf(out,
            "%" PRIu64 ",%" PRIu8 ",%" PRIu32 ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%.9f"
            ",%" PRIi8 ",%" PRIi8 ",%" PRIi8 ",%" PRIi8 ",%" PRIu32 ",%" PRIu32
            "\n",
            data->_timestamp, data->version, data->iTOW, data->lon, data->lat,
            data->height, data->hMSL, data->lonHp, data->latHp, data->heightHp,
            data->hMSLHp, data->hAcc, data->vAcc);
    fflush(out);
}

gps_parse_result_t gps_parse_buffer(gps_parsed_data_t *data,
                                    gps_protocol_and_message *match,
                                    const char *buffer, uint64_t timestamp) {

    if (match->protocol == GPS_PROTOCOL_TYPE_UBX)
    {
        if (gps_ubx_check_checksum(buffer) != 0)
            return GPS_PARSE_RESULT_CHECKSUM;
        switch (match->message)
        {
            case GPS_UBX_TYPE_NAV_DOP:
                data->dop._timestamp = timestamp;
                gps_parse_ubx_dop(&data->dop, (uint8_t *)buffer);
                break;
            case GPS_UBX_TYPE_NAV_PVT:
                data->pvt._timestamp = timestamp;
                gps_parse_ubx_pvt(&data->pvt, (uint8_t *)buffer);
                break;
            case GPS_UBX_TYPE_NAV_HPPOSECEF:
                data->hpposecef._timestamp = timestamp;
                gps_parse_ubx_hpposecef(&data->hpposecef, (uint8_t *)buffer);
                break;
            case GPS_UBX_TYPE_NAV_HPPOSLLH:
                data->hpposllh._timestamp = timestamp;
                gps_parse_ubx_hpposllh(&data->hpposllh, (uint8_t *)buffer);
                break;
            default:
                return GPS_PARSE_RESULT_NO_MATCH;
        }
    }
    else
    {
        return GPS_PARSE_RESULT_NO_MATCH;
    }
    return GPS_PARSE_RESULT_OK;
}

void gps_get_message_name(gps_protocol_and_message *match, char *buff) {
    switch (match->protocol) {
        case GPS_PROTOCOL_TYPE_NMEA:
            strcpy(buff, gps_nmea_message_type_string[match->message]);
            break;
        case GPS_PROTOCOL_TYPE_UBX:
            strcpy(buff, gps_ubx_message_type_string[match->message]);
            break;
        default:
            break;
    }
}

void gps_construct_filename(char *dest, const char *path, char *helper_buff,
                            gps_protocol_and_message *match) {
    memset(dest, 0, strlen(dest));
    memset(helper_buff, 0, strlen(helper_buff));
    gps_get_message_name(match, helper_buff);
    strcat(dest, path);
    strcat(dest, "/GPS_");
    strcat(dest, helper_buff);
    strcat(dest, ".csv");
}

void gps_open_files(gps_files_t *files, const char *path) {
    const int message_size = 20;
    const int filepath_size = 100;
    char message[message_size];
    char filepath[filepath_size];
    gps_protocol_and_message match;
    match.protocol = GPS_PROTOCOL_TYPE_NMEA;
    for (int i = 0; i < GPS_NMEA_TYPE_SIZE; i++) {
        match.message = i;
        gps_construct_filename(filepath, path, message, &match);
        files->nmea[i] = fopen(filepath, "w");
    }
    match.protocol = GPS_PROTOCOL_TYPE_UBX;
    for (int i = 0; i < GPS_UBX_TYPE_SIZE; i++) {
        match.message = i;
        gps_construct_filename(filepath, path, message, &match);
        files->ubx[i] = fopen(filepath, "w");
    }
}

void gps_close_files(gps_files_t *files) {
    for (int i = 0; i < GPS_NMEA_TYPE_SIZE; i++) {
        fclose(files->nmea[i]);
    }
    for (int i = 0; i < GPS_UBX_TYPE_SIZE; i++) {
        fclose(files->ubx[i]);
    }
}

void gps_to_file(gps_files_t *files, gps_parsed_data_t *data,
                 gps_protocol_and_message *match){
    switch (match->protocol)
    {
        case GPS_PROTOCOL_TYPE_UBX:
            switch (match->message)
            {
                case GPS_UBX_TYPE_NAV_DOP:
                    gps_ubx_value_to_file_dop(files->ubx[GPS_UBX_TYPE_NAV_DOP], &data->dop);
                    break;
                case GPS_UBX_TYPE_NAV_PVT:
                    gps_ubx_value_to_file_pvt(files->ubx[GPS_UBX_TYPE_NAV_PVT], &data->pvt);
                    break;
                case GPS_UBX_TYPE_NAV_HPPOSECEF:
                    gps_ubx_value_to_file_hpposecef(files->ubx[GPS_UBX_TYPE_NAV_HPPOSECEF],
                                                    &data->hpposecef);
                    break;
                case GPS_UBX_TYPE_NAV_HPPOSLLH:
                    gps_ubx_value_to_file_hpposllh(files->ubx[GPS_UBX_TYPE_NAV_HPPOSLLH],
                                                   &data->hpposllh);
                    break;
                default:
                    break;
            }
            break;
        
        default:
            break;
    }
}

void gps_header_to_file(gps_files_t *files) {
    // UBX
    gps_ubx_fields_dop(files->ubx[GPS_UBX_TYPE_NAV_DOP]);
    gps_ubx_fields_pvt(files->ubx[GPS_UBX_TYPE_NAV_PVT]);
    gps_ubx_fields_hpposecef(files->ubx[GPS_UBX_TYPE_NAV_HPPOSECEF]);
    gps_ubx_fields_hpposllh(files->ubx[GPS_UBX_TYPE_NAV_HPPOSLLH]);
}