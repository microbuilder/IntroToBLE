/**************************************************************************/
/*!
    @file     btle.h
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, K. Townsend (microBuilder.eu)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#ifndef _BTLE_H_
#define _BTLE_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// INCLUDES
//--------------------------------------------------------------------+
#include "common/common.h"

#include "ble_srv_common.h"
#include "ble.h"

//#include "heart_rate.h"
#include "btle_uart.h"

//--------------------------------------------------------------------+
// TYPE
//--------------------------------------------------------------------+
typedef struct {
  error_t (* const init) (void);
  void (* const event_handler) (ble_evt_t * );
}btle_service_driver_t;

typedef struct {
  error_t (* const init) (uint8_t);
  void (* const event_handler) (ble_evt_t * );

  uint8_t const uuid_base[16];
  ble_uuid_t service_uuid;
}btle_service_custom_driver_t;

// https://developer.bluetooth.org/gatt/units/Pages/default.aspx
typedef enum ble_gatt_unit_e
{
  BLE_GATT_CPF_UNIT_NONE                                                   = 0x2700,
  BLE_GATT_CPF_UNIT_LENGTH_METRE                                           = 0x2701,
  BLE_GATT_CPF_UNIT_MASS_KILOGRAM                                          = 0x2702,
  BLE_GATT_CPF_UNIT_TIME_SECOND                                            = 0x2703,
  BLE_GATT_CPF_UNIT_ELECTRIC_CURRENT_AMPERE                                = 0x2704,
  BLE_GATT_CPF_UNIT_THERMODYNAMIC_TEMPERATURE_KELVIN                       = 0x2705,
  BLE_GATT_CPF_UNIT_AMOUNT_OF_SUBSTANCE_MOLE                               = 0x2706,
  BLE_GATT_CPF_UNIT_LUMINOUS_INTENSITY_CANDELA                             = 0x2707,
  BLE_GATT_CPF_UNIT_AREA_SQUARE_METRES                                     = 0x2710,
  BLE_GATT_CPF_UNIT_VOLUME_CUBIC_METRES                                    = 0x2711,
  BLE_GATT_CPF_UNIT_VELOCITY_METRES_PER_SECOND                             = 0x2712,
  BLE_GATT_CPF_UNIT_ACCELERATION_METRES_PER_SECOND_SQUARED                 = 0x2713,
  BLE_GATT_CPF_UNIT_WAVENUMBER_RECIPROCAL_METRE                            = 0x2714,
  BLE_GATT_CPF_UNIT_DENSITY_KILOGRAM_PER_CUBIC_METRE                       = 0x2715,
  BLE_GATT_CPF_UNIT_SURFACE_DENSITY_KILOGRAM_PER_SQUARE_METRE              = 0x2716,
  BLE_GATT_CPF_UNIT_SPECIFIC_VOLUME_CUBIC_METRE_PER_KILOGRAM               = 0x2717,
  BLE_GATT_CPF_UNIT_CURRENT_DENSITY_AMPERE_PER_SQUARE_METRE                = 0x2718,
  BLE_GATT_CPF_UNIT_MAGNETIC_FIELD_STRENGTH_AMPERE_PER_METRE               = 0x2719,
  BLE_GATT_CPF_UNIT_AMOUNT_CONCENTRATION_MOLE_PER_CUBIC_METRE              = 0x271A,
  BLE_GATT_CPF_UNIT_MASS_CONCENTRATION_KILOGRAM_PER_CUBIC_METRE            = 0x271B,
  BLE_GATT_CPF_UNIT_LUMINANCE_CANDELA_PER_SQUARE_METRE                     = 0x271C,
  BLE_GATT_CPF_UNIT_REFRACTIVE_INDEX                                       = 0x271D,
  BLE_GATT_CPF_UNIT_RELATIVE_PERMEABILITY                                  = 0x271E,
  BLE_GATT_CPF_UNIT_PLANE_ANGLE_RADIAN                                     = 0x2720,
  BLE_GATT_CPF_UNIT_SOLID_ANGLE_STERADIAN                                  = 0x2721,
  BLE_GATT_CPF_UNIT_FREQUENCY_HERTZ                                        = 0x2722,
  BLE_GATT_CPF_UNIT_FORCE_NEWTON                                           = 0x2723,
  BLE_GATT_CPF_UNIT_PRESSURE_PASCAL                                        = 0x2724,
  BLE_GATT_CPF_UNIT_ENERGY_JOULE                                           = 0x2725,
  BLE_GATT_CPF_UNIT_POWER_WATT                                             = 0x2726,
  BLE_GATT_CPF_UNIT_ELECTRIC_CHARGE_COULOMB                                = 0x2727,
  BLE_GATT_CPF_UNIT_ELECTRIC_POTENTIAL_DIFFERENCE_VOLT                     = 0x2728,
  BLE_GATT_CPF_UNIT_CAPACITANCE_FARAD                                      = 0x2729,
  BLE_GATT_CPF_UNIT_ELECTRIC_RESISTANCE_OHM                                = 0x272A,
  BLE_GATT_CPF_UNIT_ELECTRIC_CONDUCTANCE_SIEMENS                           = 0x272B,
  BLE_GATT_CPF_UNIT_MAGNETIC_FLEX_WEBER                                    = 0x272C,
  BLE_GATT_CPF_UNIT_MAGNETIC_FLEX_DENSITY_TESLA                            = 0x272D,
  BLE_GATT_CPF_UNIT_INDUCTANCE_HENRY                                       = 0x272E,
  BLE_GATT_CPF_UNIT_THERMODYNAMIC_TEMPERATURE_DEGREE_CELSIUS               = 0x272F,
  BLE_GATT_CPF_UNIT_LUMINOUS_FLUX_LUMEN                                    = 0x2730,
  BLE_GATT_CPF_UNIT_ILLUMINANCE_LUX                                        = 0x2731,
  BLE_GATT_CPF_UNIT_ACTIVITY_REFERRED_TO_A_RADIONUCLIDE_BECQUEREL          = 0x2732,
  BLE_GATT_CPF_UNIT_ABSORBED_DOSE_GRAY                                     = 0x2733,
  BLE_GATT_CPF_UNIT_DOSE_EQUIVALENT_SIEVERT                                = 0x2734,
  BLE_GATT_CPF_UNIT_CATALYTIC_ACTIVITY_KATAL                               = 0x2735,
  BLE_GATT_CPF_UNIT_DYNAMIC_VISCOSITY_PASCAL_SECOND                        = 0x2740,
  BLE_GATT_CPF_UNIT_MOMENT_OF_FORCE_NEWTON_METRE                           = 0x2741,
  BLE_GATT_CPF_UNIT_SURFACE_TENSION_NEWTON_PER_METRE                       = 0x2742,
  BLE_GATT_CPF_UNIT_ANGULAR_VELOCITY_RADIAN_PER_SECOND                     = 0x2743,
  BLE_GATT_CPF_UNIT_ANGULAR_ACCELERATION_RADIAN_PER_SECOND_SQUARED         = 0x2744,
  BLE_GATT_CPF_UNIT_HEAT_FLUX_DENSITY_WATT_PER_SQUARE_METRE                = 0x2745,
  BLE_GATT_CPF_UNIT_HEAT_CAPACITY_JOULE_PER_KELVIN                         = 0x2746,
  BLE_GATT_CPF_UNIT_SPECIFIC_HEAT_CAPACITY_JOULE_PER_KILOGRAM_KELVIN       = 0x2747,
  BLE_GATT_CPF_UNIT_SPECIFIC_ENERGY_JOULE_PER_KILOGRAM                     = 0x2748,
  BLE_GATT_CPF_UNIT_THERMAL_CONDUCTIVITY_WATT_PER_METRE_KELVIN             = 0x2749,
  BLE_GATT_CPF_UNIT_ENERGY_DENSITY_JOULE_PER_CUBIC_METRE                   = 0x274A,
  BLE_GATT_CPF_UNIT_ELECTRIC_FIELD_STRENGTH_VOLT_PER_METRE                 = 0x274B,
  BLE_GATT_CPF_UNIT_ELECTRIC_CHARGE_DENSITY_COULOMB_PER_CUBIC_METRE        = 0x274C,
  BLE_GATT_CPF_UNIT_SURFACE_CHARGE_DENSITY_COULOMB_PER_SQUARE_METRE        = 0x274D,
  BLE_GATT_CPF_UNIT_ELECTRIC_FLUX_DENSITY_COULOMB_PER_SQUARE_METRE         = 0x274E,
  BLE_GATT_CPF_UNIT_PERMITTIVITY_FARAD_PER_METRE                           = 0x274F,
  BLE_GATT_CPF_UNIT_PERMEABILITY_HENRY_PER_METRE                           = 0x2750,
  BLE_GATT_CPF_UNIT_MOLAR_ENERGY_JOULE_PER_MOLE                            = 0x2751,
  BLE_GATT_CPF_UNIT_MOLAR_ENTROPY_JOULE_PER_MOLE_KELVIN                    = 0x2752,
  BLE_GATT_CPF_UNIT_EXPOSURE_COULOMB_PER_KILOGRAM                          = 0x2753,
  BLE_GATT_CPF_UNIT_ABSORBED_DOSE_RATE_GRAY_PER_SECOND                     = 0x2754,
  BLE_GATT_CPF_UNIT_RADIANT_INTENSITY_WATT_PER_STERADIAN                   = 0x2755,
  BLE_GATT_CPF_UNIT_RADIANCE_WATT_PER_SQUARE_METRE_STERADIAN               = 0x2756,
  BLE_GATT_CPF_UNIT_CATALYTIC_ACTIVITY_CONCENTRATION_KATAL_PER_CUBIC_METRE = 0x2757,
  BLE_GATT_CPF_UNIT_TIME_MINUTE                                            = 0x2760,
  BLE_GATT_CPF_UNIT_TIME_HOUR                                              = 0x2761,
  BLE_GATT_CPF_UNIT_TIME_DAY                                               = 0x2762,
  BLE_GATT_CPF_UNIT_PLANE_ANGLE_DEGREE                                     = 0x2763,
  BLE_GATT_CPF_UNIT_PLANE_ANGLE_MINUTE                                     = 0x2764,
  BLE_GATT_CPF_UNIT_PLANE_ANGLE_SECOND                                     = 0x2765,
  BLE_GATT_CPF_UNIT_AREA_HECTARE                                           = 0x2766,
  BLE_GATT_CPF_UNIT_VOLUME_LITRE                                           = 0x2767,
  BLE_GATT_CPF_UNIT_MASS_TONNE                                             = 0x2768,
  BLE_GATT_CPF_UNIT_PRESSURE_BAR                                           = 0x2780,
  BLE_GATT_CPF_UNIT_PRESSURE_MILLIMETRE_OF_MERCURY                         = 0x2781,
  BLE_GATT_CPF_UNIT_LENGTH_ANGSTROM                                        = 0x2782,
  BLE_GATT_CPF_UNIT_LENGTH_NAUTICAL_MILE                                   = 0x2783,
  BLE_GATT_CPF_UNIT_AREA_BARN                                              = 0x2784,
  BLE_GATT_CPF_UNIT_VELOCITY_KNOT                                          = 0x2785,
  BLE_GATT_CPF_UNIT_LOGARITHMIC_RADIO_QUANTITY_NEPER                       = 0x2786,
  BLE_GATT_CPF_UNIT_LOGARITHMIC_RADIO_QUANTITY_BEL                         = 0x2787,
  BLE_GATT_CPF_UNIT_LENGTH_YARD                                            = 0x27A0,
  BLE_GATT_CPF_UNIT_LENGTH_PARSEC                                          = 0x27A1,
  BLE_GATT_CPF_UNIT_LENGTH_INCH                                            = 0x27A2,
  BLE_GATT_CPF_UNIT_LENGTH_FOOT                                            = 0x27A3,
  BLE_GATT_CPF_UNIT_LENGTH_MILE                                            = 0x27A4,
  BLE_GATT_CPF_UNIT_PRESSURE_POUND_FORCE_PER_SQUARE_INCH                   = 0x27A5,
  BLE_GATT_CPF_UNIT_VELOCITY_KILOMETRE_PER_HOUR                            = 0x27A6,
  BLE_GATT_CPF_UNIT_VELOCITY_MILE_PER_HOUR                                 = 0x27A7,
  BLE_GATT_CPF_UNIT_ANGULAR_VELOCITY_REVOLUTION_PER_MINUTE                 = 0x27A8,
  BLE_GATT_CPF_UNIT_ENERGY_GRAM_CALORIE                                    = 0x27A9,
  BLE_GATT_CPF_UNIT_ENERGY_KILOGRAM_CALORIE                                = 0x27AA,
  BLE_GATT_CPF_UNIT_ENERGY_KILOWATT_HOUR                                   = 0x27AB,
  BLE_GATT_CPF_UNIT_THERMODYNAMIC_TEMPERATURE_DEGREE_FAHRENHEIT            = 0x27AC,
  BLE_GATT_CPF_UNIT_PERCENTAGE                                             = 0x27AD,
  BLE_GATT_CPF_UNIT_PER_MILLE                                              = 0x27AE,
  BLE_GATT_CPF_UNIT_PERIOD_BEATS_PER_MINUTE                                = 0x27AF,
  BLE_GATT_CPF_UNIT_ELECTRIC_CHARGE_AMPERE_HOURS                           = 0x27B0,
  BLE_GATT_CPF_UNIT_MASS_DENSITY_MILLIGRAM_PER_DECILITRE                   = 0x27B1,
  BLE_GATT_CPF_UNIT_MASS_DENSITY_MILLIMOLE_PER_LITRE                       = 0x27B2,
  BLE_GATT_CPF_UNIT_TIME_YEAR                                              = 0x27B3,
  BLE_GATT_CPF_UNIT_TIME_MONTH                                             = 0x27B4,
  BLE_GATT_CPF_UNIT_CONCENTRATION_COUNT_PER_CUBIC_METRE                    = 0x27B5,
  BLE_GATT_CPF_UNIT_IRRADIANCE_WATT_PER_SQUARE_METRE                       = 0x27B6
} ble_gatt_unit_t;

error_t btle_init(void);

#ifdef __cplusplus
 }
#endif

#endif /* _BTLE_H_ */

/** @} */
