#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "png_util/lab_png.h"
#include "png_util/crc.c"
#include "png_util/crc.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

size_t PNG_SIG[] = {2303741511, 218765834};
unsigned long crc_table2[256];
char *chunk_type[] = {"IHDR", "IDAT", "IEND"};

uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

int is_png(U32 *buf)
{
    for (int i = 0; i < 2; i++)
    {
        buf[i] = swap_uint32(buf[i]);
        if (buf[i] != PNG_SIG[i])
            return 0;
    }
    return 1;
}

int get_png_height(struct data_IHDR *buf)
{
    return swap_uint32(buf->height);
}
int get_png_width(struct data_IHDR *buf)
{
    return swap_uint32(buf->width);
}
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence)
{
    fseek(fp, offset, whence);
    fread(out, 1, DATA_IHDR_SIZE, fp);
    rewind(fp);
    return 0;
}

int cal_png_crc(FILE *fp, long offset, int whence)
{

    U32 length = 0;

    U8 *p_buffer;                     /* a buffer that contains some data to play with */
    U32 crc_val = 0; /* CRC value  */ /* return value for various routines   */

    fseek(fp, offset, whence);

    for (int i = 0; i < 3; i++)
    {
        U32 *length_p = malloc(sizeof(U32));
        fread(length_p, 1, sizeof(U32), fp);
        length = swap_uint32(*length_p);
        free(length_p);
        length = (length + 4);
        p_buffer = malloc(sizeof(U8) * length);

        fread(p_buffer, 1, length, fp);
        crc_val = crc(p_buffer, length);
        free(p_buffer);
        if (crc_val != crc_table2[i])
        {
            printf("%s chunk CRC error: computed %x, expected %lx\n", chunk_type[i], crc_val, crc_table2[i]);
        }

        fseek(fp, CHUNK_CRC_SIZE, SEEK_CUR);
    }

    return 1;
}

//PNG_SIG_SIZE, SEEK_SET
int get_png_crc(FILE *fp, long offset, int whence)
{

    U32 length = 0;
    U32 read_crc = 0;

    fseek(fp, offset, whence);

    for (int i = 0; i < 3; i++)
    {
        U32 *crc_p = malloc(sizeof(U32));
        U32 *length_p = malloc(sizeof(U32));
        U32 *type_p = malloc(sizeof(U32));
        fread(length_p, 1, sizeof(U32), fp);
        fseek(fp, sizeof(U32), SEEK_CUR);
        length = swap_uint32(*length_p);
        free(length_p);
        free(type_p);
        fseek(fp, length, SEEK_CUR);
        fread(crc_p, 1, sizeof(U32), fp);
        read_crc = swap_uint32(*crc_p);
        crc_table2[i] = read_crc;
        free(crc_p);
    }

    return cal_png_crc(fp, offset, whence);
}

int validate_png(char *png)
{
    data_IHDR_p IHDR;
    U32 *pngSig;

    IHDR = (data_IHDR_p)malloc(DATA_IHDR_SIZE);

    FILE *pngFile = fopen(png, "rb");

    if (pngFile == NULL)
    {
        //fprintf(stderr, "Couldn't open %s: %s\n", "uweng.png", strerror(errno));
        exit(1);
    }

    pngSig = (U32 *)calloc(1,PNG_SIG_SIZE);
    fread(pngSig, 1, PNG_SIG_SIZE, pngFile);
    
    // char *file_name;
    //const char ch = '/';
    // int is_path = 0;
    // if (strchr(png, ch) == NULL)
    // {
    //     file_name = png;
    //     is_path = 0;
    // }
    // else
    // {
    //     file_name = strrchr(png, ch);
    //     is_path = 1;
    // }
    int valid_png = is_png(pngSig);
    if (!valid_png)
    {
        //printf("%s: Not a PNG file\n", file_name + is_path);

        free(pngSig);
        free(IHDR);
        fclose(pngFile);
        return 0;
    }
    else
    {
        get_png_data_IHDR(IHDR, pngFile, 16, SEEK_SET);
        // int width = get_png_width(IHDR);
        // int height = get_png_height(IHDR);

        //printf("%s: %d x %d\n", file_name + is_path, width, height);

        get_png_crc(pngFile, PNG_SIG_SIZE, SEEK_SET);
        free(pngSig);
        free(IHDR);
        fclose(pngFile);
        return 1;
    }
}