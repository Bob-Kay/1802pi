#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "wiringPi.h"

#define DIR_SEPARATOR	'/'		// For unix/macos/ElfOS.  For msdos etc use '\'
#define VERSION_MAJOR	0		// Program version
#define VERSION_MINOR	0

// Pin assignments
// 	name	WiringPi	   BCM		GPIO	  1802 
#define nClock	29	//		21		40			 1
#define nClear	28	//		20		38			 3
#define TPA		25	//		26		37			34
#define TPB		24	//		19		35			33
#define SC0		27	//		16		36			 6
#define nMrd	26	//		12		32			 7
#define nMwr	23	//		13		33			35
#define N2		 0	//		17		11			17

#define MA0		22	//		 6		31			25
#define MA1		21	//		 5		29			26
#define MA2		30	//		 0		27			27
#define MA3		14	//		11		23			28
#define MA4		13	//		 9		21		 	29
#define MA5		12	//		10		19			30
#define MA6		 3	//		22		15			31
#define MA7		 2	//		27		13			32

#define DB0		16	//		15	 	10			15
#define DB1		 1	//		18	 	12			14 
#define DB2		 4	//		23		16			13
#define DB3		 5	//		24		18			12
#define DB4		 6	//		25		22			11
#define DB5		10	//		 8		24			10
#define DB6		11	//		 7		26			 9
#define DB7		31	//		 1		28			 8

#define Qout	15	//		14		10			 4
/*
		Vcc		 -			 -		 1			16
		Vss		 -			 -		 2			40
        Vdd		 -			 -		 9			20
*/

// Currently unassigned GPIO pins
#define X0		 8	//		 2		 3			 -
#define X1	 	 9	//	 	 3		 5			 -
#define X2		 7	//		 4		 7			 -

// Currently unassigned 1802 pins
#define nWait		//		 -		 - 			 2
#define SC1			//		 -		 -			 5
#define N0			//		 -		 -			19
#define N1			//		 -		 -			18
#define DMAI		//		 -		 -			38
#define DMAO		//		 -		 -			37
#define INT			//		 -		 -			36
#define EF1			//		 -		 -			24
#define EF2			//		 -		 -			23
#define EF3			//		 -		 -			22
#define EF4			//		 -		 -			21
#define XTAL		//		 -		 -			39



typedef unsigned char byte;
typedef unsigned char boolean;

// struct { time_t tv_sec, longtv_usec } timeval;

// Global variables
int clocks;								// number of clock ccycles done
int clocks_per_second = 1000;
boolean dump_flag;
uint16_t mem_addr;
int max_clocks = 2000;					// max clocks to perform
struct timespec timeCycle;				// Duration of a half clock cycle;
struct timespec timeMark;				// Time of last clock edge

byte elfram[65536];						// Elf memory

int asxhex(int c );						// ConvertASCII hex character to binary
void clockPulse( void );				// Send one clock cycle
void clockEdge( int dir );				// Send one half clock pulse
int exec_cycle( void );					// Execute one machine cucle (8 clocks)
void get_options( int argc, char **argv ); // Get command line args
void init_1802_pins( void );			// Initialize 1802 pins
int load_binary( const char *fname ); 	// Load a binary file
int load_from_command( const char *code );  // load from command line
int load_file( const char *fname );		// Load a data file
int load_hex( const char *fname );		// Load an Intel hex file
int next_byte( const char *p );			// Get bext ASCII hex byte from string
void ramdump( int addr, int range );	// Dump 1802 memory to screen
unsigned read_addr( void );				// Read 8 bits of address from address lines
void set_db_in( void );					// Set data bus pins to INPUT
void set_db_out( void );				// Set data bus pins to OUTPUT
void show_help( const char *name );		// Display help message
void show_version( const char *name );	// Display program version
void csync( void );

int main( int argc, char *argv[] )
{

    wiringPiSetup( );
    init_1802_pins( );

    get_options( argc, argv );

    if ( optind < argc )	// get code in command line tail
        load_from_command( argv[optind] );        
    timeCycle.tv_sec = 0;
    timeCycle.tv_nsec = 1000000000 / ( clocks_per_second * 2 );
    clock_gettime( CLOCK_MONOTONIC, &timeMark );	// Set starting time for clock timer

    // Send 32 clocks while in reset
    for ( int i = 0; i < 32; i++ )
        clockPulse( );

    for ( clocks = 0; clocks < max_clocks; clocks++ )
    {
        csync( );
        digitalWrite( nClock, HIGH );
        csync( );
        digitalWrite( nClock, LOW );
//        puts( "Tick!" );
    }    

    if ( dump_flag )
        ramdump( 0, 256 );

    exit( EXIT_SUCCESS );
}

// Convert one ASCII character to hex
int asxhex( int c )
{
    if ( c >= '0' && c <= '9' )
        return c - '0';
    if ( c >= 'A' && c <= 'F' )
        return c - 'A' + 10;
    if ( c >= 'a' && c <= 'f' )
        return c - 'a' + 10;
    return -1;
}

void clockEdge( int dir )
{
    csync( );
    digitalWrite( nClock, dir ? HIGH : LOW );
}

void clockPulse( void )
{
    clockEdge( HIGH );
    clockEdge( LOW );
}

// execute one 8-clock machine cycle
int exec_cycle( void )
{
//    uint mem_addr;
    // Wait for TPA to go high, latch address
    do
    {
        clockEdge( HIGH );
        clockEdge( LOW );
    } while ( digitalRead( TPA ) == LOW );
    // Read address lines and set high address
//    mem_addr = read_addr( ) << 8;
    return 0;
}

void get_options( int argc, char **argv )
{
    int option;
    boolean fHelp = FALSE;
    boolean fVersion = FALSE;
    const char *binname;

    // Strip any path information from filename
    if ( ( binname = strrchr( argv[0], DIR_SEPARATOR ) ) != NULL )
        binname++;
    else
        binname = argv[0];

    while ( ( option = getopt( argc, argv, "b:df:h:Hn:s:v" ) ) != EOF )
    {
        switch( option )
        {
            case 'b':
                load_binary( optarg );
                break;
            case 'd':
                dump_flag = TRUE;
                break;
            case 'f':
                load_file( optarg );
                break;
            case 'h':
                load_hex( optarg );
                break;
            case 'H':
                if ( !fHelp )
                {
                    show_help( binname );
                    fHelp = TRUE;
                }
                break;
            case 'n':
                max_clocks = atoi( optarg );
                if ( max_clocks < 1 )
                {
                    fputs( "Bad value for max clocks\n", stderr );
                    exit( EXIT_FAILURE );
                }
                break;
            case 'v':
                if ( !fVersion )
                    show_version( binname );
                fVersion = TRUE;
                break;
            case 's':
                clocks_per_second = atoi( optarg );
                printf( " Speed set to %d\n", clocks_per_second );
                break;
            default:
                abort( );
        }
    }

    if ( fHelp || fVersion )
        exit( EXIT_SUCCESS );
}

void init_1802_pins( void )
{
    pinMode( nClock, OUTPUT );
    pinMode( nClear, OUTPUT );
    pinMode( TPA, INPUT );
    pinMode( TPB, INPUT );
    pinMode( nMwr, OUTPUT );
    pinMode( nMrd, OUTPUT );
/*
    pinMode( MA0, INPUT );
    pinMode( MA1, INPUT );
    pinMode( MA2, INPUT );
    pinMode( MA3, INPUT );
    pinMode( MA4, INPUT );
    pinMode( MA5, INPUT );
    pinMode( MA6, INPUT );
    pinMode( MA7, INPUT );
*/
    // Data bus will normally be input, set to output when needed
    set_db_in( );
    
    clockEdge( LOW );				// Start clock low
    digitalWrite( nClear, LOW );	// Start 1802 in reset
}

int load_binary( const char *fname )
{
    FILE *fp;
    uint16_t addr;
    int c;

    if ( ( fp = fopen( fname, "rb" ) ) == NULL )
    {
        perror( fname );
        exit( EXIT_FAILURE );
    }
    addr = mem_addr;
    while ( ( c = fgetc( fp ) ) != EOF )
        elfram[addr++] = c;

    fclose( fp );    
    return 0;
}

int load_from_command( const char *code )
{
    int hi;
    int lo;

    while( *code != '\0' )
    {
        if ( isspace( *code ) )
            continue;
        hi = asxhex( *code );
        if ( hi < 0 )
        {
            fprintf( stderr, "Illegal character: %c\n", *code );
            exit( EXIT_FAILURE );
        }
        code++;
        if ( *code == '\0' )
        {
            fprintf( stderr, "Stray character at end of line: %c\n", *(code-1) );
            exit( EXIT_FAILURE );
        }
        lo = asxhex( *code );
        if ( lo < 0 )
        {
            fprintf( stderr, "Illegal character: %c\n", *code );
            exit( EXIT_FAILURE );
        }
        code++;
        elfram[mem_addr++] = ( hi << 4 ) | lo;
    }
    return 0;
}

// Load a data file.  The file is a stream of 2-char hex digits.  An optional colon precedes
// four hex chars interpreted as a memory address.  Whitespace is ignored except between digits
// of a byte.
// NOTE: Does not yet support embedded address fields
int load_file( const char *fname )
{
    FILE *fp;
    int c;
    int hi;
    int lo;

    if ( ( fp = fopen( fname, "rb" ) ) == NULL )
    {
        perror( fname );
        exit( EXIT_FAILURE );
    }
    while ( ( c = fgetc( fp ) ) != EOF )
    {
        if ( isspace( c ) )
            continue;
        hi = asxhex( c );
        if ( hi < 0 )
        {
            fprintf( stderr, "%s: illegal character %c\n", fname, c );
            fclose( fp );
            exit( EXIT_FAILURE );
        }
        if ( ( c = fgetc( fp )) == EOF )
        {
            fprintf( stderr, "%s: unexpected EOF\n", fname );
            fclose( fp );
            exit( EXIT_FAILURE );
        }

        lo = asxhex( c );
        if ( lo < 0 )
        {
            fprintf( stderr, "%s: illegal character %c\n", fname, c );
            fclose( fp );
            exit( EXIT_FAILURE );
        }
        elfram[mem_addr++] = ( hi << 4 ) | lo;
    }
    fclose( fp );
    return 0;
}

//	Load an Intel hex format file
/*	File format:
    leading garbage !=':' is legal!
    : colon
    1-byte count of data bytes to follow
    2-byte address
    1-byte record type  00 = data, 01 = eof, all others ignored.
    Data bytes
    1-byte checksum
*/
int load_hex( const char *fname )
{
    FILE *fp;
    int addr;
    char buf[512];	// line buffer
    int csum;		// checksum
    int count;		// count of data bytes to read from record
    int n;
    char *p;
    int i;

    if ( ( fp = fopen( fname, "rb" ) ) == NULL )
    {
        perror( fname );
        exit( EXIT_FAILURE );
    }
    while ( ( fgets( buf, sizeof( buf), fp ) ) != NULL )
    {
        puts( buf );
        // Extension to spec: Allow comments prefixed by '# or ';'
        if ( buf[0] == '#' || buf[0] == ';' )
            continue;
        csum = 0;
        count = 0;
        addr = 0;
        p = buf;
        if ( *p != ':' )
        {
            char *q;
            if ( ( q = strchr( p, '\n' ) ) != NULL )
                *q = '\0';
            fprintf( stderr, "Bad data:\n \"%s\"\n", buf );
            fclose( fp );
            exit( EXIT_FAILURE );
        }
        p++;
        // get byte count
        n = next_byte( p );
        csum += n;
        p += 2;
        count = n;
        // get address
        n = next_byte( p );
        csum += n;
        p += 2;
        addr = n << 8;
        n = next_byte( p );
        csum += n;
        p += 2;
        addr += n;
        // get record type
        n = next_byte( p );
        csum += n;
        p += 2;
        if ( n != '\0' )
            continue;	// ignore any record that's not data
        // read data bytes and store in vram
        mem_addr = addr;
        for ( i = 0; i < count; i++ )
        {
            n = next_byte( p );
            csum += n;
            p += 2;
            elfram[mem_addr++] = n;
        }
        // read checksum
        n = next_byte( p );
        csum += n;
        csum &= 0xFF;
        if ( csum != 0 )
        {
            char *q;
            if ( ( q = strchr( buf, '\n' ) ) != NULL )
                *q = '\0';
            fprintf( stderr, "Bad checksum in:\n\"%s\"\n", buf );
            fclose( fp );
            exit( EXIT_FAILURE );
        }
    }

    fclose( fp );
    return 0;
}

int next_byte( const char *p )
{
    int hi;
    int lo;

    hi = asxhex( *p );
    if ( hi < 0 )
    {
        fprintf( stderr, "Bad char '%c' in hex file.\n", *p );
        fcloseall( );
        exit( EXIT_FAILURE );
    }
    lo = asxhex( *(p+1) );
    if ( lo < 0 )
    {
        fprintf( stderr, "Bad char '%c' in hex file.\n", *p );
        fcloseall( );
        exit( EXIT_FAILURE );
    }
    return ( hi << 4 ) + lo;
}

void ramdump( int start, int range )
{
    int addr;
    char buf[81];
    byte c;
    int i;
    char *p;

    addr = start;
    for ( int l = 0; l < 8; l++ )		// For now, just print 8 lines of 16 bytes
    {
        p = buf;
        p += sprintf( p, "%04X:", addr );
        for ( i = 0; i < 16; i++ )
        {
            if ( ( i & 1 ) == 0 )
                *p++ = ' ';
            if ( i  ==  8 )
                *p++ = ' ';
            p += sprintf( p, "%02X", elfram[addr+i] );
        }
        *p++ = ' ';
        *p++ = ' ';
        for ( i = 0; i < 16; i++ )
        {
            c = elfram[addr + i];
            if ( ( c < 0x20 ) || ( c > 0x7E ) )
                *p++ = '.';
            else
                *p++ = c;
        }
        *p++ = '\0';
        puts( buf );
        addr += 16;
    }
}

unsigned read_addr( void )
{
    unsigned addr;
    addr = digitalRead( MA0 );
    addr |= digitalRead( MA1 ) << 1;
    addr |= digitalRead( MA2 ) << 2;
    addr |= digitalRead( MA3 ) << 3;
    addr |= digitalRead( MA4 ) << 4;
    addr |= digitalRead( MA5 ) << 5;
    addr |= digitalRead( MA6 ) << 6;
    addr |= digitalRead( MA7 ) << 7;
    return addr;
}

/* We can't truly set the data bus to tristate ( unless there's something in WiringPi
   I'm unaware of). So we'll normally set the bus to INPUT and set it to OUTPUT
   when necessary (controlled by nMRD from the 1802).
*/
void set_db_in( void )
{
    pinMode( DB0, INPUT );
    pinMode( DB1, INPUT );
    pinMode( DB2, INPUT );
    pinMode( DB3, INPUT );
    pinMode( DB4, INPUT );
    pinMode( DB5, INPUT );
    pinMode( DB6, INPUT );
    pinMode( DB7, INPUT );
}

void set_db_out( void )
{
    pinMode( DB0, OUTPUT );
    pinMode( DB1, OUTPUT );
    pinMode( DB2, OUTPUT );
    pinMode( DB3, OUTPUT );
    pinMode( DB4, OUTPUT );
    pinMode( DB5, OUTPUT );
    pinMode( DB6, OUTPUT );
    pinMode( DB7, OUTPUT );
}

void show_help( const char *name )

{
    printf( "\nUsage: %s [-f file -h file -H -s -v] [code]\n", name );
    puts( "Options:" );
    puts( "  -b file   Load binary file into 1802 memory" );
    puts( "  -d        Dump 256 bytes of 1802 memory at end of program" );
    puts( "  -f file   Load file into 1802 memory" );
    puts( "  -h file   Load Intel hex fileinto1802 memory" );
    puts( "  -H        Show this help" );
    puts( "  -n        Max clock cycles to execute" );
    puts( "  -s speed  Set clock speed to speed" );
    puts( "  -v        Show version information" );
}

void show_version( const char *name )
{
    printf( "%s version %d.%d %s\n", name, VERSION_MAJOR, VERSION_MINOR, __DATE__ );
}

// Wait until time for next clock edge, using clock_nanaosleep() and absolute time
void csync( void )
{
    int result;
    struct timespec timeNext;
    struct timespec timeRemain;

//    printf( "entry: %d.%9.9d\n", timeMark.tv_sec, timeMark.tv_nsec );
//    printf( "delay: %d.%9.9d\n", timeCycle.tv_sec, timeCycle.tv_nsec );
    timeNext.tv_nsec = timeMark.tv_nsec + timeCycle.tv_nsec;
    if ( timeNext.tv_nsec > 1000000000 )
    {
        timeNext.tv_sec = 1;
        timeNext.tv_nsec -= 1000000000;
    }
    else
        timeNext.tv_sec = 0;
    timeNext.tv_sec += timeMark.tv_sec;
//    printf( "next: %d.%9.9d\n", timeNext.tv_sec, timeNext.tv_nsec );
    while ( ( result = clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &timeNext, &timeRemain ) ) == EINTR )
        ;
    if ( result != 0 )
    {
        fputs( "Error in clock delay\n", stderr );
        exit( EXIT_FAILURE );
    }
    clock_gettime( CLOCK_MONOTONIC, &timeMark );
//   printf( "exit: %d.%9.9d\n", timeMark.tv_sec, timeMark.tv_nsec );
}

