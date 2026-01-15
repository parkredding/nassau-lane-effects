#!/usr/bin/env bash
set -euo pipefail

GPIO_INPUTS=(14 15 16 2 3 4 5 6 7)
GPIO_OUTPUTS=(17 27 22)

for pin in "${GPIO_INPUTS[@]}"; do
  if [ ! -d "/sys/class/gpio/gpio${pin}" ]; then
    echo "${pin}" > /sys/class/gpio/export
  fi
  echo "in" > "/sys/class/gpio/gpio${pin}/direction"
done

for pin in "${GPIO_OUTPUTS[@]}"; do
  if [ ! -d "/sys/class/gpio/gpio${pin}" ]; then
    echo "${pin}" > /sys/class/gpio/export
  fi
  echo "out" > "/sys/class/gpio/gpio${pin}/direction"
  echo "0" > "/sys/class/gpio/gpio${pin}/value"
done

exit 0
