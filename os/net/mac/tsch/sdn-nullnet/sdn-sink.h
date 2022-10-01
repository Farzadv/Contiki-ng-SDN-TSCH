

#ifndef _SDN_SINK_H_
#define _SDN_SINK_H_

void sdn_sink_init(void);
int is_registered(struct sdn_packet* p);
struct sdn_packet * create_confing(void);
struct sdn_packet * create_confing2(void);
struct sdn_packet * create_confing1(void);
struct sdn_packet * create_confing3(void);
struct sdn_packet * create_confing4(void);
struct sdn_packet * create_confing5(void);

#endif