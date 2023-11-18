FROM shaguarger/platformio as build-stage

WORKDIR /app

COPY platformio.ini /app/
RUN pio pkg install
RUN pip install esptool

ADD . /app
RUN pio run
RUN cd /app/.pio/build/esp32dev && python -m esptool --chip esp32 merge_bin -o merged-firmware.bin --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin

FROM flashspys/nginx-static
COPY static /static
COPY --from=build-stage /app/.pio/build/esp32dev/merged-firmware.bin /static/firmware/merged-firmware.bin