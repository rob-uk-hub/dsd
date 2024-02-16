
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
    char destinationMessage[50];
    if(isGroupCall) sprintf(destinationMessage, "%d-GROUP", dest);
    else sprintf(destinationMessage, "%d", dest);
    // TODO - desination should be escaped for json
    // Source/dest do not seem to be covered by a checksum - TODO - should they be?
    snprintf(msg, 8191, "decoder:dsd_dmr\nreceived:%s\ngenerated:%s\nsrc:%d\ndest:%s\nlatitude:%f\nlongitude:%f\nknots:%f\ndegrees:%f\nraw:%s", 
        received_date_time, generated_date_time, source, destinationMessage, lat, lon, knots, bearing, sentence);
    mqtt_send_position(opts, msg, strlen(msg));

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

    fprintf(stderr, "Not read\n");
}
