#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pcap.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/types.h>
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <errno.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <regex.h>
#include <iostream>

/* returns packet id */#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pcap.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/types.h>
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <errno.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <regex.h>
#include <iostream>

/* returns packet id */
using namespace std;

char * condi;
char * D_A;
char * hostdata;
uint8_t *tcpdata;

static u_int32_t print_pkt (struct nfq_data *tb)
{
    /*
    uint8_t url[20]{0};  //fix size
    memcpy(url,(const char*)condi,20);
    */
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
    struct nfqnl_msg_packet_hw *hwph;
    u_int32_t mark,ifi;
    int ret;
    u_char *data;

    ph = nfq_get_msg_packet_hdr(tb);
    if (ph) {
        id = ntohl(ph->packet_id);
        printf("hw_protocol=0x%04x hook=%u id=%u ", ntohs(ph->hw_protocol), ph->hook, id);  //ph?
    }

    hwph = nfq_get_packet_hw(tb);
    if (hwph) {
        int i, hlen = ntohs(hwph->hw_addrlen);

        printf("hw_src_addr=");
        for (i = 0; i < hlen-1; i++)
            printf("%02x:", hwph->hw_addr[i]);
        printf("%02x ", hwph->hw_addr[hlen-1]);
    }

    mark = nfq_get_nfmark(tb);
    if (mark)
        printf("mark=%u ", mark);

    ifi = nfq_get_indev(tb);
    if (ifi)
        printf("indev=%u ", ifi);

    ifi = nfq_get_outdev(tb);
    if (ifi)
        printf("outdev=%u ", ifi);
    ifi = nfq_get_physindev(tb);
    if (ifi)
        printf("physindev=%u ", ifi);

    ifi = nfq_get_physoutdev(tb);
    if (ifi)
        printf("physoutdev=%u ", ifi);

    ret = nfq_get_payload(tb, &data);
    if (ret >= 0)
    {
                printf("첫번째 통과!!``````````````` \n");
                struct iphdr *ipp;
                ipp=(struct iphdr*)data;

                if(ipp->protocol == IPPROTO_TCP)
                {
                    printf("두번째 통과!!```````````````\n");
                    data += sizeof(struct iphdr);
                    data += sizeof(struct tcphdr);

                    for(; ret>0; ret--)
                    {
                        uint32_t *host_start = (uint32_t *)data;
                        if(*host_start == ntohl(0x486f7374))
                        {

                            for(; ret>0; ret--)
                            {
                                uint16_t *host_fin = (uint16_t *)data;
                                printf("%c", *data);

                                data++;
                                if(*host_fin == ntohs(0x0d0a))
                                {
                                    printf("\n");
                                    break;
                                }
                            }
                        }
                        else
                            data++;
                    }
           }
           printf("payload_len=%d ", ret);
           fputc('\n', stdout);
    }
    return id;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    u_int32_t id = print_pkt(nfa);
    printf("Entering callback\n");
    uint32_t *Drop_or_Acpt = (uint32_t*)D_A;

    if(*Drop_or_Acpt==ntohl(0x64726f70))
            return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
    else if(*Drop_or_Acpt==ntohl(0x61637074))
            return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char *argv[])
{
    if(argc!=3)
    {
        printf(" 사용법 : <Write URL> <drop or acpt> \n");
        return 0;
    }
    condi = argv[1];
    D_A = argv[2];
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    u_char *data;
    int fd;
    int rv;
    char buf[4096] __attribute__ ((aligned));



    printf("opening library handle\n");
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("binding this socket to queue '0'\n");
    qh = nfq_create_queue(h, 0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
        exit(1);
    }

    fd = nfq_fd(h);

    for (;;) {
        if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
            printf("pkt received \n");
            nfq_handle_packet(h, buf, rv); //패킷받는곳
            continue;
        }

        if (rv < 0 && errno == ENOBUFS) {
            printf("losing packets!\n");
            continue;
        }
        perror("recv failed");
        break;
    }

    printf("unbinding from queue 0\n");
    nfq_destroy_queue(qh);

    printf("closing library handle\n");
    nfq_close(h);

    exit(0);
}

using namespace std;

char * condi;
char * D_A;
char * hostdata;
uint8_t *tcpdata;

static u_int32_t print_pkt (struct nfq_data *tb)
{
    /*
    uint8_t url[20]{0};  //fix size
    memcpy(url,(const char*)condi,20);
    */
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
    struct nfqnl_msg_packet_hw *hwph;
    u_int32_t mark,ifi;
    int ret;
    u_char *data;

    ph = nfq_get_msg_packet_hdr(tb);
    if (ph) {
        id = ntohl(ph->packet_id);
        printf("hw_protocol=0x%04x hook=%u id=%u ", ntohs(ph->hw_protocol), ph->hook, id);  //ph?
    }

    hwph = nfq_get_packet_hw(tb);
    if (hwph) {
        int i, hlen = ntohs(hwph->hw_addrlen);

        printf("hw_src_addr=");
        for (i = 0; i < hlen-1; i++)
            printf("%02x:", hwph->hw_addr[i]);
        printf("%02x ", hwph->hw_addr[hlen-1]);
    }

    mark = nfq_get_nfmark(tb);
    if (mark)
        printf("mark=%u ", mark);

    ifi = nfq_get_indev(tb);
    if (ifi)
        printf("indev=%u ", ifi);

    ifi = nfq_get_outdev(tb);
    if (ifi)
        printf("outdev=%u ", ifi);
    ifi = nfq_get_physindev(tb);
    if (ifi)
        printf("physindev=%u ", ifi);

    ifi = nfq_get_physoutdev(tb);
    if (ifi)
        printf("physoutdev=%u ", ifi);

    ret = nfq_get_payload(tb, &data);
    if (ret >= 0)
    {
                printf("첫번째 통과!!``````````````` \n");
                struct iphdr *ipp;
                ipp=(struct iphdr*)data;

                if(ipp->protocol == IPPROTO_TCP)
                {
                    printf("두번째 통과!!```````````````\n");
                    data += sizeof(struct iphdr);
                    data += sizeof(struct tcphdr);

                    for(; ret>0; ret--)
                    {
                        uint32_t *host_start = (uint32_t *)data;
                        if(*host_start == ntohl(0x486f7374))
                        {

                            for(; ret>0; ret--)
                            {
                                uint16_t *host_fin = (uint16_t *)data;
                                printf("%c", *data);

                                data++;
                                if(*host_fin == ntohs(0x0d0a))
                                {
                                    printf("\n");
                                    break;
                                }
                            }
                        }
                        else
                            data++;
                    }
                }
                //printf("AAAAAAAAAAAAAAAAAAAAAAAA%s \n",box);
           printf("payload_len=%d ", ret);
           fputc('\n', stdout);
    }
    return id;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    u_int32_t id = print_pkt(nfa);
    printf("Entering callback\n");
    uint32_t *Drop_or_Acpt = (uint32_t*)D_A;

    if(*Drop_or_Acpt==ntohl(0x64726f70))
            return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
    else if(*Drop_or_Acpt==ntohl(0x61637074))
            return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char *argv[])
{
    if(argc!=3)
    {
        printf(" 사용법 : <Write URL> <drop or acpt> \n");
        return 0;
    }
    condi = argv[1];
    D_A = argv[2];
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    u_char *data;
    int fd;
    int rv;
    char buf[4096] __attribute__ ((aligned));



    printf("opening library handle\n");
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("binding this socket to queue '0'\n");
    qh = nfq_create_queue(h, 0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
        exit(1);
    }

    fd = nfq_fd(h);

    for (;;) {
        if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
            printf("pkt received \n");
            nfq_handle_packet(h, buf, rv); //패킷받는곳
            continue;
        }

        if (rv < 0 && errno == ENOBUFS) {
            printf("losing packets!\n");
            continue;
        }
        perror("recv failed");
        break;
    }

    printf("unbinding from queue 0\n");
    nfq_destroy_queue(qh);

    printf("closing library handle\n");
    nfq_close(h);

    exit(0);
}
