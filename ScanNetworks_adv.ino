/*
 Advanced dual channel WiFi Scan example for Ameba 

 This example scans for available Wifi networks using the build in functionality.
 Every 3 seconds, it scans again. 
 It doesn't actually connect to any network.

 based on the work of https://gist.github.com/EstebanFuentealba/3da9ccecefa7e1b44d84e7cfaad2f35f
 */

const byte numChars = 32;
char receivedChars[numChars];

boolean newData = false;

#include <WiFi.h>
#include <wifi_conf.h>

static uint8_t _networkCount;
static char _networkSsid[WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH];
static int32_t _networkRssi[WL_NETWORKS_LIST_MAXNUM];
static uint32_t _networkEncr[WL_NETWORKS_LIST_MAXNUM];
static uint8_t _networkChannel[WL_NETWORKS_LIST_MAXNUM];
static uint8_t _networkBand[WL_NETWORKS_LIST_MAXNUM];
static char _networkMac[WL_NETWORKS_LIST_MAXNUM][18];

static rtw_result_t wifidrv_scan_result_handler(rtw_scan_handler_result_t *malloced_scan_result) {
  rtw_scan_result_t *record;

  if (malloced_scan_result->scan_complete != RTW_TRUE) {
    record = &malloced_scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

    if (_networkCount < WL_NETWORKS_LIST_MAXNUM) {
      strcpy(_networkSsid[_networkCount], (char *)record->SSID.val);
      _networkRssi[_networkCount] = record->signal_strength;
      _networkEncr[_networkCount] = record->security;
      _networkChannel[_networkCount] = record->channel;
      _networkBand[_networkCount] = record->band;
      sprintf(_networkMac[_networkCount], "%02X:%02X:%02X:%02X:%02X:%02X",
              record->BSSID.octet[0], record->BSSID.octet[1], record->BSSID.octet[2],
              record->BSSID.octet[3], record->BSSID.octet[4], record->BSSID.octet[5]);
      _networkCount++;
    }
  }
  return RTW_SUCCESS;
}

String getEncryptionTypeEx(uint32_t thisType) {
  switch (thisType) {
    case RTW_SECURITY_OPEN:
    case RTW_SECURITY_WPS_OPEN:
      return "Open";
    case RTW_SECURITY_WEP_PSK:
    case RTW_SECURITY_WEP_SHARED:
      return "WEP";
    case RTW_SECURITY_WPA_TKIP_PSK:
    case RTW_SECURITY_WPA_AES_PSK:
    case RTW_SECURITY_WPA_MIXED_PSK:
      return "WPA PSK";
    case RTW_SECURITY_WPA2_AES_PSK:
    case RTW_SECURITY_WPA2_TKIP_PSK:
    case RTW_SECURITY_WPA2_MIXED_PSK:
    case RTW_SECURITY_WPA2_AES_CMAC:  //Might be incorrect because I'm a crypto noob
      return "WPA2 PSK";
    case RTW_SECURITY_WPA_WPA2_TKIP_PSK:
    case RTW_SECURITY_WPA_WPA2_AES_PSK:
    case RTW_SECURITY_WPA_WPA2_MIXED_PSK:
      return "WPA/WPA2 PSK";
    case RTW_SECURITY_WPA_TKIP_ENTERPRISE:
    case RTW_SECURITY_WPA_AES_ENTERPRISE:
    case RTW_SECURITY_WPA_MIXED_ENTERPRISE:
      return "WPA Enterprise";
    case RTW_SECURITY_WPA2_TKIP_ENTERPRISE:
    case RTW_SECURITY_WPA2_AES_ENTERPRISE:
    case RTW_SECURITY_WPA2_MIXED_ENTERPRISE:
      return "WPA2 Enterprise";
    case RTW_SECURITY_WPA_WPA2_TKIP_ENTERPRISE:
    case RTW_SECURITY_WPA_WPA2_AES_ENTERPRISE:
    case RTW_SECURITY_WPA_WPA2_MIXED_ENTERPRISE:
      return "WPA/WPA2 Enterprise";
    case RTW_SECURITY_WPA3_AES_PSK:
      return "WPA3 PSK";
    case RTW_SECURITY_WPA2_WPA3_MIXED:
      return "WPA2/WPA3 PSK";
    case RTW_SECURITY_WPS_SECURE:
    default:
      return "Unknown";
  }
}

static int8_t ATWS() {
  _networkCount = 0;
  if (wifi_scan_networks(wifidrv_scan_result_handler, NULL) != RTW_SUCCESS) {
    return WL_FAILURE;
  }
  return _networkCount;
}


void setup() {
  // Initialize Serial1 and wait for port to open:
  Serial1.begin(38400);
  while (!Serial1) {
    ;  // wait for Serial1 port to connect. Needed for native USB port only
  }
  // Initialize the onboard WiFi:
  WiFi.status();
}

void loop() {
  recvWithStartEndMarkers();
  if (newData) {
    if (strcmp(receivedChars, "ATWS") == 0) {
      ATWS();
    }
    if (strcmp(receivedChars, "AT") == 0) {
      Serial1.println("OK");
    }
    newData = false;
  }
  if (_networkCount > 0) {
    printNetworkList();
    _networkCount = 0;
  }
}

void printNetworkList() {
  for (int network = 0; network < _networkCount; network++) {
    Serial1.print("AP : ");
    Serial1.print(network + 1);
    Serial1.print(",");
    Serial1.print(_networkSsid[network]);
    Serial1.print(",");
    Serial1.print(_networkChannel[network]);
    Serial1.print(",");
    Serial1.print(getEncryptionTypeEx(_networkEncr[network]));
    Serial1.print(",");
    Serial1.print(_networkRssi[network]);
    Serial1.print(",");
    Serial1.print(_networkMac[network]);
    Serial1.println("");
  }
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = 'A';
  char endMarker = '\n';
  char rc;

  while (Serial1.available() > 0 && newData == false) {
    rc = Serial1.read();
    Serial1.print(rc);
    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0';  // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      recvInProgress = true;
    }
  }
}
