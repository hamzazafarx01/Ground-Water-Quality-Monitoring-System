- Sensor Calibration

## TDS Sensor
The TDS sensor reads analog voltage and estimates dissolved solids in ppm.

Calibration steps:
1. Read raw ADC value from ESP32.
2. Convert ADC value to voltage.
3. Apply calibration factor.
4. Test with known clean water sample.
5. Adjust calibration factor if reading is inaccurate.

-  Turbidity Sensor
The turbidity sensor detects how cloudy the water is.

Calibration steps:
1. Test with clean water.
2. Record analog value.
3. Test with muddy/cloudy water.
4. Record analog value.
5. Set threshold for clear, moderate, and dirty water.

-  Temperature Sensor
DS18B20 is used to measure water temperature. Temperature helps improve the accuracy of water quality readings.
