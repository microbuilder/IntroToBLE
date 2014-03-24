/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */


#ifndef _COMPILER_ABSTRACTION_H
#define _COMPILER_ABSTRACTION_H

/*lint ++flb "Enter library region" */

#if defined ( __CC_ARM )
    
    #ifndef __ASM
        #define __ASM            __asm                                  /*!< asm keyword for ARM Compiler                  */
    #endif
    
    #ifndef __INLINE
        #define __INLINE         __inline                               /*!< inline keyword for ARM Compiler               */
    #endif
    
    #define GET_SP()         __current_sp()                             /*!> read current SP function for ARM Compiler     */
  
#elif defined ( __ICCARM__ )
    
    #ifndef __ASM
        #define __ASM            __asm                                  /*!< asm keyword for IAR Compiler                  */
    #endif
    
    #ifndef __INLINE
        #define __INLINE         inline                                 /*!< inline keyword for IAR Compiler. Only available in High optimization mode! */
    #endif
    
    #define GET_SP()         __get_SP()                                 /*!> read current SP function for IAR Compiler     */
    
#elif defined   ( __GNUC__ )
    
    #ifndef __ASM
        #define __ASM            __asm                                  /*!< asm keyword for GNU Compiler                  */
    #endif
    
    #ifndef __INLINE
        #define __INLINE         inline                                 /*!< inline keyword for GNU Compiler               */
    #endif
    
    #define GET_SP()         gcc_current_sp()                           /*!> read current SP function for GNU Compiler     */

    static inline unsigned int gcc_current_sp(void)
    {
        register unsigned sp asm("sp");
        return sp;
    }
    
#elif defined   ( __TASKING__ )
        
    #ifndef __ASM        
        #define __ASM            __asm                                  /*!< asm keyword for TASKING Compiler              */
    #endif
    
    #ifndef __INLINE
        #define __INLINE         inline                                 /*!< inline keyword for TASKING Compiler           */
    #endif
    
    #define GET_SP()         __get_MSP()                                /*!> read current SP function for TASKING Compiler */
    
#endif

/*lint --flb "Leave library region" */

#endif
