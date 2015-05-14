/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "spi_flash_platform_interface.h"
#include "MicoPlatform.h"

#if defined ( USE_MICO_SPI_FLASH )

int sflash_platform_init ( /*@shared@*/ void* peripheral_id, /*@out@*/ void** platform_peripheral_out )
{
    UNUSED_PARAMETER( peripheral_id );  /* Unused due to single SPI Flash */

    if ( kNoErr != MicoSpiInitialize( &mico_spi_flash ) )
    {
        /*@-mustdefine@*/ /* Lint: failed - do not define platform peripheral */
        return -1;
        /*@+mustdefine@*/
    }

    if( platform_peripheral_out != NULL)
      *platform_peripheral_out = NULL;
    
    return 0;
}


extern int sflash_platform_send_recv ( const void* platform_peripheral, /*@in@*/ /*@out@*/ sflash_platform_message_segment_t* segments, unsigned int num_segments  )
{
    UNUSED_PARAMETER( platform_peripheral );
    MicoSpiFinalize( &mico_spi_flash );
    
    if ( 0 != sflash_platform_init( NULL, NULL ) )
    {
        return -1;
    }

    if ( kNoErr != MicoSpiTransfer( &mico_spi_flash, (mico_spi_message_segment_t*) segments, (uint16_t) num_segments ) )
    {
        return -1;
    }

    return 0;
}
#else
int sflash_platform_init( /*@shared@*/ void* peripheral_id, /*@out@*/ void** platform_peripheral_out )
{
    UNUSED_PARAMETER( peripheral_id );
    UNUSED_PARAMETER( platform_peripheral_out );
    return -1;
}

extern int sflash_platform_send_recv( const void* platform_peripheral, /*@in@*//*@out@*/sflash_platform_message_segment_t* segments, unsigned int num_segments )
{
    UNUSED_PARAMETER( platform_peripheral );
    UNUSED_PARAMETER( segments );
    UNUSED_PARAMETER( num_segments );
    return -1;
}
#endif
