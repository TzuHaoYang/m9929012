#ifndef CONNECTOR_WIFI_H_INCLUDED
#define CONNECTOR_WAN_H_INCLUDED

int connector_wifi_ssid_and_psk_push_ovsdb(const char *vap);

int connector_wifi_ssid_push_cms(const struct schema_Wifi_VIF_Config *vconf);
int connector_wifi_psk_push_cms(const struct schema_Wifi_VIF_Config *vconf);

#endif /* CONNECTOR_WIFI_H_INCLUDED */
