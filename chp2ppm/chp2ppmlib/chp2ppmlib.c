#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 0x740
#define PALETTE_OFFSET 0x20
#define TILE_OFFSET (PALETTE_OFFSET + 0x100 * 4)

static unsigned char g_header[HEADER_SIZE];

static unsigned int chp2ppmlib_read_int32( unsigned char *buffer )
{
    return (unsigned int)( (unsigned int)(buffer[0]) + 
                           ((unsigned int)(buffer[1]) << 8) + 
                           ((unsigned int)(buffer[2]) << 16) + 
                           ((unsigned int)(buffer[3]) << 24));
}

static void chp2ppmlib_write_line(FILE *out_fp, const char *line)
{
    fprintf(out_fp, "%s%c", line, 0x0a);
}

extern int chp2ppmlib_process( FILE *in_fp,
                            FILE *out_fp )
{
    int rc = -1;
    unsigned int num_read;
    unsigned int num_tiles_x;
    unsigned int num_tiles_y;
    unsigned int padded_tile_width;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int tile_size;
    unsigned int tile_y;

    char tile_metadata[100];

    num_read = fread( g_header, HEADER_SIZE, 1, in_fp );
    if ( num_read < 1 )
    {
        fprintf( stderr, "Not a valid CHP file - header too short\n" );

        goto out;
    }

    if ( strncmp( (const char *)g_header, "PCv", 3 ) != 0 )
    {
        fprintf( stderr, "Not a valid CHP file - no marker found\n" );

        goto out;
    }

    num_tiles_x = chp2ppmlib_read_int32( g_header + 0x08 );
    num_tiles_y = chp2ppmlib_read_int32( g_header + 0x0c );

    tile_width = chp2ppmlib_read_int32( g_header + 0x10 );
    tile_height = chp2ppmlib_read_int32( g_header + 0x14 );

    tile_size = chp2ppmlib_read_int32( g_header + 0x18 );

    /* round up to nearest 16 */
    padded_tile_width = (tile_width + 16 - 1) & ~(16 - 1); 

    chp2ppmlib_write_line(out_fp, "P6");
    chp2ppmlib_write_line(out_fp, "# CREATOR: CHP2PPM");

    sprintf(tile_metadata, "%d %d",
            num_tiles_x * tile_width, num_tiles_y * tile_height);
    chp2ppmlib_write_line(out_fp, tile_metadata); 
    chp2ppmlib_write_line(out_fp, "255");
                       

    printf("Tile width: %u\n", tile_width);
    printf("Tile height: %u\n", tile_height);
    printf("Tile size: %u\n", tile_size);
    printf("Tiles %u x %u\n", num_tiles_x, num_tiles_y);

    for ( tile_y = 0; tile_y < num_tiles_y; tile_y++ )
    {
        int row;

        for ( row = tile_height-1; row >= 0; row-- )
        {
            int tile_index;
            unsigned int tile_x;

            tile_index = tile_y * num_tiles_x;

            for ( tile_x = 0; tile_x < num_tiles_x; tile_x++ )
            {
                unsigned int column;
                unsigned int tile_start;

                tile_start = chp2ppmlib_read_int32( g_header + TILE_OFFSET + tile_index * 8 );
                fseek( in_fp, tile_start + row * padded_tile_width, SEEK_SET );

                for ( column = 0; column < tile_width; column++ )
                {
                    int colour = fgetc( in_fp );
                    const unsigned char *rgb = g_header + PALETTE_OFFSET + colour * 4;
                    int blue = *( rgb );
                    int green = *( rgb + 1 );
                    int red = *( rgb + 2 );

                    fputc( red, out_fp );
                    fputc( green, out_fp );
                    fputc( blue, out_fp );
                }

                tile_index ++;
            }
        }
    }

    rc = 0;

out:
    return rc;
}
