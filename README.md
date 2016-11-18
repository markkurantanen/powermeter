# powermeter reader on ESP8266

This is a sample code of a working pulse reader for my power meter.
The reader is an ESP8266-01 with an attached phototransistor TLS257.
The pulse reader reads optical flashes of the power meter, and will update those to MQTT server on a raspberry PI at every 20 seconds.
