/*  voot.c

    VOOT netplay protocol debug implementation.

*/

#include "vars.h"
#include "biudp.h"
#include "search.h"
#include "voot.h"

static void dump_framebuffer_udp(void)
{
    uint32 index;
    uint16 *vram_start;

    #define UPSCALE_5_STYLE(bits)   (((bits) << 3) | ((bits) >> 2))
    #define UPSCALE_6_STYLE(bits)   (((bits) << 2) | ((bits) >> 4))

    #define RED_565_TO_INT(color)   UPSCALE_5_STYLE((color) & 0x1F)
    #define GREEN_565_TO_INT(color) UPSCALE_6_STYLE(((color) >> 5) & 0x3F)
    #define BLUE_565_TO_INT(color)  UPSCALE_5_STYLE(((color) >> 11) & 0x1F)

    biudp_write_str("[UBC] Dumping screenshot.\r\n#");
    vram_start = (uint16 *) (0xa5000000 + *((volatile unsigned int *)0xa05f8050));    /* Buffer start ? */

    #define MAP_NUM_PIXELS  (640 * 480)
    #define STRIP_SIZE      300

    for (index = 0; index <= MAP_NUM_PIXELS; index++)
    {
        uint8 strip[STRIP_SIZE][3];
                
        strip[index % STRIP_SIZE][0] = RED_565_TO_INT(vram_start[index]);
        strip[index % STRIP_SIZE][1] = GREEN_565_TO_INT(vram_start[index]);
        strip[index % STRIP_SIZE][2] = BLUE_565_TO_INT(vram_start[index]);

        if ((index % STRIP_SIZE) == STRIP_SIZE - 1)
            biudp_write_buffer((uint8 *) strip, sizeof(strip));
    }

    biudp_write_str("#\r\n[UBC] Done with screenshot.\r\n");
}

static void grep_memory(const char *string)
{
    volatile uint8 *mem_loc;

    biudp_write_str("[UBC] Grepping memory for '");
    biudp_write_str(string);
    biudp_write_str("' ...\r\n");

    mem_loc = search_sysmem(string, strlen(string));

    if (mem_loc)
    {
        biudp_write_str("[UBC] Match @ 0x");
        biudp_write_hex((uint32) mem_loc);
        biudp_write_str("\r\n[UBC] Grep done!\r\n");
    }
}

static void maybe_respond_command(uint8 maybe_command)
{
    volatile uint16 *player_a_health = (uint16 *) 0x8CCF6284;
    volatile uint16 *player_b_health = (uint16 *) 0x8CCF7402;

    switch (maybe_command)
    {
        /* STAGE: Reset health commands. */
        case '1':
            biudp_write_str("[UBC] Resetting player A health.\r\n");
            *player_a_health = 1200;
            break;

        case '2':
            biudp_write_str("[UBC] Resetting player B health.\r\n");
            *player_b_health = 1200;
            break;

        /* STAGE: Do we take a screenshot? */
        case '3':
            dump_framebuffer_udp();
            break;

        /* STAGE: Do we search for the binary header location? */
        case '4':
            biudp_write_str("[UBC] Calling search functions.\r\n");
            grep_memory("fieldtst.bin");
            grep_memory("1E3F3G3H3I3J3K3L3M3N3O3P3Q3R3S3T3U3V3W3X3");
            grep_memory("DIGITALMEDIA");
            break;

        /* STAGE: Dump 16mb of system memory. */
        case '5':
            biudp_write_buffer((const uint8 *) SYS_MEM_START, SYS_MEM_END - SYS_MEM_START);
            break;

        default:
            break;
    }
}

void voot_handle_packet(ether_info_packet_t *frame, udp_header_t *udp, uint16 udp_data_length)
{
    uint8 *command;

    command = (uint8 *) udp + sizeof(udp_header_t);

    maybe_respond_command(*command);
}