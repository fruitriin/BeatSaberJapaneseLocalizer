#!/usr/bin/env bash
# quest/common の辞書の照合を試験する。
# rapidjson のヘッダーは、quest/1.40.8 で qpm restore した extern から使う（RAPIDJSON_INCLUDE で差し替えられる）。
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
rapidjson=${RAPIDJSON_INCLUDE:-$root/quest/1.40.8/extern/includes/beatsaber-hook/shared/rapidjson/include}
if [ ! -f "$rapidjson/rapidjson/document.h" ]; then
  echo "rapidjson が見つからない: $rapidjson（quest/1.40.8 で qpm restore するか、RAPIDJSON_INCLUDE を指定する）" >&2
  exit 2
fi
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
${CXX:-clang++} -std=c++20 -O1 -I "$root/quest/common/include" -I "$rapidjson" \
  "$root/quest/common/src/Dictionaries.cpp" "$root/tests/run_tests.cpp" -o "$out/run_tests"
"$out/run_tests" "$root"
