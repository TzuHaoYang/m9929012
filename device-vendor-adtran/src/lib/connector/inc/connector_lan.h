#ifndef CONNECTOR_LAN_H_INCLUDED
#define CONNECTOR_LAN_H_INCLUDED

int connector_lan_br_config_push_ovsdb(void);
int connector_lan_br_config_push_cms(const struct schema_Wifi_Inet_Config *inet);

#endif /* CONNECTOR_LAN_H_INCLUDED */
