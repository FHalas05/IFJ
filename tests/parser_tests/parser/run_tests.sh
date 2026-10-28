#!/usr/bin/env bash
# Spusti vsechny testy a porovna navratove kody.
#
# Pouziti:   ./run_tests.sh <cesta_k_prekladaci>
# Priklad:   ./tests/parser/run_tests.sh ./ifj26
#
# Slozky:
#   ok/          ocekavany kod 0 (syntakticky spravne programy)
#   err_syntax/  ocekavany kod 2 (syntakticka chyba)
#   err_lex/     ocekavany kod 1 (lexikalni chyba)
#   spec/        zadani/demo to jednoznacne nerika -> jen vypise kod,
#                ocekavana hodnota je v komentari na prvnim radku souboru

BIN="${1:-./ifj26}"
DIR="$(cd "$(dirname "$0")" && pwd)"

if [ ! -x "$BIN" ]; then
    echo "Prekladac '$BIN' neexistuje nebo neni spustitelny."
    echo "Pouziti: $0 <cesta_k_prekladaci>"
    exit 1
fi

RED='\033[31m'; GREEN='\033[32m'; YELLOW='\033[33m'; RESET='\033[0m'
pass=0
fail=0
errfile="$(mktemp)"

run_dir() {
    local sub="$1" expected="$2"
    echo "=== $sub (ocekavano $expected) ==="
    for f in "$DIR/$sub"/*.ifj26; do
        timeout 5 "$BIN" < "$f" > /dev/null 2> "$errfile"
        local code=$?
        local name="$(basename "$f")"
        if [ "$code" -eq "$expected" ]; then
            pass=$((pass + 1))
            printf "${GREEN}PASS${RESET} %s\n" "$name"
        else
            fail=$((fail + 1))
            [ "$code" -eq 124 ] && code="TIMEOUT (zacykleni?)"
            printf "${RED}FAIL${RESET} %s  (dostal %s)\n" "$name" "$code"
            head -n 1 "$errfile" | sed 's/^/       stderr: /'
        fi
    done
    echo
}

run_dir ok 0
run_dir err_syntax 2
run_dir err_lex 1

echo "=== spec (nejasne v zadani - porovnej s komentarem v souboru) ==="
for f in "$DIR/spec"/*.ifj26; do
    timeout 5 "$BIN" < "$f" > /dev/null 2> "$errfile"
    code=$?
    printf "${YELLOW}%-3s${RESET} %-40s %s\n" "$code" "$(basename "$f")" "$(head -n 1 "$f")"
done
echo

rm -f "$errfile"
echo "Vysledek: $pass PASS, $fail FAIL"
[ "$fail" -eq 0 ]
