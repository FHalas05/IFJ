# Testy překladače IFJ26 (hlavně parser)

320 testů psaných **černou skříňkou** podle zadání a demo slajdů (ne podle vaší implementace).
Překladač čte program ze stdin; testy kontrolují **jen návratový kód**.

## Spuštění

```bash
chmod +x tests/run_tests.sh
./tests/run_tests.sh -s ./ifj26        # dokud nemáte sémantiku
./tests/run_tests.sh ./ifj26           # plný režim (vč. kódů 3–10)
./tests/run_tests.sh -q ./ifj26        # jen FAILy
./tests/run_tests.sh -v -f err_syntax/147 ./ifj26   # jeden test, celý stderr + program
./tests/run_tests.sh -m -s ./ifj26     # pod valgrindem (memory leaky)
./tests/run_tests.sh -e ./ifj26        # bez base_only/ (až budete mít rozšíření)
```

`-s` / `--syntax-only`: testy, které čekají sémantickou chybu (≥ 3), musí vrátit **0**.
Tím se ověří, že parser neodmítá syntakticky správné programy.

## Složky

| Složka | Počet | Očekáváno | Co testuje |
|---|---|---|---|
| `ok/` | 60 | 0 | správné programy: všechny konstrukce, demo programy ze slajdů 6, 22, 25, 36, 37, 40, 52, odsazení, komentáře, závorky přes řádky, středníky, priority operátorů, literály |
| `err_lex/` | 29 | 1 | tab na začátku řádku, neznámé znaky, neukončené řetězce a komentáře (i vnořené), špatné escape sekvence a čísla |
| `err_syntax/` | 126 | 2 | prolog, hlavička funkce, zakázané NL (slajd 22/33), prázdné bloky, `if/else/while`, výrazy, volání |
| `err_sem/` | 47 | 3–7 | nedefinované symboly, redefinice, argumenty, typy, zápis do `val` |
| `base_only/` | 30 | 2 (někde 1) | v základu chyba, s rozšířením platné (FUNEXP, BOOLTHEN, CONTROL…). **Oficiální testy je v základu nekontrolují.** |
| `unclear/` | 28 | víc možností | zadání to neříká jednoznačně. Ověřte na fóru a pak si v `.code` nechte jen jeden kód. |

Každý test tvoří dva soubory: `nazev.ifj26` (program) a `nazev.code`.
V `.code` je na 1. řádku seznam povolených kódů (`2`, nebo `1 2`) a na dalších řádcích s `#` poznámka.
Poznámka se vypíše, když test neprojde.

## Na co si dát pozor

- **Jak testy vznikly.** Očekávané kódy 0/1/2 jsem ověřil nezávislým referenčním parserem napsaným jen podle zadání.
  Sémantické kódy (3–10) jsou podle tabulky chyb ze zadání.
- **`ok/47_builtins`** používá všechny vestavěné funkce. Pokud v sémantice spadne s kódem 5, zkontrolujte signatury v zadání.
- **`ok/03_demo_slide22_exact`** obsahuje koncovou čárku `add(x, 1,⟨NL⟩)`. Pokud scanner zahazuje NL uvnitř závorek,
  parser musí vědět, že před `)` byl konec řádku (jinak nerozlišíte `ok/31` od `err_syntax/151`).
- **Chybné odsazení** (`unclear/09`, `10`, `11`): chyba určitě, ale jestli 1, nebo 2, rozhodne fórum.
- **Lexikální chyba až za syntaktickou**: parser řídí scanner, takže vyhraje chyba, na kterou se narazí dřív.
  V `err_lex/` je lexikální chyba vždy první.

## Přidání vlastního testu

Vytvořte `err_syntax/200_muj_test.ifj26` a `err_syntax/200_muj_test.code` s obsahem `2`. Hotovo.
Celou sadu umí znovu vygenerovat `gen_tests.py`: `python3 gen_tests.py tests_out`.
