#include "dsd.h"
#include "mqtt.h"
#include "decode_dmr_half_rate.h"
#include <time.h>

bool prefix(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

int indexOf(char* string, char search, int start) {

    char* e = strchr(string+start, search);
    if(e == NULL) {
        return -1;
    }
    int index = (int)(e - string);
    return index;
}

#define bytes_to_u16(MSB,LSB) (((unsigned int) ((unsigned char) MSB)) & 255)<<8 | (((unsigned char) LSB)&255) 

bool try_parse_message(dsd_opts * opts, char* bytes, int length, int source_id, int destination_id, bool destination_is_group) 
{
    // Message length => 2 bytes
    if(length < 2) {
        //  Length is missing
        return false;
    }

    int message_length = bytes_to_u16(bytes[0], bytes[1]);
    fprintf(stderr, "\nMessage length: %d\n", message_length);
    
    if(length < message_length + 8)
    {
        // Skip
        return false;
    }

    // TODO - what is in the 8 bytes?

    char* message = bytes + 2 + 8;
    char message_content[1024];
    // TODO - there must be a better way?    
    for(int i=0;i<message_length/2;i++) 
    {
        int si = i*2;
        if(i > 1023) break;
        char c = message[si];
        if(c == 0) break;
        snprintf(message_content+i, 2, "%c", c);
    }
    message_content[message_length/2] = 0;

    char received_date_time[25];
    time_t rawtime = time(NULL);
    struct tm *ptm = gmtime(&rawtime);
    snprintf(received_date_time, 25, "%04d-%02d-%02dT%02d:%02d:%02d.000Z", 
        ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour,
        ptm->tm_min, ptm->tm_sec);

    char msg[8192];
    // TODO - escaping??
    snprintf(msg, 8191, "decoder:dsd_dmr\nreceived:%s\nsrc:%d\ndest:%d\ndest_is_group:%d\nmessage:%s\n", 
        received_date_time, source_id, destination_id, destination_is_group, message_content);

    fprintf(stderr, "Sending: %s", msg);

    char sourceMessage[20];
    sprintf(sourceMessage, "%d", source_id);

    mqtt_send_message(opts, msg, strlen(msg), sourceMessage);

    return true;
}

uint32_t bytes_to_uint(unsigned char buffer2, unsigned char buffer1, unsigned char buffer0)
{
    return ((uint32_t)buffer2 << 16) | ((uint32_t)buffer1 << 8) | (uint32_t)buffer0;
}

bool try_parse_ip_data(dsd_opts * opts, int source, int dest, bool isGroupCall, char* bytes, int length) 
{
    // IP Header => 20 bytes
    if(length < 20) {
        // IP header is 20 bytes, so don't try
        return false;
    }

    // IP version - should be 0x04
    int ip_version = (bytes[0] & 0xF0) >> 4;
    if(ip_version != 0x04) return false;

    // Service type, must be 0x00
    if(bytes[1] != 0x00) return false;

    // IPv4 Protocol (8 bits), always 00010001
    if(bytes[9] != 0x11) return false;

    int src_id = bytes_to_uint(bytes[13], bytes[14], bytes[15]);

    // 12 - individual
    // 225 - group
    bool destination_is_group = bytes[16] == (char)225;

    int dest_id = bytes_to_uint(bytes[17], bytes[18], bytes[19]);


    char* bytes_after_ip_header = bytes+20;
    int length_after_ip_header = length-20;
    // UDP Header => 8 bytes
    if(length_after_ip_header < 8) {
        return false;
    }

    char* bytes_after_udp_header = bytes_after_ip_header+8;
    int length_after_udp_header = length_after_ip_header-8;

    return try_parse_message(opts, bytes_after_udp_header, length_after_udp_header, src_id, dest_id, destination_is_group);
}

bool try_read_gps(dsd_opts * opts, int source, int dest, bool isGroupCall, char* bytes, int length) {
    if (length < 12) return false;

    if (!(bytes[5] == 0x27 && bytes[6] == 0x7E && bytes[7] == 0x27 && bytes[8] == 0x7E)) return false;

    char sentence[length];
    sprintf(sentence, "%s", bytes + 9); 
    
    if(!prefix("$GPRMC,", sentence)) {
        return false;
    }
 
    int start = indexOf(sentence, '$', 0);
    int end = indexOf(sentence, '*', start+1);
    
    if (start == -1 || end == -1) return false;

    int xor = 0x00;
    for (int i = start + 1; i < end; i++)
    {
        xor ^= (int)sentence[i];
    }
    char actualChecksum[3];
    char expectedChecksum[3];
    sprintf(actualChecksum, "%02X", xor);
    snprintf(expectedChecksum, 3, "%s", sentence+end+1);
    bool checksumOk = strncmp(actualChecksum, expectedChecksum, 2) == 0;
    fprintf(stderr, "checksum exp: %s, act: %s, ok=%d", expectedChecksum, actualChecksum, checksumOk);

    int startDate = indexOf(sentence, ',', start);
    if(startDate <0) return false;
    int startStatus = indexOf(sentence, ',', startDate+1);
    if(startStatus <0) return false;

    char datetime[3];
    snprintf(datetime, 3, "%s", sentence + startDate+1);
    int hour = atoi(datetime);
    
    snprintf(datetime, 3, "%s", sentence + startDate+3);
    int min = atoi(datetime);

    snprintf(datetime, 3, "%s", sentence + startDate+5);
    float sec = atof(datetime);

    bool gpsValid = sentence[startStatus+1] == 'A';
    if(!gpsValid) return false;

    int startLat = indexOf(sentence, ',', startStatus+1);
    char deg[4];
    snprintf(deg, 3, "%s", sentence + startLat+1);
    int latDeg = atoi(deg);

    char posMin[8];
    snprintf(posMin, 8, "%s", sentence + startLat+3);
    float latMin = atof(posMin);

    int startLatDir = indexOf(sentence, ',', startLat+1);
    int latDirN = sentence[startLatDir+1] == 'N' ? 1 : -1;
    float lat = (latDeg + (latMin / 60)) * latDirN;

    // Longitude
    int startLon = indexOf(sentence, ',', startLatDir+1);
    snprintf(deg, 4, "%s", sentence + startLon+1);
    int lonDeg = atoi(deg);

    snprintf(posMin, 8, "%s", sentence + startLon+4);
    float lonMin = atof(posMin);

    int startLonDir = indexOf(sentence, ',', startLon+1);
    int lonDirE = sentence[startLonDir+1] == 'E' ? 1 : -1;
    float lon = (lonDeg + (lonMin / 60)) * lonDirE;

    // Speed
    int startKnots = indexOf(sentence, ',', startLonDir+1);
    int startBearing = indexOf(sentence, ',', startKnots+1);
    char value[10];
    snprintf(value, startBearing-startKnots-1, "%s", sentence);
    float knots = atof(value);

    // Bearing
    int startTime = indexOf(sentence, ',', startBearing+1);
    snprintf(value, startTime-startBearing-1, "%s", sentence);
    float bearing = atof(value);
    
    // Time
    int startMagVarDef = indexOf(sentence, ',', startTime+1);

    snprintf(datetime, 3, "%s", sentence + startTime+1);
    int dd = atoi(datetime);
    
    snprintf(datetime, 3, "%s", sentence + startTime+3);
    int mm = atoi(datetime);

    snprintf(datetime, 3, "%s", sentence + startTime+5);
    int yy = atoi(datetime);

    int startMagVarDir = indexOf(sentence, ',', startMagVarDef+1);

    char generated_date_time[25];
    // E.g. 2012-04-23T18:25:43.511Z
    // generated:2024-02-16T17:06:0.000Z

    snprintf(generated_date_time, 25, "20%02d-%02d-%02dT%02d:%02d:%06.3fZ", yy, mm, dd, hour, min, sec);

    char received_date_time[25];
    time_t rawtime = time(NULL);
    struct tm *ptm = gmtime(&rawtime);
    snprintf(received_date_time, 25, "%04d-%02d-%02dT%02d:%02d:%02d.000Z", 
        ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour,
        ptm->tm_min, ptm->tm_sec);

    char msg[8192];

    char sourceMessage[20];
    sprintf(sourceMessage, "%d", source);

    // Source/dest do not seem to be covered by a checksum - TODO - should they be?
    snprintf(msg, 8191, "decoder:dsd_dmr\nreceived:%s\ngenerated:%s\nsrc:%d\ndest:%d\ndest_is_group:%d\nlatitude:%f\nlongitude:%f\nknots:%f\ndegrees:%f\nraw:%s", 
        received_date_time, generated_date_time, source, dest, isGroupCall, lat, lon, knots, bearing, sentence);
    mqtt_send_position(opts, msg, strlen(msg), sourceMessage);

    return true;
}

void parse_dmr_half_rate(dsd_opts * opts, int source, int dest, bool isGroupCall, short* rate12DataWordBigEndian, int length) {
    int bytesLength = length*2;
    char bytes[bytesLength];

    for(int i = 0; i < length; i++)
    {
      bytes[i*2 + 0] = rate12DataWordBigEndian[i] & 0x00FF;
      bytes[i*2 + 1] = (rate12DataWordBigEndian[i] >> 8);
    }

    bool read = try_read_gps(opts, source, dest, isGroupCall, bytes, bytesLength);
    if(read) return;

    read = try_parse_ip_data(opts, source, dest, isGroupCall, bytes, bytesLength);
    if(read) return;

    fprintf(stderr, "Not read\n");
}
