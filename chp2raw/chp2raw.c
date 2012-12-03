#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define HEADER_SIZE 0x740
#define PALETTE_OFFSET 0x20
#define TILE_OFFSET (PALETTE_OFFSET + 0x100 * 4)

static char g_header[HEADER_SIZE];

static int chp2raw_read_int32( char *buffer )
{
    return (int)( buffer[0] + 
                  ( buffer[1] << 8 ) + 
                  ( buffer[2] << 16 ) + 
                  ( buffer[3] << 24 ));
}

static int chp2raw_process( FILE *in_fp,
                            FILE *out_fp )
{
    int rc = -1;
    int num_read;
    int num_tiles_x;
    int num_tiles_y;
    int padded_tile_width;
    int tile_width;
    int tile_height;
    int tile_size;
    int tile_y;

    num_read = fread( g_header, HEADER_SIZE, 1, in_fp );
    if ( num_read < 1 )
    {
        fprintf( stderr, "Not a valid CHP file - header too short\n" );

        goto out;
    }

    if ( strncmp( g_header, "PCv", 3 ) != 0 )
    {
        fprintf( stderr, "Not a valid CHP file - no marker found\n" );

        goto out;
    }

    num_tiles_x = chp2raw_read_int32( g_header + 0x08 );
    num_tiles_y = chp2raw_read_int32( g_header + 0x0c );

    tile_width = chp2raw_read_int32( g_header + 0x10 );
    tile_height = chp2raw_read_int32( g_header + 0x14 );

    tile_size = chp2raw_read_int32( g_header + 0x18 );

    /* round up to nearest 16 */
    padded_tile_width = (tile_width + 16 - 1) & ~(16 - 1); 

    for ( tile_y = 0; tile_y < num_tiles_y; tile_y++ )
    {
        int row;

        for ( row = tile_height-1; row >= 0; row-- )
        {
            int tile_index;
            int tile_x;

            tile_index = tile_y * num_tiles_x;

            for ( tile_x = 0; tile_x < num_tiles_x; tile_x++ )
            {
                int col;
                int tile_start;

                tile_start = chp2raw_read_int32( g_header + TILE_OFFSET + tile_index * 8 );
                fseek( in_fp, tile_start + row * padded_tile_width, SEEK_SET );

                for ( col = 0; col < tile_width; col++ )
                {
                    int c = fgetc( in_fp );
                    char *palette = g_header + PALETTE_OFFSET + c * 4;
                    int b = *( palette );
                    int g = *( palette + 1 );
                    int r = *( palette + 2 );

                    fputc( r, out_fp );
                    fputc( g, out_fp );
                    fputc( b, out_fp );
                }

                tile_index ++;
            }
        }
    }

    printf( "rawtoppm parameters %d %d\n", 
            num_tiles_x * tile_width, num_tiles_y * tile_height );

    rc = 0;

out:
    return rc;
}

int main( int argc, char *argv[] )
{
    int rc;

    FILE *in_fp;
    FILE *out_fp;

    if ( argc < 3 )
    {
        fprintf( stderr, "Usage: %s <file_in> <file_out>\n", argv[0] );
        exit( -1 );
    }

    in_fp = fopen( argv[1], "r" );

    if ( !in_fp )
    {
        fprintf( stderr,
                 "Unable to open file '%s' for read\n",
                 argv[1] );
        exit(-1);
    }

    out_fp = fopen( argv[2], "w" );

    if ( !out_fp )
    {
        fclose( in_fp );

        fprintf( stderr,
                 "Unable to open file '%s' for write\n",
                 argv[2] );
        exit(-1);
    }

    rc = chp2raw_process( in_fp, out_fp );

    fclose( in_fp );
    fclose( out_fp );

    return rc;
}
