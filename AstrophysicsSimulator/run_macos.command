#!/bin/bash
cd "$(dirname "$0")" || exit 1
./build_macos.sh
status=$?
if [[ $status -ne 0 ]]; then
  echo ""
  echo "Ошибка сборки. Проверьте, что установлены Xcode CLI, CMake и Qt 6."
  echo "Команды: xcode-select --install ; brew install cmake qt"
  read -r -p "Нажмите Enter для закрытия окна..." _
fi
exit "$status"
