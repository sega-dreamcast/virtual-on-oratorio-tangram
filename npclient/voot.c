/*  voot.c

DESCRIPTION

    VOOT Netplay protocol parser and constructor. It uses file handles and
    memory structures.

CHANGELOG

    Sun Jan  6 19:14:48 PST 2002    Scott Robinson <scott_np@dsn.itgo.com>
        Added a changelog. I figure I should do some cleaning up so when I
        write a final version some day.

    Tue Jan 22 00:31:19 PST 2002    Scott Robinson <scott_np@dsn.itgo.com>
        Changed the recvfrom() to recv(). I think it made a difference in
        the win32 port.

    Thu Jan 24 00:51:03 PST 2002    Scott Robinson <scott_np@dsn.itgo.com>
        Added data and debug packet sending functions.

    Thu Jan 24 01:35:27 PST 2002    Scott Robinson <scott_np@dsn.itgo.com>
        Added full parsing of incoming packets for ensuring of packet
        availability and making sure we only get a certain # of pcakets.

*/

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <stdio.h>

#include "voot.h"

static bool voot_check_packet(voot_packet *packet, uint32 size)
{
    /* See if there is as much data as advertised. */
    return ntohs(packet->header.size) == size - sizeof(voot_packet_header);
}

uint32 voot_check_packet_advsize(voot_packet *packet, uint32 size)
{
    uint32 advsize;

    if (size < sizeof(voot_packet_header))
        return 0;

    return ((advsize = (sizeof(voot_packet_header) + ntohs(packet->header.size))) > size) ? size : advsize;
}

voot_packet* voot_parse_buffer(unsigned char *buffer, unsigned int buffer_size)
{
    voot_packet *packet;

    if (!voot_check_packet((voot_packet *) buffer, buffer_size))
        return NULL;

    packet = malloc(sizeof(voot_packet));
    memcpy(packet, buffer, buffer_size);

    return packet;
}

uint32 voot_socket_verify_packet(int32 socket)
{
    voot_packet packet;
    int32 packet_header_size;
    int32 full_packet_size;
    int32 predict_packet_size;

    /* First check if there is an actual full packet header. */
    packet_header_size = recv(socket, &packet, sizeof(voot_packet_header), MSG_PEEK);

    /* Make sure we received a full packet - cygwin tells us full size */
    if (packet_header_size < sizeof(voot_packet_header))
        return 0;

    /* Now confirm the full packet is waiting for us. */
    predict_packet_size = sizeof(voot_packet_header) + ntohs(packet.header.size);
    full_packet_size = recv(socket, &packet, predict_packet_size, MSG_PEEK);

    if (full_packet_size >= predict_packet_size)
        return predict_packet_size;
    else
        return 0;
}

voot_packet* voot_parse_socket(int32 socket)
{
    unsigned char data[BIUDP_SEGMENT_SIZE];
    int32 packet_size;
    int32 rx;

    /* Verify the packet existance and size. */
    packet_size = voot_socket_verify_packet(socket);

    /* If it's there, receive it. */
    if (packet_size)
        rx = recv(socket, data, packet_size, 0);
    else
        return NULL;

    return voot_parse_buffer(data, rx);
}    

int32 voot_send_packet(int32 socket, voot_packet *packet, uint32 size)
{
    if (socket < 0)
        return FALSE;
    else if (!voot_check_packet(packet, size))
        return FALSE;

    return send(socket, packet, size, 0);
}

int32 voot_send_command(int32 socket, uint8 command)
{
    int32 retval;
    voot_packet *packet;

    packet = malloc(sizeof(voot_packet));

    packet->header.type = VOOT_PACKET_TYPE_COMMAND;
    packet->header.size = htons(2);
    packet->buffer[0] = command;

    retval = voot_send_packet(socket, packet, voot_check_packet_advsize(packet, sizeof(voot_packet)));

    free(packet);
    
    return retval;
}

void voot_send_data(int32 socket, uint8 packet_type, uint8 *data, uint32 data_size)
{
    voot_packet *packet;

    packet = malloc(sizeof(voot_packet));

    packet->header.type = packet_type;
    packet->header.size = htons(data_size);
    memcpy(packet->buffer, data, data_size);

    voot_send_packet(socket, packet, voot_check_packet_advsize(packet, sizeof(voot_packet)));

    free(packet);
}
