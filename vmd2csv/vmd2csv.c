#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "util.h"

#define MAX_CODES 0x10000

#define VMT_HEADER_SIZE 0x20
#define VMT_RECORD_SIZE 0x7
#define VMD_HEADER_SIZE 0x1f

typedef enum
{
    state_NEXT_CODE,
    state_BYTE_POINTS,
    state_16BIT_POINTS
} vmd_state;

static char g_header[VMD_HEADER_SIZE];
static char *g_data;

static int g_length;
static int g_num_points;

static int g_x;
static int g_y;

static int g_x1;
static int g_y1;

static int g_scale;

static int vmd2csv_read_int32( char *buffer )
{
    return (int)( buffer[0] + 
                  ( buffer[1] << 8 ) + 
                  ( buffer[2] << 16 ) + 
                  ( buffer[3] << 24 ));
}

static short vmd2csv_read_int16( char *buffer )
{
    return (short)( buffer[0] + 
                  ( buffer[1] << 8 ));
}

static void vmd2csv_plot_geometry( void )
{
    int i;
    int points_plotted = 0;
    
    int old_plot_x = g_x, old_plot_y = g_y;

    vmd_state state = state_NEXT_CODE;

    assert( g_data );

    printf( "# Bounding box\n" );

    printf( "# %.8d, %.8d\n", g_x, g_y );
    printf( "# %.8d, %.8d\n", g_x1, g_y );
    printf( "# %.8d, %.8d\n", g_x1, g_y1 );
    printf( "# %.8d, %.8d\n", g_x, g_y1 );
    printf( "# %.8d, %.8d\n", g_x, g_y );

    for ( i = 0; i < g_length; i++ )
    {
        if ( i % 4 == 0 )
            printf( "\n# ");
        else
            printf( " " );

        printf( "%.2x", g_data[i] );
    }

    printf("\n");

    i = 0;

    while ( i < g_length )
    {
        int code;
        int num_bytes;

        BOOL skipping;
        
        switch ( state )
        {
        case state_NEXT_CODE:
            code = g_data[i];
            
            if (( code % 2 ) == 1 )
            {
                state = state_BYTE_POINTS;

                num_bytes = ( code + 1 ) / 2;
                
                printf( "# %d byte pairs\n", num_bytes / 2 );
            }
            else
            {
                state = state_16BIT_POINTS;
                
                if (( code % 4 ) == 2 )
                {
                    num_bytes = code + 2;
                    
                    printf( "# %d 16-bit pairs\n", num_bytes / 2 );

                    skipping = FALSE;
                } 
                else if (( code % 4 ) == 0 )
                {
                    num_bytes = code + 4;
                    
                    printf( "# Skipping %d 16-bit pairs\n", num_bytes / 2 );

                    skipping = TRUE;
                }
                else
                {
                    printf( "# UNEXPECTED CODE %.2x\n", code );
                }
                
            }
            i++;

            break;

        case state_BYTE_POINTS:
            if ( i + 2 <= g_length )
            {
                int plot_x, plot_y;
                int x_off;
                int y_off;

                x_off = (signed char)(g_data[i]);
                y_off = (signed char)(g_data[i + 1]);

                plot_x = old_plot_x + ( g_scale * x_off );
                plot_y = old_plot_y + ( g_scale * y_off );

                old_plot_x = plot_x;
                old_plot_y = plot_y;

                if (( plot_x < g_x ) || ( plot_x > g_x1 ) || 
                    ( plot_y < g_y ) || ( plot_y > g_y1 ))
                {
                    printf( "\n# OUT OF RANGE\n" );
                }
                                
                printf( "# %.2x %.2x\n",
                        g_data[i],
                        g_data[i+1] );
                
                points_plotted ++;
                
                printf( "%d, %d\n", plot_x, plot_y );
                
                i += 2;
                num_bytes -= 2;

                if ( num_bytes <= 0 )
                {
                    state = state_NEXT_CODE;
                }
            }
            else
            {
                printf("# These bytes left over:\n");
                
                for ( ; i < g_length; i++)
                {
                    printf( "# %.2x\n", g_data[i] );
                }
            }

            break;

        case state_16BIT_POINTS:
            if ( i + 4 <= g_length )
            {
                int plot_x, plot_y;
                int x_off;
                int y_off;

                x_off = vmd2csv_read_int16( g_data + i );
                y_off = vmd2csv_read_int16( g_data + i + 2 );

                plot_x = g_x + ( g_scale * x_off );
                plot_y = g_y + ( g_scale * y_off );

                old_plot_x = plot_x;
                old_plot_y = plot_y;

                if (( plot_x < g_x ) || ( plot_x > g_x1 ) || 
                    ( plot_y < g_y ) || ( plot_y > g_y1 ))
                {
                    printf( "\n# OUT OF RANGE\n" );
                }
                                
                printf( "# %.2x%.2x %.2x%.2x\n",
                        g_data[i],
                        g_data[i+1],
                        g_data[i+2],
                        g_data[i+3] );

                points_plotted ++;
                if ( skipping )
                    printf( "\n# %d, %d\n", plot_x, plot_y );
                else
                    printf( "%d, %d\n", plot_x, plot_y );
                    
                i += 4;
                num_bytes -= 4;

                if ( num_bytes <= 0 )
                {
                    state = state_NEXT_CODE;
                }
            }
            else
            {
                printf( "# These bytes left over:\n" );

                for ( ; i < g_length; i++)
                {
                    printf( "# %.2x\n", g_data[i] );
                }
            }
                    
            break;

        default:
            fprintf( stderr, "Unexpected state %d\n", state );
            break;
        }
    }
                    
    printf( "# Total points plotted: %d\n", points_plotted );

    if ( points_plotted != g_num_points )
        printf( "# Unexpected number of points (%d != %d)\n",
                points_plotted, g_num_points );
}

static int vmd2csv__process_data( FILE *vmt_fp, FILE *vmd_fp )
{
    static char vmt_header[VMT_HEADER_SIZE];
    static char vmt_record[VMT_RECORD_SIZE];

    int i;
    int n;
    int rc = -1;
    int vmd_start;
    int vmd_pos;

    int vmd_index;
    int vmt_index = 0;

    int codes[MAX_CODES];

    for ( i=0; i<MAX_CODES; i++ )
    {
        codes[i] = 0;
    }

    n = fread( vmt_header, VMT_HEADER_SIZE, 1, vmt_fp );

    if ( n < 1 )
    {
        fprintf( stderr, "Not a valid VMT file - header too short\n" );
        
        goto out;

    }
    
    if ( strncmp( vmt_header, "VMT", 3 ) != 0 )
    {
        fprintf( stderr, "Not a valid VMT file - no marker found\n" );
        
        goto out;
    }

    while (( n = fread( vmt_record, VMT_RECORD_SIZE, 1, vmt_fp )) == 1 )
    {
        int num_records = 0;
        int vmd_data;

        BOOL end_reached = FALSE;

        vmd_index = 0;
        vmt_index ++;

        vmd_start = vmd2csv_read_int32( vmt_record );
        if ( vmd_start == -1 )
        {
            printf( "# Ignoring record\n" );

            continue;
        }

        vmd_pos = vmd_start;

        printf("\n\n#----------------------------------\n");
        printf( "# Reading VMD from %x\n", vmd_start );

        vmd_data = vmd2csv_read_int16( vmt_record + 4 );
      
        printf( "# Data: %.2x (%d)\n", vmd_data, vmd_data );

        fseek( vmd_fp, vmd_start, SEEK_SET );

        while ( !end_reached && 
                ( n = fread( g_header, 
                             VMD_HEADER_SIZE, 1, vmd_fp )) == 1 )
        {
            int scale_code;
            short code;

            BOOL in_range;

            g_data = NULL;

            num_records ++;

            vmd_index ++;
            code = vmd2csv_read_int16( g_header );

            if ( code == 0x0ca0 || code == 0x1620 )
                in_range = TRUE;
            else
                in_range = FALSE;

            codes[code]++;

#if 0
            printf( "\n# Record %.4x at %x\n", code, vmd_pos );
#endif
            vmd_pos += VMD_HEADER_SIZE;

            g_length = vmd2csv_read_int32( g_header + 2 );

            if ( g_length > 0 )
            {
                g_data = util_malloc( g_length );

                n = fread( g_data, g_length, 1, vmd_fp );
                if ( n < 1 )
                {
                    fprintf( stderr, 
                             "Unexpected error reading VMD record length %d\n",
                             g_length );
                    goto out;
                }
                
                vmd_pos += g_length;

            }

#if 0
            printf( "# End: %x\n", vmd_pos );
#endif
            g_num_points = vmd2csv_read_int16( g_header + 6 );

            g_x = vmd2csv_read_int32( g_header + 14 );
            g_y = vmd2csv_read_int32( g_header + 18 );

            g_x1 = vmd2csv_read_int32( g_header + 22 );
            g_y1 = vmd2csv_read_int32( g_header + 26 );

            scale_code = g_header[0x1e];

            if ( scale_code > 20 )
            {
                printf( "# Unexpected scale\n" );

                g_scale = 1;
            }
            else
            {
                g_scale = 1 << ( 20 - scale_code );
            }


            printf( "\n# ----- NEW RECORD ----\n" );
            printf( "# VMD header:");
            for ( i=0; i<VMD_HEADER_SIZE; i++ )
            {
                if (( i % 4 ) == 0 )
                    printf( "\n# %.2x: ",i);
                
                printf( "%.2x ", g_header[i] );
            }
            
            printf( "\n" );

            if ( code != 5 )
            {
                printf( "# Data:%.2x Code:%.4x Class:%.2x\n# length:%.3d (VMT %d, VMD %d)\n", 
                        vmd_data, code, code / 64, g_length, vmt_index,vmd_index );
                printf( "# Num points:%d\n", g_num_points );
            }
            else
            {
                printf( "# End of record marker\n" );
            }
#if 0
            if ( in_range && 
                 ( x >= -7345077 && x <= -7049309 && 
                   y >= 59183449 && y <= 59301930 ))
                in_range = TRUE;
            else
                in_range = FALSE;
#endif

            if ( code == 0x0005 )
                end_reached = TRUE;
            else if (( code & 0x3 ) == 0x3 )
            {
                /* Text */
                printf( "# %.8d, %.8d\n", g_x, g_y );
                printf( "# '%s'\n", g_data );
            }
            else 
            {
                if ( g_data )
                {
                    if ( in_range )
                        vmd2csv_plot_geometry();
                }
                else
                {
                    printf( "# No geometry!\n" );
                }
            }

            if ( g_data )
                util_free( g_data );
        }
    }


    printf("# Summary of records:\n");
    for ( i=0; i<MAX_CODES; i++ )
    {
        if ( codes[i] > 0 )
            printf( "# %.4x:%d\n",i,codes[i] );
    }

    rc = 0;

out:

    return rc;
}

int main( int argc, char *argv[] )
{
    int rc;

    FILE *vmt_fp;
    FILE *vmd_fp;

    if ( argc < 3 )
    {
        fprintf( stderr, "Usage: %s <VMT file> <VMD file>\n", argv[0] );
        exit( -1 );
    }

    vmt_fp = fopen( argv[1],"r" );

    if ( !vmt_fp )
    {
        fprintf( stderr,
                 "Unable to open file '%s' for read\n",
                 argv[1] );
        exit( -1 );
    }

    vmd_fp = fopen( argv[2],"r" );

    if ( !vmd_fp )
    {
        fprintf( stderr,
                 "Unable to open file '%s' for read\n",
                 argv[2] );
        exit( -1 );
    }

    rc = vmd2csv__process_data( vmt_fp, vmd_fp );

    fclose( vmt_fp );
    fclose( vmd_fp );

    util_report_leaks();

    return rc;
}
