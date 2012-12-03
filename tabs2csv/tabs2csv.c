#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define FIELD_NAME_LEN 0x40
#define HEADER_SIZE 28
#define LOOKUP_SIZE 8

static int tabs2csv_read_int32( char *buffer )
{
    return (int)(buffer[0] + 
            ( buffer[1] << 8 ) + 
            ( buffer[2] << 16 ) + 
            ( buffer[3] << 24 ));
}

static int tabs2csv_read( FILE *fp )
{
    static char header[HEADER_SIZE];

    int n;
    int rc = -1;

    n = fread( header, HEADER_SIZE, 1, fp );

    if ( n < 1 )
    {
        fprintf( stderr, "Not a valid TABS file - header too short\n" );
    }
    else if ( strncmp( header, "TABS", 4 ) != 0 )
    {
        fprintf( stderr, "Not a valid TABS file - no marker found\n" );
    }
    else
    {
        int i;
        int num_rows;
        int num_cols;
        int first_field_name;
        int lookup_table_start;
        int data_start;
        int lookup_pos;

        /* ignore the next 4 bytes after the TABS */

        /* Number of rows */
        num_rows = tabs2csv_read_int32( header + 0x08 );

#if 0
        printf( "Number of rows: %d\n", num_rows );
#endif
        /* Number of columns */
        num_cols = tabs2csv_read_int32( header + 0x0c );
#if 0
        printf( "Number of columns: %d\n", num_cols );
#endif
        /* Offset of first field name */
        first_field_name = tabs2csv_read_int32( header + 0x10 );
#if 0
        printf( "First field: %x\n", first_field_name );
#endif
        /* Offset of lookup table */
        lookup_table_start = tabs2csv_read_int32( header + 0x14 );
#if 0
        printf( "Lookup table start: %x\n", lookup_table_start );
#endif
        /* Offset of data */
        data_start = tabs2csv_read_int32( header + 0x18 );
#if 0
        printf( "Data start: %x\n", data_start );
#endif
        fseek( fp, first_field_name, SEEK_SET );

        for ( i = 0; i < num_cols; i++ )
        {
            char field_name[FIELD_NAME_LEN];

            n = fread( field_name, FIELD_NAME_LEN, 1, fp );
            if ( n != 1 )
            {
                fprintf( stderr, "Unable to read field name %d\n", i );
                goto out;
            }
            
            if ( i == 0 )
                printf( "\"%s\"", field_name );
            else
                printf( ",\"%s\"", field_name );
        }

        printf( "\n" );

        lookup_pos = lookup_table_start;

        for ( i = 0; i < num_rows; i++ )
        {
            char *p;
            char *record_data;
            int j;
            int record_length;
            int record_offset;

            static char lookup_data[LOOKUP_SIZE];

            fseek( fp, lookup_pos, SEEK_SET );

            n = fread( lookup_data, LOOKUP_SIZE, 1, fp );
            if ( n != 1 )
            {
                fprintf( stderr, 
                         "Unable to read lookup table entry %d\n", i );

                goto out;
            }

            record_offset = tabs2csv_read_int32( lookup_data );
            record_length = tabs2csv_read_int32( lookup_data + 0x4 );
            
            record_data = (char *) util_malloc( record_length );
            
            fseek( fp, data_start + record_offset, SEEK_SET );
            
            n = fread( record_data, record_length, 1, fp );
            if ( n != 1 )
            {
                util_free( record_data );

                fprintf( stderr, 
                         "Unable to read record %d\n", i );

                goto out;
            }

            for ( p = record_data, j = 0; j < num_cols; j++ )
            {
                if ( j == 0 )
                    printf( "\"%s\"", p );
                else
                    printf( ",\"%s\"", p );

                p += strlen(p) + 1;
            }

            printf( "\n" );

            util_free( record_data );

            lookup_pos += 8;
        }

        rc = 0;
    }

out:
    return rc;
}

int main( int argc, char *argv[] )
{
    int rc;

    FILE *in_fp;

    if ( argc < 2 )
    {
        fprintf( stderr, "Usage: %s <file_in>\n", argv[0] );
        exit( -1 );
    }

    in_fp = fopen( argv[1],"r" );

    if (!in_fp)
    {
        fprintf(stderr,
                "Unable to open file '%s' for read\n",
                argv[1]);
        exit(-1);
    }

    rc = tabs2csv_read( in_fp );

    fclose( in_fp );

    return rc;
}
