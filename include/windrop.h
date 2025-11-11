//
// 
//

#ifndef PACKETSNIFF_WINDROP_H
#define PACKETSNIFF_WINDROP_H
#include <windows.h>
#include "windivert.h"

#define NTOHL(x)(WinDivertHelperNtohl(x))
#define NTOHS(x)(WinDivertHelperNtohs(x))
#define HTONS(x)(WinDivertHelperHtons(x))
#define PACKET_SIZE 0XFFFF

typedef struct {
    HANDLE wDivert;
    BOOL alive;
    HANDLE hwnd;
} engine_param_t;

typedef struct {
    engine_param_t param_t;
    HANDLE main_thread;
} engine_t;
typedef struct {
    WINDIVERT_IPHDR ip;
    WINDIVERT_TCPHDR tcp;
} tcp_packet_t;

void InitPacket(tcp_packet_t* tcp_packet) {

    tcp_packet->ip.Version = 4; // ipv4
    tcp_packet->ip.HdrLength = 5;
    tcp_packet->ip.Length = HTONS(sizeof(tcp_packet_t));
    tcp_packet->ip.Protocol = IPPROTO_TCP;
    tcp_packet->ip.TTL = 64;

    tcp_packet->tcp.HdrLength = 5;
    tcp_packet->tcp.Ack = 1;
    tcp_packet->tcp.Rst = 1;
    tcp_packet->tcp.Window = HTONS(0);

}

BOOL SendTCPResetPacket(HANDLE wDivert,tcp_packet_t* tcp_packet,PWINDIVERT_IPHDR ipheader,PWINDIVERT_TCPHDR tcpheader,WINDIVERT_ADDRESS* address) {

    tcp_packet->ip.DstAddr = ipheader->SrcAddr;
    tcp_packet->ip.SrcAddr = ipheader->DstAddr;
    tcp_packet->tcp.DstPort = tcpheader->SrcPort;
    tcp_packet->tcp.SrcPort = tcpheader->DstPort;
    tcp_packet->tcp.SeqNum = tcpheader->AckNum;
    tcp_packet->tcp.AckNum = tcpheader->SeqNum + 1; // + 1 for RST Packet
    WinDivertHelperCalcChecksums((PVOID)tcp_packet,sizeof(tcp_packet_t),address,0);
    return WinDivertSend(wDivert,(PVOID)tcp_packet,sizeof(tcp_packet_t),NULL,address);
}
void StartEngine(engine_param_t* param) {
    tcp_packet_t tcp_packet = { 0 };
    InitPacket(&tcp_packet);
    DWORD timeout = 0;
    uint8_t packet_buffer[PACKET_SIZE];
    UINT total_received = 0;
    PWINDIVERT_IPHDR ipheader;
    PWINDIVERT_TCPHDR tcpheader;
    PWINDIVERT_UDPHDR udpheader;
    WINDIVERT_ADDRESS address;
    char dst_ip[40];
    size_t dst_ip_len = sizeof(dst_ip);
    
    while (param->alive) {
        if (!WinDivertRecv(param->wDivert,packet_buffer,PACKET_SIZE,&total_received,&address)) {
            // Cannot received data
            continue;
        }
        if (!WinDivertHelperParsePacket(packet_buffer,total_received,&ipheader,NULL,NULL,NULL,NULL,&tcpheader,&udpheader,NULL,NULL,NULL,NULL)) {
            // cannot parse packet
            continue;
        }
        if (!WinDivertHelperFormatIPv4Address(NTOHL(ipheader->DstAddr),dst_ip,dst_ip_len)) {
            dst_ip[0] = '\0';  // printf cannot print buffer
        }
        switch (ipheader->Protocol) {
            case IPPROTO_TCP:
                if (!SendTCPResetPacket(param->wDivert,&tcp_packet,ipheader,tcpheader,&address)) {
                    sMessageBoxW(W("SEND ERROR"),W("SEND ERROR %lu"),GetLastError());
                }
            
                break; 
            case IPPROTO_UDP:
                break;
                //Since our goal is to drop, we don't do anything, but we can manipulate it if we want :)
        }
        Sleep(200); // so that the processor does not curse us :)
    }
}

void CloseEngine(engine_t* param) {
    if (param->param_t.alive && param->param_t.wDivert || param->param_t.wDivert != INVALID_HANDLE_VALUE) {
        param->param_t.alive = FALSE;
        WinDivertClose(param->param_t.wDivert);
    }
    CloseHandle(param->main_thread);
}
#endif //PACKETSNIFF_WINDROP_H