#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chp2ppmlib.h"

int main( int argc, char *argv[] )
{
    const char *error;
    int rc = 0;

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

    error = chp2ppmlib_process( in_fp, out_fp );
    if (error != NULL)
    {
        fprintf(stderr, "%s\n", error);
        rc = -1;
    }

    fclose( in_fp );
    fclose( out_fp );

    return rc;
}
