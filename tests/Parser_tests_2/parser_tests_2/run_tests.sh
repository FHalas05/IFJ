#!/usr/bin/env bash
# Spousti testy prekladace IFJ26 a porovnava navratove kody.
#
# Pouziti:
#   ./run_tests.sh [prepinace] <cesta_k_prekladaci>
#
# Prepinace:
#   -s, --syntax-only   jeste nemate semantiku: testy s ocekavanym kodem >= 3
#                       musi vratit 0 (parser je nesmi odmitnout syntakticky)
#   -e, --ext           vynechat slozku base_only/ (implementujete rozsireni)
#   -f, --filter TEXT   spustit jen testy, jejichz cesta obsahuje TEXT
#   -v, --verbose       u FAIL vypsat cely stderr a obsah testu
#   -q, --quiet         nevypisovat PASS radky
#   -m, --valgrind      spoustet pod valgrindem a hlasit memory leaky
#
# Kazdy test = <nazev>.ifj26 (program) + <nazev>.code
#   1. radek .code: povolene navratove kody oddelene mezerou (napr. "2" nebo "1 2")
#   dalsi radky zacinajici '#': poznamka (vypise se u FAIL)
#
# Slozky:
#   ok/          spravne programy (0)
#   err_lex/     lexikalni chyby (1)
#   err_syntax/  syntakticke chyby (2)
#   err_sem/     semanticke chyby (3-10), syntakticky spravne
#   base_only/   chyba jen v zakladu, s nekterym rozsirenim platne
#   unclear/     zadani to nerika jednoznacne -> vice povolenych kodu,
#                vysledek ber jako informaci a over na foru

SYNTAX_ONLY=0; SKIP_EXT=0; FILTER=""; VERBOSE=0; QUIET=0; VALGRIND=0; BIN=""
while [ $# -gt 0 ]; do
    case "$1" in
        -s|--syntax-only) SYNTAX_ONLY=1 ;;
        -e|--ext) SKIP_EXT=1 ;;
        -f|--filter) shift; FILTER="$1" ;;
        -v|--verbose) VERBOSE=1 ;;
        -q|--quiet) QUIET=1 ;;
        -m|--valgrind) VALGRIND=1 ;;
        -h|--help) sed -n '2,30p' "$0"; exit 0 ;;
        *) BIN="$1" ;;
    esac
    shift
done

DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="${BIN:-./ifj26}"
if [ ! -x "$BIN" ]; then
    echo "Prekladac '$BIN' neexistuje nebo neni spustitelny. Pouziti: $0 [-s] <prekladac>"
    exit 1
fi
if [ "$VALGRIND" -eq 1 ] && ! command -v valgrind >/dev/null; then
    echo "valgrind neni nainstalovany"; exit 1
fi

if [ -t 1 ]; then R='\033[31m'; G='\033[32m'; Y='\033[33m'; B='\033[1m'; N='\033[0m'
else R=''; G=''; Y=''; B=''; N=''; fi

total_pass=0; total_fail=0; leaks=0
failed_list=()
errfile="$(mktemp)"; vgfile="$(mktemp)"
trap 'rm -f "$errfile" "$vgfile"' EXIT

run_dir() {
    local sub="$1"
    [ -d "$DIR/$sub" ] || return
    local pass=0 fail=0
    printf "${B}=== %s ===${N}\n" "$sub"
    for f in "$DIR/$sub"/*.ifj26; do
        local name="$sub/$(basename "$f" .ifj26)"
        [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue
        local codefile="${f%.ifj26}.code"
        local expected
        expected="$(head -n 1 "$codefile")"
        local note
        note="$(grep '^#' "$codefile" | sed 's/^# *//')"

        # v rezimu --syntax-only se semanticke kody (>=3) meni na 0
        if [ "$SYNTAX_ONLY" -eq 1 ]; then
            local conv=""
            for c in $expected; do
                if [ "$c" -ge 3 ]; then conv="$conv 0"; else conv="$conv $c"; fi
            done
            expected="$(echo $conv | tr ' ' '\n' | sort -un | tr '\n' ' ' | sed 's/ $//')"
        fi

        local code
        if [ "$VALGRIND" -eq 1 ]; then
            timeout 30 valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect \
                --error-exitcode=200 -q --log-file="$vgfile" "$BIN" < "$f" > /dev/null 2> "$errfile"
            code=$?
        else
            timeout 5 "$BIN" < "$f" > /dev/null 2> "$errfile"
            code=$?
        fi

        local okcode=0
        for c in $expected; do [ "$code" -eq "$c" ] && okcode=1; done

        if [ "$okcode" -eq 1 ]; then
            pass=$((pass + 1))
            [ "$QUIET" -eq 0 ] && printf "${G}PASS${N} %-60s (%s)\n" "$name" "$code"
        else
            fail=$((fail + 1))
            failed_list+=("$name")
            local shown="$code"
            [ "$code" -eq 124 ] && shown="TIMEOUT (zacykleni?)"
            [ "$code" -eq 200 ] && shown="VALGRIND chyba pameti"
            [ "$code" -ge 128 ] && [ "$code" -ne 200 ] && shown="$code (SIGNAL $((code - 128)) - pad programu)"
            printf "${R}FAIL${N} %-60s ocekavano [%s], dostal %s\n" "$name" "$expected" "$shown"
            [ -n "$note" ] && printf "       ${Y}pozn.:${N} %s\n" "$note"
            if [ "$VERBOSE" -eq 1 ]; then
                sed 's/^/       stderr: /' "$errfile"
                echo "       ---- program ----"
                cat -n "$f" | sed 's/^/       /'
                echo "       -----------------"
            else
                [ -s "$errfile" ] && head -n 2 "$errfile" | sed 's/^/       stderr: /'
            fi
        fi
        if [ "$VALGRIND" -eq 1 ] && grep -q "definitely lost\|indirectly lost\|Invalid" "$vgfile"; then
            leaks=$((leaks + 1))
            printf "       ${Y}valgrind:${N} %s\n" "$(grep -m1 'lost\|Invalid' "$vgfile")"
        fi
    done
    total_pass=$((total_pass + pass)); total_fail=$((total_fail + fail))
    printf "  -> %d PASS, %d FAIL\n\n" "$pass" "$fail"
}

run_dir ok
run_dir err_lex
run_dir err_syntax
run_dir err_sem
[ "$SKIP_EXT" -eq 0 ] && run_dir base_only
run_dir ext_funexp
run_dir unclear

printf "${B}Celkem: ${G}%d PASS${N}${B}, ${R}%d FAIL${N}\n" "$total_pass" "$total_fail"
[ "$VALGRIND" -eq 1 ] && printf "Testy s chybou pameti: %d\n" "$leaks"
[ "$SYNTAX_ONLY" -eq 1 ] && echo "(rezim --syntax-only: semanticke testy musely vratit 0)"
if [ "$total_fail" -gt 0 ]; then
    echo; echo "Neprosle testy:"; printf '  %s\n' "${failed_list[@]}"
fi
[ "$total_fail" -eq 0 ]
