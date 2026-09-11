#ifndef SANKATNET_ADS1115_DRIVER_H
#define SANKATNET_ADS1115_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_ADS1X15.h>

#include "../config/PinConfig.h"
#include "../config/Calibration.h"


namespace SankatNet {


class ADS1115Driver {

public:

    // ========================================================
    // CONSTRUCTOR
    // ========================================================

    ADS1115Driver()
        : adc1Ready(false),
          adc2Ready(false),
          initialized(false)
    {
    }


    // ========================================================
    // BEGIN
    // ========================================================

    bool begin()
    {
        /*
         * ADS1115 #1
         *
         * Address: 0x48
         *
         * A0 -> MQ-2
         * A1 -> MQ-135
         * A2 -> Water Level
         * A3 -> Soil Moisture
         */

        adc1Ready =
            adc1.begin(
                Pins::I2CAddress::ADS1115_1,
                &Wire
            );


        /*
         * ADS1115 #2
         *
         * Address: 0x49
         *
         * A0 -> Turbidity
         * A1 -> pH interface
         * A2 -> TDS interface
         * A3 -> Reserved
         */

        adc2Ready =
            adc2.begin(
                Pins::I2CAddress::ADS1115_2,
                &Wire
            );


        if (
            adc1Ready
        ) {

            adc1.setGain(
                GAIN_ONE
            );

            adc1.setDataRate(
                RATE_ADS1115_128SPS
            );
        }


        if (
            adc2Ready
        ) {

            adc2.setGain(
                GAIN_ONE
            );

            adc2.setDataRate(
                RATE_ADS1115_128SPS
            );
        }


        initialized =
            adc1Ready ||
            adc2Ready;


        return initialized;
    }


    // ========================================================
    // STATUS
    // ========================================================

    bool isInitialized() const
    {
        return initialized;
    }


    bool isADC1Ready() const
    {
        return adc1Ready;
    }


    bool isADC2Ready() const
    {
        return adc2Ready;
    }


    // ========================================================
    // MQ-2
    // ========================================================

    bool readMQ2(
        uint16_t& value
    )
    {
        int16_t raw;


        if (
            !readADC1(
                ADS1X15_REG_CONFIG_MUX_SINGLE_0,
                raw
            )
        ) {

            return false;
        }


        value =
            Calibration::MQ2::calibrate(
                raw
            );


        return true;
    }


    // ========================================================
    // MQ-135
    // ========================================================

    bool readMQ135(
        uint16_t& value
    )
    {
        int16_t raw;


        if (
            !readADC1(
                ADS1X15_REG_CONFIG_MUX_SINGLE_1,
                raw
            )
        ) {

            return false;
        }


        value =
            Calibration::MQ135::calibrate(
                raw
            );


        return true;
    }


    // ========================================================
    // WATER LEVEL
    // ========================================================

    bool readWaterLevel(
        uint16_t& value
    )
    {
        int16_t raw;


        if (
            !readADC1(
                ADS1X15_REG_CONFIG_MUX_SINGLE_2,
                raw
            )
        ) {

            return false;
        }


        value =
            Calibration::WaterLevel::normalize(
                raw
            );


        return true;
    }


    // ========================================================
    // SOIL MOISTURE
    // ========================================================

    bool readSoilMoisture(
        uint16_t& percentage
    )
    {
        int16_t raw;


        if (
            !readADC1(
                ADS1X15_REG_CONFIG_MUX_SINGLE_3,
                raw
            )
        ) {

            return false;
        }


        percentage =
            Calibration::SoilMoisture::percentage(
                raw
            );


        return true;
    }


    // ========================================================
    // TURBIDITY
    // ========================================================

    bool readTurbidity(
        uint16_t& value
    )
    {
        int16_t raw;


        if (
            !readADC2(
                ADS1X15_REG_CONFIG_MUX_SINGLE_0,
                raw
            )
        ) {

            return false;
        }


        value =
            Calibration::Turbidity::calibrate(
                raw
            );


        return true;
    }


    // ========================================================
    // PH
    // ========================================================

    bool readPH(
        float& ph
    )
    {
        int16_t raw;


        if (
            !readADC2(
                ADS1X15_REG_CONFIG_MUX_SINGLE_1,
                raw
            )
        ) {

            return false;
        }


        /*
         * Convert ADS1115 counts to voltage.
         *
         * GAIN_ONE corresponds to ±4.096 V.
         */

        float voltage =
            adcCountsToVoltage(
                raw
            );


        ph =
            Calibration::PH::fromVoltage(
                voltage
            );


        return true;
    }


    // ========================================================
    // TDS
    // ========================================================

    bool readTDS(
        uint16_t& value
    )
    {
        int16_t raw;


        if (
            !readADC2(
                ADS1X15_REG_CONFIG_MUX_SINGLE_2,
                raw
            )
        ) {

            return false;
        }


        float voltage =
            adcCountsToVoltage(
                raw
            );


        /*
         * TDS calibration uses water temperature.
         *
         * The driver does not own the DS18B20, so the caller
         * can use the temperature-aware overload below.
         */

        value =
            Calibration::TDS::fromVoltage(
                voltage,
                25.0f
            );


        return true;
    }


    // ========================================================
    // TDS WITH TEMPERATURE COMPENSATION
    // ========================================================

    bool readTDS(
        float waterTemperature,
        uint16_t& value
    )
    {
        int16_t raw;


        if (
            !readADC2(
                ADS1X15_REG_CONFIG_MUX_SINGLE_2,
                raw
            )
        ) {

            return false;
        }


        float voltage =
            adcCountsToVoltage(
                raw
            );


        value =
            Calibration::TDS::fromVoltage(
                voltage,
                waterTemperature
            );


        return true;
    }


    // ========================================================
    // GENERIC ADC1 READ
    // ========================================================

    bool readADC1Channel(
        uint8_t channel,
        int16_t& value
    )
    {
        if (
            channel > 3
        ) {

            return false;
        }


        if (
            !adc1Ready
        ) {

            return false;
        }


        switch (
            channel
        ) {

            case 0:

                return readADC1(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_0,
                    value
                );


            case 1:

                return readADC1(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_1,
                    value
                );


            case 2:

                return readADC1(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_2,
                    value
                );


            case 3:

                return readADC1(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_3,
                    value
                );


            default:

                return false;
        }
    }


    // ========================================================
    // GENERIC ADC2 READ
    // ========================================================

    bool readADC2Channel(
        uint8_t channel,
        int16_t& value
    )
    {
        if (
            channel > 3
        ) {

            return false;
        }


        if (
            !adc2Ready
        ) {

            return false;
        }


        switch (
            channel
        ) {

            case 0:

                return readADC2(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_0,
                    value
                );


            case 1:

                return readADC2(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_1,
                    value
                );


            case 2:

                return readADC2(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_2,
                    value
                );


            case 3:

                return readADC2(
                    ADS1X15_REG_CONFIG_MUX_SINGLE_3,
                    value
                );


            default:

                return false;
        }
    }


    // ========================================================
    // ADC COUNTS → VOLTAGE
    // ========================================================

    float adcCountsToVoltage(
        int16_t counts
    ) const
    {
        /*
         * ADS1115 GAIN_ONE:
         *
         * Full scale = ±4.096 V
         *
         * 16-bit signed range:
         *
         * -32768 ... +32767
         *
         * LSB ≈ 125 µV
         */

        constexpr float LSB =
            4.096f / 32768.0f;


        return static_cast<float>(
            counts
        ) * LSB;
    }


private:

    // ========================================================
    // ADS1115 OBJECTS
    // ========================================================

    Adafruit_ADS1115 adc1;

    Adafruit_ADS1115 adc2;


    // ========================================================
    // STATUS
    // ========================================================

    bool adc1Ready;

    bool adc2Ready;

    bool initialized;


    // ========================================================
    // INTERNAL ADC1 READ
    // ========================================================

    bool readADC1(
        uint16_t mux,
        int16_t& value
    )
    {
        if (
            !adc1Ready
        ) {

            return false;
        }


        /*
         * readADC_SingleEnded() is used for the actual channel.
         *
         * Convert the MUX identifier into the corresponding
         * physical channel.
         */

        uint8_t channel;


        switch (
            mux
        ) {

            case ADS1X15_REG_CONFIG_MUX_SINGLE_0:
                channel = 0;
                break;

            case ADS1X15_REG_CONFIG_MUX_SINGLE_1:
                channel = 1;
                break;

            case ADS1X15_REG_CONFIG_MUX_SINGLE_2:
                channel = 2;
                break;

            case ADS1X15_REG_CONFIG_MUX_SINGLE_3:
                channel = 3;
                break;

            default:
                return false;
        }


        int16_t result =
            adc1.readADC_SingleEnded(
                channel
            );


        /*
         * ADS1115 normally returns a valid signed conversion.
         * A negative value is possible for differential operation,
         * but these channels are single-ended.
         */

        if (
            result < 0
        ) {

            value = 0;

            return true;
        }


        value =
            result;


        return true;
    }


    // ========================================================
    // INTERNAL ADC2 READ
    // ========================================================

    bool readADC2(
        uint16_t mux,
        int16_t& value
    )
    {
        if (
            !adc2Ready
        ) {

            return false;
        }


        uint8_t channel;


        switch (
            mux
        ) {

            case ADS1X15_REG_CONFIG_MUX_SINGLE_0:
                channel = 0;
                break;

            case ADS1X15_REG_CONFIG_MUX_SINGLE_1:
                channel = 1;
                break;

            case ADS1X15_REG_CONFIG_MUX_SINGLE_2:
                channel = 2;
                break;

            case ADS1X15_REG_CONFIG_MUX_SINGLE_3:
                channel = 3;
                break;

            default:
                return false;
        }


        int16_t result =
            adc2.readADC_SingleEnded(
                channel
            );


        if (
            result < 0
        ) {

            value = 0;

            return true;
        }


        value =
            result;


        return true;
    }
};


} // namespace SankatNet


#endif