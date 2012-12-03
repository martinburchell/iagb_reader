#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define HEADER_SIZE 0x20
#define RECORD1_SIZE 9
#define CATEGORY_SIZE 5
#define RECORD3_SIZE 70

static int vmc2csv_read_int32( const char *buffer )
{
    return (int)( buffer[0] + 
                  ( buffer[1] << 8 ) + 
                  ( buffer[2] << 16 ) + 
                  ( buffer[3] << 24 ));
}

static int vmc2csv_read_int16( const char *buffer )
{
    return (int)( buffer[0] + 
                  ( buffer[1] << 8 ));
}

static void vmc2csv_dump_record3( const char *record3,
                                  const char *feature_name )
{
    int symbol = vmc2csv_read_int16( record3 + 0x3d );
    int text_height = vmc2csv_read_int16( record3 + 0x16 );

    printf( "0x00-0x01: %.2x%.2x %s\n",
            record3[0x01],record3[0x00],
            feature_name );

    printf( "0x04-0x05: %.2x%.2x %s\n",
            record3[0x05],record3[0x04],
            feature_name );

    printf( "RGB fg: %.2x%.2x%.2x\n", 
            record3[0x06], record3[0x07], record3[0x08] );

    printf( "0x09: %.2x %s\n", record3[0x09],
            feature_name );

    printf( "Line style: %.2x%.2x\n", record3[0x0b],record3[0x0a] );

    printf( "Line width: %.2x%.2x\n", record3[0x0d],record3[0x0c] );

    printf( "0x0e-0x0f: %.2x%.2x %s\n",
            record3[0x0e],record3[0x0f],
            feature_name );

    printf( "RGB bg: %.2x%.2x%.2x\n", 
            record3[0x10], record3[0x11], record3[0x12] );

    printf( "0x13: %.2x %s\n", record3[0x13],
            feature_name );

    printf( "0x14-0x15: %.2x%.2x %s\n",
            record3[0x15],record3[0x14],
            feature_name );

    if ( text_height != 0 )
    {
        printf( "Text height: %.4x (%.1fmm)\n", 
                text_height, (float)(text_height)/3300 );
    }

    printf( "0x18-0x19: %.2x%.2x %s\n",
            record3[0x19],record3[0x18],
            feature_name );

    printf( "0x1a: %.2x %s\n", record3[0x1a], feature_name );
    printf( "0x1b: %.2x %s\n", record3[0x1b], feature_name );
    printf( "0x1c: %.2x %s\n", record3[0x1c], feature_name );

    if ( record3[0x1d] != '\0' )
    {
        printf( "Font: %s\n", record3 + 0x1d );
    }
     
    if ( symbol != 0 )
    {
        printf( "Symbol: %.4x ('%c')\n", symbol, record3[0x3d] );
    }

    printf( "0x3f-0x40: %.2x%.2x %s\n", 
            record3[0x40],record3[0x3f],
            feature_name );

    printf( "Symbol size: %.2x%.2x\n",
            record3[0x42],record3[0x41] );

    printf( "0x43-0x44: %.2x%.2x %s\n",
            record3[0x44],record3[0x43],
            feature_name );

    printf( "0x45: %.2x %s\n", record3[0x45], feature_name );
    
    printf( "\n" );
}

static char *vmc2csv_get_feature_name( FILE *fp, long pos )
{
    char *feature_name;

    int c;
    int i;
    int len = 0;

    fseek( fp, pos, SEEK_SET );

    while (( c = fgetc( fp )) != '\0' )
    {
        len ++;
    }

    feature_name = util_malloc( len + 1 );

    fseek( fp, pos, SEEK_SET );

    for ( i = 0; i <= len; i++ )
    {
        feature_name[i] = fgetc( fp );
    }

    return feature_name;
}

static int vmc2csv_read( FILE *fp )
{
    static char header[HEADER_SIZE];
    static char record1[RECORD1_SIZE];
    static char category[CATEGORY_SIZE];
    static char record3[RECORD3_SIZE];

    char *feature_names_data;
    char **feature_names;
    char *last_feature;

    int feature_names_start;
    int feature_names_len;
    int first_record1;
    int first_category;
    int first_record3;
    int i;
    int j;
    int n;
    int name_ptr;
    int num_non_empty_records;
    int num_features = 0;
    int num_records1;
    int num_categories;
    int num_records3;
    int offset;
    int pos;
    int rc = -1;
    int *record3_count;

    n = fread( header, HEADER_SIZE, 1, fp );

    if ( n < 1 )
    {
        fprintf( stderr, "Not a valid VMC file - header too short\n" );
        
        goto out;

    }
    
    if ( strncmp( header, "VMC", 3 ) != 0 )
    {
        fprintf( stderr, "Not a valid VMC file - no marker found\n" );
        
        goto out;
    }

    first_record1 = vmc2csv_read_int32( header + 0x06 );
    first_category = vmc2csv_read_int32( header + 0x0c );
    first_record3 = vmc2csv_read_int32( header + 0x12 );

    num_records1 = vmc2csv_read_int16( header + 0x0a );
    num_categories = vmc2csv_read_int16( header + 0x10 );
    num_records3 = vmc2csv_read_int16( header + 0x16 );

    feature_names_start = vmc2csv_read_int32( header + 0x18 );
    feature_names_len = vmc2csv_read_int32( header + 0x1c );

    printf( "First record (1) starts at %x\n", first_record1 );
    printf( "Number of records (1): %d\n", num_records1 );

    printf( "First category starts at %x\n", first_category );
    printf( "Number of categories: %d\n", num_categories );

    printf( "First record (3) starts at %x\n", first_record3 );
    printf( "Number of records (3): %d\n", num_records3 );

    record3_count = util_malloc( num_records3 * sizeof(int) );

    for ( i = 0; i < num_records3; i++ )
    {
        record3_count[i] = 0;
    }

    printf( "Feature names start at %x\n", feature_names_start );

    feature_names_data = util_malloc( feature_names_len );

    fseek( fp, feature_names_start, SEEK_SET );

    n = fread( feature_names_data, feature_names_len, 1, fp );
    if ( n < 1 )
    {
        fprintf( stderr, "Failed to read feature names data\n" );
        goto out;
    }

    pos = 0;

    for ( i=0; i<feature_names_len; i++ )
    {
        if ( feature_names_data[i] == '\0' )
            num_features++;
    }

    feature_names = util_malloc( sizeof(char *) * num_features );

    num_features = 0;
    last_feature = feature_names_data;

    for ( i=0; i<feature_names_len; i++ )
    {
        if ( feature_names_data[i] == '\0' )
        {
            offset = last_feature - feature_names_data;

            feature_names[num_features] = last_feature;

            printf( "%x %x %s\n", 
                    offset, feature_names_start+offset,
                    last_feature );

            last_feature = feature_names_data+i+1;
            num_features++;
        }
    }


    printf( "Total features: %d\n", num_features );

    fseek( fp, first_record1, SEEK_SET );


    printf( "\nRecords (1)\n" );

    num_non_empty_records = 0;

    for ( i=0; i< num_records1; i++ )
    {
        n = fread( record1, RECORD1_SIZE, 1, fp );

        if ( n < 1 )
        {
            fprintf( stderr, "Failed to read record\n" );
        
            goto out;
        }

        name_ptr = vmc2csv_read_int16( record1 + 0x01 );

        if ( name_ptr != 0xffff )
        {
            char *feature_name;
            int third_record_index;

            long current_pos = ftell( fp );

            feature_name = vmc2csv_get_feature_name( 
                fp,
                name_ptr + feature_names_start );

            printf( "\n-------------------------------------------------\n" );

            printf( "%s\n", feature_name );
            printf( "-------------------------------------------------\n" );

            printf( "Feature class: %x\n", i );

#if 0
            for ( j=0; j<RECORD1_SIZE; j++ )
            {
                printf( "%.2x ", record1[j] );
            }

            printf( "\nName: %.4x\n", name_ptr );
            
            printf( "Category: %.4x\n", 
                    vmc2csv_read_int16( record1 + 0x5 ));
#endif
            third_record_index = vmc2csv_read_int16( record1 + 0x7 );

            printf( "\nRecord 3 (%.4x):\n", third_record_index );

            do
            {
                assert( third_record_index < num_records3 );

                record3_count[ third_record_index ]++;

                fseek( fp, 
                       first_record3 + RECORD3_SIZE * third_record_index, 
                       SEEK_SET );

                n = fread( record3, RECORD3_SIZE, 1, fp );

                if ( n < 1 )
                {
                    fprintf( stderr, "Failed to read record 3\n" );
                    
                    goto out;
                }
             
                vmc2csv_dump_record3( record3, feature_name );

                third_record_index = vmc2csv_read_int16( record3 + 0x2 );

                if ( third_record_index != 0xffff )
                {
                    printf( "Links to record 3 (%.4x):\n",third_record_index );
                }
            } while ( third_record_index != 0xffff );

            num_non_empty_records ++;

            fseek( fp, current_pos, SEEK_SET );

            util_free( feature_name );
        }
    }

    printf( "%d/%d records non-empty\n", num_non_empty_records, num_records1 );

    printf( "\nCategories\n" );

    fseek( fp, first_category, SEEK_SET );

    num_non_empty_records = 0;

    offset = 0;

    for ( i=0; i< num_categories; i++ )
    {
        n = fread( category, CATEGORY_SIZE, 1, fp );

        if ( n < 1 )
        {
            fprintf( stderr, "Failed to read category\n" );
        
            goto out;
        }

        printf( "\nOffset: %x\nIndex: %x\n", offset, i );

        for ( j=0; j<CATEGORY_SIZE; j++ )
        {
            printf( "%.2x ", category[j] );
        }

        printf( "\n" );

        name_ptr = vmc2csv_read_int16( category + 0x01 );

        printf( "Category name offset : %x\n", name_ptr );

        if ( name_ptr != 0xffff )
        {
            int c;
            long current_pos = ftell( fp );

            fseek( fp, name_ptr + feature_names_start, SEEK_SET );
            
            printf( "Name:" );

            while (( c = fgetc( fp )) != '\0' )
            {
                printf( "%c", c );
            }

            printf( "\n" );

            fseek( fp, current_pos, SEEK_SET );

            num_non_empty_records ++;
        }

        offset += CATEGORY_SIZE;
    }

    printf( "%d/%d records non-empty\n", num_non_empty_records, num_categories );
    util_free( feature_names );
    util_free( feature_names_data );

#if 0
    printf( "\n\nAll record 3s\n" );

    fseek( fp, 
           first_record3, 
           SEEK_SET );

    for ( i=0; i<num_records3; i++ )
    { 
        printf( "Index: %.4x\n", i );

        n = fread( record3, RECORD3_SIZE, 1, fp );

        if ( n < 1 )
        {
            fprintf( stderr, "Failed to read record 3\n" );
        
            goto out;
        }
             
        vmc2csv_dump_record3( record3,"" );
    }
#endif

    printf( "Record 3 usage:\n" );
    for ( i = 0; i < num_records3; i++ )
    {
        printf( "%.4x: %d\n", i, record3_count[i] );
    }

    util_free( record3_count );

    rc = 0;

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

    rc = vmc2csv_read( in_fp );

    fclose( in_fp );

    return rc;
}
