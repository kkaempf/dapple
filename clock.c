/*
 * ClockData[ 0x00 ] = Year
 * ClockData[ 0x01 ] = Century
 * ClockData[ 0x02 ] = Month
 * ClockData[ 0x03 ] = Day
 * ClockData[ 0x04 ] = Day of Week
 * ClockData[ 0x05 ] = Hour
 * ClockData[ 0x06 ] = Minute
 * ClockData[ 0x07 ] = Second
 *
 * ClockData[ 0x08 ] = Mode
 * ClockData[ 0x09 ] = String Length
 * ClockData[ 0x0a ] = Character Position
 * ClockData[ 0x0b ] = Character
 *
 */

unsigned char ClockData[ 16 ];

/* Given a printf-like format string, build a string in buf that contains
   the appropriately formatted string. Format commands are as follows:
     %y = year
     %c = century
     %o = month
     %d = day
     %w = day of week
     %h = hour
     %m = minute
     %s = second
*/
void FormatDateTime( char *buf, char *fmt )
{
    int idx, done;

    done = 0;
    while ( !done )
    {
        if ( *fmt == '%' )
        {
            /* Handle % formatting */
            fmt++; /* skip % */
            idx = -1;
            switch ( *fmt )
            {
            case 'Y':
            case 'y':
                idx = 0;
                break;
            case 'C':
            case 'c':
                idx = 1;
                break;
            case 'O':
            case 'o':
                idx = 2;
                break;
            case 'D':
            case 'd':
                idx = 3;
                break;
            case 'W':
            case 'w':
                idx = 4;
                break;
            case 'H':
            case 'h':
                idx = 5;
                break;
            case 'M':
            case 'm':
                idx = 6;
                break;
            case 'S':
            case 's':
                idx = 7;
                break;
            case 0:
                done = 1;
                break;
            }
            if ( idx!=-1 )
            {
                /* found a valid format specifier */
                *buf++ = ( ClockData[ idx ] >>   4 ) + '0';
                *buf++ = ( ClockData[ idx ] &  0xf ) + '0';
            }
            else
            {
                /* invalid format specifier: copy the character */
                *buf++ = *fmt;
            }
            fmt++;
        }
        else {
            /* not a % format */
            if ( *fmt == 0 )
            {
                done = 1;
            }
            *buf++ = *fmt++;
        }
    }
    *buf=0;
}

unsigned char ReadClockIO( unsigned short int Address )
{
    char string[ 256 ];
    int Loop;

    /* Get Date */
//    _AH = 0x2a;
//    geninterrupt (0x21);
//    ClockData[ 0x00 ] = _CL; /* Year, low byte e.g. 1994=0x07ca, byte=0xca */
//    ClockData[ 0x01 ] = _CH; /* Year, high byte, e.g. 0x07 */
//    ClockData[ 0x02 ] = _DH; /* Month, Jan=1,... */
//    ClockData[ 0x03 ] = _DL; /* Day, 1-31 */
//    ClockData[ 0x04 ] = _AL; /* Day-of-week 0=Sunday,... */
    /* Get Time */
//    _AH = 0x2c;
//    geninterrupt (0x21);
//    ClockData[ 0x05 ]=_CH; /* Hour, 0-23 */
///    ClockData[ 0x06 ]=_CL; /* Minute 0-59 */
//    ClockData[ 0x07 ]=_DH; /* Second 0-59 */

    /* Now that we've got everything - convert it into BCD */
    Loop = ClockData[ 0x01 ] * 256 + ClockData[ 0x00 ];
    ClockData[ 0x00 ] = Loop /   10 % 10 * 16 + Loop       % 10;
    ClockData[ 0x01 ] = Loop / 1000 % 10 * 16 + Loop / 100 % 10;
    for ( Loop = 2; Loop < 8; Loop++ )
    {
        ClockData[ Loop ] = ClockData[ Loop ] / 10 % 10 * 16 +
                            ClockData[ Loop ]      % 10;
    }

    /* Build appropriately formatted string */
//  ; $A3 = '#' : ProDOS format "oo,ww,dd,hh,mm"
//  ; $BE = '>' : Integer BASIC format   "oo/dd  hh;mm;ss;.000" ?
//  ; $A0 = ' ' : Applesoft BASIC format "oo/dd  hh;mm;ss;.000"
//  ; $A5 = '%' : Applesoft BASIC format "oo/dd  hh;mm;ss;.000"
//  ; $A4 = '$' : My custom ProDOS clock format "yy/oo/dd hh:mm:ss" for use with
    switch ( ClockData[ 0x08 ] )
    {
    case 0xa3: /* ProDOS format */
        FormatDateTime( string, "%o,%w,%d,%h,%m" );
        break;
    case 0xbe:
    case 0xa0:
    case 0xa5: /* Applesoft BASIC format */
        FormatDateTime( string, " %o/%d  %h;%m;%s;.000" );
        break;
    case 0xa4: /* Custom ProDOS format */
        FormatDateTime( string, "%y/%o/%d %h;%m;%s" );
        break;
    }
    /* set high bit for all characters */
    Loop = 0;
    while ( string[ Loop ] != 0 )
    {
        string[ Loop++ ] |= 0x80;
    }
    /* set return character */
    ClockData[ 0x0b ] = string[ ClockData[ 0x0a ] ];

    /* Return appropriate value */
    return ClockData[ Address & 0x0f ];
}

#pragma argsused
void WriteClockIO( unsigned short int Address, unsigned char Data )
{
    switch ( Address & 0x0f )
    {
     case 0x08:
        /* Set mode */
        switch ( Data )
        {
        case 0xa3:
        case 0xbe:
        case 0xa0:
        case 0xa5:
        case 0xa4:
            ClockData[ 0x08 ] = Data;
            break;
        }
        /* Set lengths */
        switch ( ClockData[ 0x08 ] )
        {
        case 0xa3:
            ClockData[ 0x09 ] = 14;
            break;
        case 0xbe:
        case 0xa0:
        case 0xa5:
            ClockData[ 0x09 ] = 21;
            break;
        case 0xa4:
            ClockData[ 0x09 ] = 17;
            break;
        }
        break;
     case 0x0a:
        /* Set current character position */
        if ( Data < ClockData[ 0x09 ] )
        {
            ClockData[ 0x0a ] = Data;
        }
        break;
    }
    /* Update current clock data */
    ReadClockIO( Address );
}
